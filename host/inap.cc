#include <arpa/inet.h>
#include <endian.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <rte_atomic.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_flow.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_random.h>
#include <rte_ring.h>

#include <hiredis/hiredis.h>

#include "common.h"
#include "conf.h"
#include "debug.h"
#include "demo.h"
#include "inap.h"

#include <cassert>
#include <unordered_map>
#include <vector>

volatile bool force_quit = false;

#define MAX_TASK     65536
#define RTE_RAND_MAX UINT64_MAX;

struct rte_ring    *task_todo[MAX_THREAD];
struct rte_ring    *task_done;
struct rte_mempool *task_pool = NULL;

double loss_rate;

void inap_init(void) {
    char name[20];
    for (int loop = 0; loop < MAX_THREAD; loop++) {
        sprintf(name, "TASK_TODO_%d", loop);
        task_todo[loop] = rte_ring_create(name, MAX_TASK, rte_socket_id(), 0);
    }
    task_done = rte_ring_create("TASK_DONE", MAX_TASK * MAX_THREAD, rte_socket_id(), 0);
    task_pool = rte_mempool_create("TASK_POOL", MAX_TASK * MAX_THREAD - 1, sizeof(struct task_t), 0, 0, NULL, NULL, NULL, NULL, rte_socket_id(), 0);
}
void inap_terminate(void) {
    force_quit = true;
}

int push_task(struct task_t new_task) {
    struct conf_t *conf   = get_conf();
    struct task_t *task   = NULL;
    int            status = rte_mempool_get(task_pool, (void **)&task);
    if (unlikely(status != 0)) {
        return -1;
    }

    task->addr = new_task.addr;
    task->op   = new_task.op;
    rte_ring_enqueue(task_todo[new_task.thread_id], (void *)task);
    return 0;
}

int check_task(void) {
    int            key  = -1;
    struct task_t *done = NULL;
    int            ret  = rte_ring_dequeue(task_done, (void **)&done);
    if (ret == 0) {
        key = 1;
        rte_mempool_put(task_pool, done);
    }
    return key;
}

static inline void
send_all_client(uint16_t port_id, uint16_t queue_id, struct rte_mbuf **bufs, uint16_t burst_num) {
    struct rte_mbuf *bufs_tx[CLIENT_SIZE_BURST_TX];
    int              cnt = 0;
    for (int i = 0; i < burst_num; i++) {
        double           randomValue = (double)rte_rand() / RTE_RAND_MAX;
        struct myh_hdr  *h_myh       = rte_pktmbuf_mtod_offset(bufs[i], struct myh_hdr *, LEN_UPTO_IP4);
        uint32_t         seq         = ntohl(h_myh->psn);
        struct kv_pair_t kv_pair;
        {
            if (h_myh->op == 0x00) {
                Dprintf("[Send] Read Data (psn = %d, Key =", seq);
            } else {
                Dprintf("[Send] Write Data (psn = %d, Key = ", seq);
            }
            rte_memcpy(&kv_pair.key, h_myh->key, sizeof(uint8_t) * 16);
            for (int i = 0; i < 16; i++) {
                Dprintf("%02x", kv_pair.key[i]);
            }
            Dprintf(" , Value = ");
            rte_memcpy(&kv_pair.value, h_myh->value, sizeof(uint8_t) * 16);
            for (int i = 0; i < 16; i++) {
                Dprintf("%02x", kv_pair.value[i]);
            }
            Dprintf(" )\n");
        }
        if (randomValue > loss_rate) {
            bufs_tx[cnt++] = bufs[i];
        } else {
            Dprintf("From client to switch, Packet (PSN = %d) Loss %lf\n", seq, randomValue);
            rte_pktmbuf_free(bufs[i]);
        }
    }
    burst_num = cnt;

    uint16_t nb_tx = rte_eth_tx_burst(port_id, queue_id, bufs_tx, burst_num);
    uint16_t temp  = 0;
    if (unlikely(nb_tx < burst_num)) {
        while (nb_tx < burst_num) {
            temp = rte_eth_tx_burst(port_id, queue_id, &bufs_tx[nb_tx], burst_num - nb_tx);
            nb_tx += temp;
        }
    }
}

static inline void
send_all_server(uint16_t port_id, uint16_t queue_id, struct rte_mbuf **bufs, uint16_t burst_num) {
    uint16_t nb_tx = rte_eth_tx_burst(port_id, queue_id, bufs, burst_num);
    uint16_t temp  = 0;
    if (unlikely(nb_tx < burst_num)) {
        while (nb_tx < burst_num) {
            temp = rte_eth_tx_burst(port_id, queue_id, &bufs[nb_tx], burst_num - nb_tx);
            nb_tx += temp;
        }
    }
}

static inline void
gen_packet(struct rte_mbuf *buf, struct conn_t *conn, char *data, uint8_t op, uint32_t seq, std::unordered_map<int, std::vector<int> *> &reuse_map) {
    struct rte_ether_hdr *h_eth = NULL;
    struct rte_ipv4_hdr  *h_ip4 = NULL;
    struct myh_hdr       *h_myh = NULL;

    h_eth             = rte_pktmbuf_mtod(buf, struct rte_ether_hdr *);
    h_eth->src_addr   = conn->s_mac;
    h_eth->dst_addr   = conn->d_mac;
    h_eth->ether_type = htons(RTE_ETHER_TYPE_IPV4);
    buf->pkt_len      = RTE_ETHER_HDR_LEN;
    buf->data_len     = RTE_ETHER_HDR_LEN;

    h_ip4                  = (struct rte_ipv4_hdr *)rte_pktmbuf_append(buf, LEN_IP4_HDR);
    h_ip4->version_ihl     = 0x45;
    h_ip4->type_of_service = 0;
    h_ip4->packet_id       = 0;
    h_ip4->fragment_offset = 0;
    h_ip4->time_to_live    = 0x0f;
    h_ip4->next_proto_id   = PROTO_MYH;
    h_ip4->hdr_checksum    = 0;
    h_ip4->src_addr        = conn->s_ip;
    h_ip4->dst_addr        = conn->d_ip;

    h_myh         = (struct myh_hdr *)rte_pktmbuf_append(buf, LEN_MYH_HDR);
    h_myh->psn    = htonl(seq);
    h_myh->is_req = 0x1;
    h_myh->op     = op & 0x3;

    if (seq % 100000 == 0) {
        printf("seq = %d\n", seq);
    }

    if (likely(data != NULL)) {
        rte_memcpy(h_myh->key, data + seq * sizeof(struct kv_pair_t), sizeof(h_myh->key));
        rte_memcpy(h_myh->value, data + seq * sizeof(struct kv_pair_t) + 16, sizeof(h_myh->value));

        int key;
        rte_memcpy(&key, h_myh->key, sizeof(int));

        // reuse_distance
        uintptr_t ptr_value = 0;
        if (reuse_map.find(key) != reuse_map.end()) {
            auto int_vec = *reuse_map[key];

            int seq_max  = -1;
            int is_exist = 0;
            for (int i = 0; i < int_vec.size(); i++) {
                if (int_vec[i] < seq && int_vec[i] > seq_max) {
                    seq_max = int_vec[i];
                } else if (int_vec[i] == seq) {
                    is_exist = 1;
                }
            }

            if (seq_max == -1) {
                h_myh->reuse_distance = 0;
            } else {
                h_myh->reuse_distance = seq - seq_max;
            }
            if (!is_exist) {
                int_vec.push_back(seq);
            }
        } else {
            h_myh->reuse_distance = 0;
            auto *int_vec         = new std::vector<int>();
            int_vec->push_back(seq);
            reuse_map[key] = int_vec;
        }
    }

    h_ip4->total_length = htons(buf->data_len - RTE_ETHER_HDR_LEN);
}

int lcore_client(void *arg) {
    struct conn_t *conn = (struct conn_t *)arg;
    if (rte_eth_dev_socket_id(conn->port_id) > 0 && rte_eth_dev_socket_id(conn->port_id) != (int)rte_socket_id())
        RTE_LOG(WARNING, APP, "Port %u is on remote NUMA node to polling thread. Performance will not be optimal.\n", conn->port_id);

    struct conf_t   *conf       = get_conf();
    struct myh_hdr  *h_myh      = NULL;
    struct task_t   *task       = NULL;
    struct rte_ring *task_queue = task_todo[conn->ID];
    struct rte_mbuf *bufs_rx[CLIENT_SIZE_BURST_RX];
    struct rte_mbuf *bufs_tx[CLIENT_SIZE_BURST_TX];
    uint16_t         nb_rx, loop, burst_num = 0;
    uint32_t         seq, seq_index, seq_next, seq_base, seq_max;
    uint64_t         ts_cur = 0;
    struct kv_pair_t kv_pair;

    struct seq_state_c_t *ssc = (struct seq_state_c_t *)rte_zmalloc(NULL, sizeof(struct seq_state_c_t), 0);
    ssc->last_sent            = 0xffffffff;
    ssc->last_acked           = 0xffffffff;
    ssc->window_size          = conf->window;
    ssc->window_mask          = (2 * ssc->window_size - 1);
    loss_rate                 = conf->loss_rate;
    seq_base                  = ssc->last_sent + 1;
    if (conf->is_load == 1) {
        seq_max = NUM_KEY - 1;
    } else {
        seq_max = NUM_REQ - 1;
    }

    std::unordered_map<int, std::vector<int> *> reuse_map;

    for (;;) {
        if (rte_ring_dequeue(task_queue, (void **)&task) == 0) {
            printf("task here!");

            for (;;) {
                nb_rx = rte_eth_rx_burst(conn->port_id, conn->queue_id, bufs_rx, CLIENT_SIZE_BURST_RX);
                for (loop = 0; loop < nb_rx; loop++) {
                    double randomValue = (double)rte_rand() / RTE_RAND_MAX;
                    h_myh              = rte_pktmbuf_mtod_offset(bufs_rx[loop], struct myh_hdr *, LEN_UPTO_IP4);
                    seq                = ntohl(h_myh->psn);
                    if (randomValue > loss_rate) {
                        seq_index                 = seq & ssc->window_mask;
                        ssc->state[seq_index].ts  = 0;
                        ssc->state[seq_index].seq = 0;

                        int key;
                        rte_memcpy(&key, h_myh->key, sizeof(int));
                        // reuse_distance
                        if (reuse_map.find(key) != reuse_map.end()) {
                            auto int_vec = *reuse_map[key];
                            if (int_vec.size() == 1) {
                                delete reuse_map[key];
                                reuse_map.erase(key);
                            } else {
                                for (int i = 0; i < int_vec.size(); i++) {
                                    if (int_vec[i] == seq) {
                                        int_vec.erase(int_vec.begin() + i);
                                        break;
                                    }
                                }
                            }
                        }

                        {
                            if (h_myh->op == 0x00) {
                                Dprintf("[Recv] Read Data (psn = %d, Key =", seq);
                            } else {
                                Dprintf("[Recv] Write Data (psn = %d, Key = ", seq);
                            }
                            rte_memcpy(&kv_pair.key, h_myh->key, sizeof(uint8_t) * 16);
                            for (int i = 0; i < 16; i++) {
                                Dprintf("%02x", kv_pair.key[i]);
                            }
                            Dprintf(" , Value = ");
                            rte_memcpy(&kv_pair.value, h_myh->value, sizeof(uint8_t) * 16);
                            for (int i = 0; i < 16; i++) {
                                Dprintf("%02x", kv_pair.value[i]);
                            }
                            Dprintf(" )\n");
                        }
                    } else {
                        Dprintf("From switch to client, Packet (PSN = %d) Loss\n", seq);
                    }
                    rte_pktmbuf_free(bufs_rx[loop]);
                }
                while (likely(ssc->last_acked != ssc->last_sent)) {
                    seq_next  = ssc->last_acked + 1;
                    seq_index = seq_next & ssc->window_mask;
                    if (ssc->state[seq_index].ts == 0)
                        ssc->last_acked = seq_next;
                    else
                        break;
                }

                if (unlikely(ssc->last_acked == seq_max)) {
                    break;
                }

                ts_cur    = rte_rdtsc();
                burst_num = 0;
                if (unlikely(ssc->last_sent == ssc->last_acked + ssc->window_size) || unlikely(ssc->last_sent == seq_max)) {
                    while (burst_num < CLIENT_SIZE_BURST_TX) {
                        seq_next  = ssc->last_acked + burst_num + 1;
                        seq_index = seq_next & ssc->window_mask;
                        if (ssc->state[seq_index].ts > 0 && unlikely(ts_cur > ssc->state[seq_index].ts + 23000000)) {
                            bufs_tx[burst_num] = rte_pktmbuf_alloc(conn->mbuf_pool);
                            gen_packet(bufs_tx[burst_num++], conn, task->addr, task->op[seq_next], seq_next, reuse_map);
                            ssc->state[seq_index].ts = ts_cur;
                        } else
                            break;
                        // if (unlikely(seq_next == ssc->last_sent))
                        //     break;
                    }
                }
                while (burst_num < CLIENT_SIZE_BURST_TX) {
                    if (unlikely(ssc->last_sent == ssc->last_acked + ssc->window_size || ssc->last_sent == seq_max))
                        break;
                    seq_next           = ssc->last_sent + 1;
                    seq_index          = seq_next & ssc->window_mask;
                    bufs_tx[burst_num] = rte_pktmbuf_alloc(conn->mbuf_pool);
                    gen_packet(bufs_tx[burst_num++], conn, task->addr, task->op[seq_next], seq_next, reuse_map);
                    ssc->state[seq_index].seq = seq_next;
                    ssc->state[seq_index].ts  = ts_cur;
                    ssc->last_sent            = seq_next;
                }
                send_all_client(conn->port_id, conn->queue_id, bufs_tx, burst_num);
            }
            rte_ring_enqueue(task_done, (void *)task);
        }
        if (unlikely(force_quit == true))
            break;
    }

    RTE_LOG(INFO, APP, "task_daemon finish! \n");

    return 0;
}

static inline void
dataprocess(struct myh_hdr *h_myh, redisContext *conn) {
    int     key;
    uint8_t value[20];
    if (h_myh->op == 0x00) {
        rte_memcpy(&key, h_myh->key, sizeof(int));
        redisReply *reply = (redisReply *)redisCommand(conn, "GET %d", key);
        if (reply->type == REDIS_REPLY_STRING) {
            rte_memcpy(h_myh->value, reply->str, sizeof(h_myh->value));
        } else {
            Dprintf("error: %s\n", reply->str);
        }
        freeReplyObject(reply);
    } else if (h_myh->op == 0x01) {
        rte_memcpy(&key, h_myh->key, sizeof(int));
        rte_memcpy(value, h_myh->value, sizeof(h_myh->value));
        value[sizeof(h_myh->value)] = '\0';
        redisReply *reply           = (redisReply *)redisCommand(conn, "SET %d %s", key, value);
        if (reply->type == REDIS_REPLY_STATUS) {
            Dprintf("SET: %s\n", reply->str);
        } else {
            Dprintf("error: %s\n", reply->str);
        }
        freeReplyObject(reply);
    } else if (h_myh->op == 0x02) {
        rte_memcpy(&key, h_myh->key, sizeof(int));
        rte_memcpy(value, h_myh->value, sizeof(h_myh->value));
        value[sizeof(h_myh->value)] = '\0';
        redisReply *reply           = (redisReply *)redisCommand(conn, "SET %d %s", key, value);
        if (reply->type == REDIS_REPLY_STATUS) {
            Dprintf("SET: %s\n", reply->str);
        } else {
            Dprintf("error: %s\n", reply->str);
        }
        freeReplyObject(reply);
    }
}

// uint8_t *eseq = (uint8_t *)rte_zmalloc(NULL, KEY_IN_SWITCH * sizeof(uint8_t), 0);
uint8_t *eseq = NULL;

int lcore_server(void *arg) {
    struct conn_t *conn = (struct conn_t *)arg;
    if (rte_eth_dev_socket_id(conn->port_id) > 0 && rte_eth_dev_socket_id(conn->port_id) != (int)rte_socket_id())
        RTE_LOG(WARNING, APP, "Port %u is on remote NUMA node to polling thread. Performance will not be optimal.\n", conn->port_id);

    struct conf_t        *conf = get_conf();
    struct rte_mbuf      *bufs_tx[SERVER_SIZE_BURST_TX];
    struct rte_mbuf      *bufs_rx[SERVER_SIZE_BURST_RX];
    struct rte_ether_hdr *h_eth = NULL;
    struct rte_ipv4_hdr  *h_ip4 = NULL;
    struct myh_hdr       *h_myh = NULL;
    uint16_t              nb_rx, nb_tx, loop;

    uint32_t window_mask = (2 * conf->window - 1);
    uint32_t seq, seq_index, index, psn;

    uint8_t         *logseq   = (uint8_t *)rte_zmalloc(NULL, KEY_IN_SWITCH * sizeof(uint8_t) * 16 * conf->window, 0);
    uint8_t         *logpsn   = (uint8_t *)rte_zmalloc(NULL, sizeof(uint8_t) * 16 * conf->window * 2, 0);
    uint8_t         *progress = (uint8_t *)rte_zmalloc(NULL, 2 * conf->window * sizeof(uint8_t), 0);
    struct kv_pair_t kv_pair;

    redisContext *redis_conn = redisConnect("127.0.0.1", 6379);
    if (redis_conn->err)
        printf("connection error:%s\n", redis_conn->errstr);

    for (;;) {
        nb_tx = 0;
        nb_rx = rte_eth_rx_burst(conn->port_id, conn->queue_id, bufs_rx, SERVER_SIZE_BURST_RX);
        if (nb_rx > 0) {
            for (loop = 0; loop < nb_rx; loop++) {
                bufs_tx[nb_tx] = bufs_rx[loop];
                h_eth          = rte_pktmbuf_mtod(bufs_tx[nb_tx], struct rte_ether_hdr *);
                h_ip4          = rte_pktmbuf_mtod_offset(bufs_tx[nb_tx], struct rte_ipv4_hdr *, RTE_ETHER_HDR_LEN);
                h_myh          = rte_pktmbuf_mtod_offset(bufs_tx[nb_tx], struct myh_hdr *, LEN_UPTO_IP4);

                h_eth->dst_addr = h_eth->src_addr;
                h_eth->src_addr = conn->s_mac;
                h_ip4->dst_addr = h_ip4->src_addr;
                h_ip4->src_addr = conn->s_ip;
                h_myh->is_req   = 0x0;

                // h_ip4->total_length = htons(LEN_IP4_HDR + LEN_MYH_HDR);
                bufs_tx[nb_tx]->data_len = LEN_UPTO_MYH;
                bufs_tx[nb_tx]->pkt_len  = LEN_UPTO_MYH;

                h_ip4->total_length = htons(bufs_tx[nb_tx]->data_len - RTE_ETHER_HDR_LEN);

                // {
                //     seq_index = seq & window_mask;
                //     if (seq == ebpsn) {
                //         Dprintf("PROCESS %d ", seq);
                //         if (h_myh->op == 0x00) {
                //             Dprintf("READ");
                //             rte_memcpy(&key, h_myh->key, sizeof(key));
                //             rte_memcpy(&value, h_myh->value, sizeof(key));
                //             rte_memcpy(h_myh->value, kv_map + key * 16, sizeof(h_myh->value));
                //         } else if (h_myh->op == 0x01) {
                //             Dprintf("WRITE");
                //             rte_memcpy(&key, h_myh->key, sizeof(key));
                //             rte_memcpy(&value, h_myh->value, sizeof(key));
                //             rte_memcpy(kv_map + key * 16, h_myh->value, sizeof(h_myh->value));
                //         } else if (h_myh->op == 0x02) {
                //             Dprintf("END");
                //             force_quit = true;
                //             break;
                //         }
                //         // for (uint16_t loop2 = 0; loop2 < sizeof(h_myh->key); loop2++) {
                //         //     if (loop2 % 4 == 0)
                //         //         Dprintf(" ");
                //         //     Dprintf("%02x", h_myh->key[loop2]);
                //         // }
                //         Dprintf(" Key = %d , Value = %d", key, value);
                //         Dprintf("\n");
                //         rte_memcpy(bufs_state + 16 * seq_index, h_myh->value, sizeof(h_myh->value));
                //         ebpsn++;
                //     } else if (seq < ebpsn) {
                //         Dprintf("%d previous result is resend\n", seq);
                //         rte_memcpy(h_myh->value, bufs_state + 16 * seq_index, sizeof(h_myh->value));
                //     } else {
                //         Dprintf("Drop PSN Not Correct Packet %d\n", seq);
                //         nb_tx--;
                //     }
                // }

                if (h_myh->cached == 0x1 && (h_myh->op == 0x00 || h_myh->op == 0x01)) {
                    psn   = ntohl(h_myh->psn);
                    seq   = ntohl(h_myh->gpsn);
                    index = ntohl(h_myh->index);

                    Dprintf("PROCESS client %d psn %d seq %d index %d seq_changed %d\n", ntohl(conn->s_ip), psn, seq, index, h_myh->seq_changed);
                    Dprintf("process_index_mod %d process_pre_reuse_index_diff_mod %d process_pre_reuse_index_diff_mod %d\n", ntohl(h_myh->process_index_mod), ntohl(h_myh->process_pre_reuse_index_diff_mod), ntohl(h_myh->process_pre_w_index_diff_mod));
                    Dprintf("process_index_add_a %d process_pre_reuse_index %d process_pre_w_index %d process_index %d\n", ntohl(h_myh->process_index_add_a), ntohl(h_myh->process_pre_reuse_index), ntohl(h_myh->process_pre_w_index), ntohl(h_myh->process_index));
                    Dprintf("process_pre_reuse_valid %d process_valid %d\n", h_myh->process_pre_reuse_valid, h_myh->process_valid);

                    {
                        if (h_myh->op == 0x00) {
                            Dprintf("[Recv] Read Data (seq = %d, Key =", seq);
                        } else {
                            Dprintf("[Recv] Write Data (seq = %d, Key = ", seq);
                        }
                        rte_memcpy(&kv_pair.key, h_myh->key, sizeof(uint8_t) * 16);
                        for (int i = 0; i < 16; i++) {
                            Dprintf("%02x", kv_pair.key[i]);
                        }
                        Dprintf(" , Value = ");
                        rte_memcpy(&kv_pair.value, h_myh->value, sizeof(uint8_t) * 16);
                        for (int i = 0; i < 16; i++) {
                            Dprintf("%02x", kv_pair.value[i]);
                        }
                        Dprintf(" )\n");
                    }
                    assert(index < KEY_IN_SWITCH);
                    int seq_value = eseq[index];
                    if (seq < seq_value) {
                        Dprintf("%d previous result is resend\n", seq);
                        rte_memcpy(h_myh->value, logseq + 16 * (index * conf->window + seq), sizeof(h_myh->value));
                    } else if (seq == seq_value) {
                        dataprocess(h_myh, redis_conn);
                        eseq[index]++;
                        rte_memcpy(logseq + 16 * (index * conf->window + seq), h_myh->value, sizeof(h_myh->value));
                    } else {
                        Dprintf("Drop PSN Not Correct Packet %d\n", seq);
                        nb_tx--;
                    }
                } else {
                    seq = ntohl(h_myh->psn);
                    Dprintf("PROCESS client %d psn %d seq_changed %d\n", ntohl(conn->s_ip), seq, h_myh->seq_changed);
                    Dprintf("process_index_mod %d process_pre_reuse_index_diff_mod %d process_pre_reuse_index_diff_mod %d\n", ntohl(h_myh->process_index_mod), ntohl(h_myh->process_pre_reuse_index_diff_mod), ntohl(h_myh->process_pre_w_index_diff_mod));
                    Dprintf("process_index_add_a %d process_pre_reuse_index %d process_pre_w_index %d process_index %d\n", ntohl(h_myh->process_index_add_a), ntohl(h_myh->process_pre_reuse_index), ntohl(h_myh->process_pre_w_index), ntohl(h_myh->process_index));
                    Dprintf("process_pre_reuse_valid %d process_valid %d\n", h_myh->process_pre_reuse_valid, h_myh->process_valid);

                    int progress_index           = seq & window_mask;
                    int progress_pre_reuse_index = (seq - h_myh->reuse_distance) & window_mask;
                    int progress_pre_w_index     = (seq - conf->window) & window_mask;
                    if (progress[progress_index] == 0) {
                        if (h_myh->reuse_distance != 0 && progress[progress_pre_reuse_index] == 0) {
                            Dprintf("Drop PSN Not Correct Packet %d\n", seq);
                            nb_tx--;
                        } else {
                            dataprocess(h_myh, redis_conn);
                            rte_memcpy(logpsn + 16 * progress_index, h_myh->value, sizeof(h_myh->value));
                            progress[progress_index]       = 1;
                            progress[progress_pre_w_index] = 0;
                        }
                    } else {
                        Dprintf("%d previous result is resend\n", seq);
                        rte_memcpy(h_myh->value, logpsn + 16 * progress_index, sizeof(h_myh->value));
                    }
                }

                nb_tx++;
            }
            send_all_server(conn->port_id, conn->queue_id, bufs_tx, nb_tx);
        }
        if (unlikely(force_quit == true))
            break;
    }

    redisFree(redis_conn);

    return 0;
}