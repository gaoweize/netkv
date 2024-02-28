#include "conf.h"
#include "common.h"
#include "env.h"

#include <rte_atomic.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_flow.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>

#include <arpa/inet.h>
#include <getopt.h>
#include <net/if.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

/*----------------------------------------------------------------------------------------------*/
struct conf_t conf = {0};

struct conf_t *
get_conf(void) {
    return &conf;
}
void print_conf(void) {
    RTE_LOG(INFO, APP, "Showing basic configuration ...\n");
    RTE_LOG(INFO, APP, "  mode           = %d\n", conf.mode);
    RTE_LOG(INFO, APP, "  worker id      = %d\n", conf.worker_id);
    RTE_LOG(INFO, APP, "  port id        = %hu\n", conf.port_id);
    RTE_LOG(INFO, APP, "  thread number  = %d\n", conf.num_thread);
    RTE_LOG(INFO, APP, "  client address = %s\n", conf.client_addr);
    RTE_LOG(INFO, APP, "  server address = %s\n", conf.server_addr);
}

/*----------------------------------------------------------------------------------------------*/
struct conn_arr_t conn_arr = {0};

struct conn_arr_t *
get_connarr(void) {
    return &conn_arr;
}
void print_conn(void) {
    RTE_LOG(INFO, APP, "Showing connection configuration (ID, Total Thread, Port, Queue, Lcore, mbuf_pool...)\n");
    for (int loop = 0; loop < conn_arr.num_lcore; loop++) {
        struct conn_t conn = conn_arr.conn[loop];
        RTE_LOG(INFO, APP, "  [%02d, %02d, %hu, %02hu (rule=0x%08x), %u, %s, 0x%08x <=> 0x%08x ]\n",
                conn.ID, conn.num_thread, conn.port_id, conn.queue_id, ntohl(conn.d_ip), conn.lcore_id, conn.mbuf_pool->name, ntohl(conn.s_ip), ntohl(conn.d_ip));
    }
}

/*----------------------------------------------------------------------------------------------*/
/*
 * worker1  192.168.0.1  b8:59:9f:1d:04:f2
 * worker2  192.168.0.2  b8:59:9f:0b:30:72
 * worker3  192.168.0.3  98:03:9b:03:46:50
 * worker4  192.168.0.4  b8:59:9f:02:0d:14
 * worker5  192.168.0.5  b8:59:9f:b0:2d:50
 * worker6  192.168.0.6  b8:59:9f:b0:2b:b0
 * worker7  192.168.0.7  b8:59:9f:b0:2b:b8
 * worker8  192.168.0.8  b8:59:9f:b0:2d:18
 * worker9  192.168.0.9  b8:59:9f:b0:2d:58
 * worker21 192.168.1.21 0c:42:a1:7a:b6:69
 * worker22 192.168.1.22 0c:42:a1:7a:ca:29
 */
#define TOTAL_WORKER 11
static const char *bind_NIC = "ens10f1np1";
static const char *MACs[]   = {
    "10:70:fd:19:00:95",
    "10:70:fd:2f:d8:51",
    "10:70:fd:2f:e4:41",
    "10:70:fd:2f:d4:21",
};
static const char *IPs[] = {
    "192.168.1.1",
    "192.168.1.2",
    "192.168.1.3",
    "192.168.1.4",
};
struct rte_ether_addr ether_arr[] = {
    {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
    {{0x10, 0x70, 0xfd, 0x19, 0x00, 0x95}},
    {{0x10, 0x70, 0xfd, 0x2f, 0xd8, 0x51}},
    {{0x10, 0x70, 0xfd, 0x2f, 0xe4, 0x41}},
    {{0x10, 0x70, 0xfd, 0x2f, 0xd4, 0x21}},
};
/* return a index from MACs */
static inline int
parse_nic(const char *NIC) {
    struct ifreq ifreq;
    int          sock;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(0);
    }
    strcpy(ifreq.ifr_name, NIC);
    if (ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0) {
        perror("ioctl");
        exit(0);
    }
    char MAC[20] = {0};
    sprintf(MAC, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned char)ifreq.ifr_hwaddr.sa_data[0],
            (unsigned char)ifreq.ifr_hwaddr.sa_data[1],
            (unsigned char)ifreq.ifr_hwaddr.sa_data[2],
            (unsigned char)ifreq.ifr_hwaddr.sa_data[3],
            (unsigned char)ifreq.ifr_hwaddr.sa_data[4],
            (unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
    for (int loop = 0; loop < TOTAL_WORKER; loop++) {
        if (strcmp(MAC, MACs[loop]) == 0) {
            return loop;
        }
    }
    return -1;
}

inline struct rte_ether_addr
get_macaddr(uint32_t host) {
    return ether_arr[ntohl(host) & 0xff];
}
static inline const char *
get_host(void) {
    int index = parse_nic(bind_NIC);
    return IPs[index];
}
static inline int
get_port(void) {
    return 0;
}
static inline int
get_id(void) {
    int index = parse_nic(bind_NIC);
    return index + 1;
}

/*-------------------------------------------- OPT --------------------------------------------------*/

static void
show_usage(const char *prgname) {
    RTE_LOG(INFO, APP, "\nUsage: %s --client=ip --server=server_ip --thread=num_thread --window=window_size --trace=trace_file --loss=loss_rate --load\n", prgname);
}

int           lopt = 0;
extern double loss_rate;

void opt_parser(int argc, char *argv[]) {
    if (argc <= 1)
        show_usage(argv[0]);

    struct conf_t *conf = get_conf();
    conf->is_load       = 0;

    static struct option opts[] = {
        {"help", no_argument, NULL, 'h'},
        {"client", required_argument, &lopt, 1},
        {"server", required_argument, &lopt, 2},
        {"thread", required_argument, &lopt, 3},
        {"window", required_argument, &lopt, 4},
        {"trace", required_argument, &lopt, 5},
        {"loss", required_argument, &lopt, 6},
        {"load", no_argument, &lopt, 7},
        {NULL, 0, NULL, 0}};
    int c, opt_index = 0;
    while ((c = getopt_long(argc, argv, "", opts, &opt_index)) != -1) {
        switch (c) {
        case 0:
            switch (lopt) {
            case 1:
                strcpy(conf->client_addr, optarg);
                break;
            case 2:
                strcpy(conf->server_addr, optarg);
                break;
            case 3:
                conf->num_thread = atoi(optarg);
                if (conf->num_thread > MAX_THREAD) {
                    conf->num_thread = MAX_THREAD;
                    RTE_LOG(INFO, APP, "Only %d threads are supported!\n", MAX_THREAD);
                }
                break;
            case 4:
                conf->window = atoi(optarg);
                break;
            case 5:
                strcpy(conf->trace_file, optarg);
                break;
            case 6:
                conf->loss_rate = strtod(optarg, NULL);
                break;
            case 7:
                conf->is_load = 1;
                break;
            default:
                show_usage(argv[0]);
            }
            break;
        default:
            show_usage(argv[0]);
        }
    }

    if (strcmp(conf->server_addr, get_host()) == 0) {
        conf->mode = MODE_SERVER;
    } else {
        conf->mode = MODE_CLIENT;
    }
    conf->port_id   = get_port();
    conf->worker_id = get_id();
}

void para_init(void) {
    struct conf_t     *conf     = get_conf();
    struct conn_arr_t *conn_arr = get_connarr();
    memset(conn_arr, 0, sizeof(struct conn_arr_t));

    unsigned lcore_arr[MAX_THREAD + 1] = {0};
    unsigned lcore_id;
    uint16_t index = 0;
    RTE_LCORE_FOREACH(lcore_id) { lcore_arr[index++] = lcore_id; }

    uint16_t queue_next = 0;
    char     mbuf_pool_name[20];

    struct in_addr i_addr;
    struct in_addr d_addr;
    inet_aton(conf->client_addr, &i_addr);
    inet_aton(conf->server_addr, &d_addr);

    conn_arr->num_thread = conf->num_thread;
    conn_arr->num_lcore  = conf->num_thread + 1;
    for (int loop = 0; loop < conn_arr->num_lcore; loop++) {
        conn_arr->conn[loop].ID         = loop - 1;
        conn_arr->conn[loop].port_id    = conf->port_id;
        conn_arr->conn[loop].queue_id   = queue_next++;
        conn_arr->conn[loop].lcore_id   = lcore_arr[loop];
        conn_arr->conn[loop].s_mac      = get_macaddr(i_addr.s_addr);
        conn_arr->conn[loop].d_mac      = get_macaddr(d_addr.s_addr);
        conn_arr->conn[loop].s_ip       = htonl((uint32_t)conn_arr->conn[loop].queue_id);
        conn_arr->conn[loop].d_ip       = htonl((uint32_t)conn_arr->conn[loop].queue_id);
        conn_arr->conn[loop].num_thread = conn_arr->num_thread;
        sprintf(mbuf_pool_name, "MP_%02hu", conn_arr->conn[loop].queue_id);
        conn_arr->conn[loop].mbuf_pool = rte_pktmbuf_pool_create(mbuf_pool_name, SIZE_N, SIZE_MBUF_CACHE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    }
}
