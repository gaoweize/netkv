#include <arpa/inet.h>
#include <getopt.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <rte_atomic.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_flow.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>

#include "common.h"
#include "conf.h"
#include "env.h"

static void
assert_link_status(uint16_t tx_port) {
    RTE_LOG(INFO, APP, "Asserting link status for Port %hu ...\n", tx_port);
    struct rte_eth_link link;
    uint8_t             rep_cnt = 9; /* 9s (9 * 1000ms) in total */
    memset(&link, 0, sizeof(link));
    do {
        rte_eth_link_get_nowait(tx_port, &link);
        if (link.link_status == RTE_ETH_LINK_UP)
            break;
        rte_delay_ms(1000);
    } while (--rep_cnt);
    if (link.link_status == RTE_ETH_LINK_DOWN)
        rte_exit(EXIT_FAILURE, ":: error: link is still down\n");
}

static int
port_init(void) {
    struct conf_t     *conf     = get_conf();
    struct conn_arr_t *conn_arr = get_connarr();

    uint16_t port = conf->port_id;
    if (!rte_eth_dev_is_valid_port(port))
        rte_exit(EXIT_FAILURE, "Port %hu is invalid\n", port);

    struct rte_eth_dev_info dev_info;
    rte_eth_dev_info_get(port, &dev_info);

    struct rte_eth_conf port_conf = {
        // .rxmode = { .max_rx_pkt_len = RTE_ETHER_MAX_LEN, },
        .txmode = {
            .mq_mode = RTE_ETH_MQ_TX_NONE,
        },
    };
    if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
        port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;

    int retval = rte_eth_dev_configure(port, conn_arr->num_lcore, conn_arr->num_lcore, &port_conf);
    if (retval < 0)
        rte_exit(EXIT_FAILURE, "Cannot init port %hu\n", port);

    struct rte_eth_txconf txq_conf = dev_info.default_txconf;
    struct rte_eth_rxconf rxq_conf = dev_info.default_rxconf;
    txq_conf.offloads              = port_conf.txmode.offloads;
    rxq_conf.offloads              = port_conf.rxmode.offloads;

    for (int loop = 0; loop < conn_arr->num_lcore; loop++) {
        uint16_t q_id = conn_arr->conn[loop].queue_id;
        retval        = rte_eth_rx_queue_setup(port, q_id, SIZE_RING / 4, rte_eth_dev_socket_id(port), &rxq_conf, conn_arr->conn[loop].mbuf_pool);
        if (retval < 0)
            rte_exit(EXIT_FAILURE, "set up rx %hu failed\n", q_id);
        retval = rte_eth_tx_queue_setup(port, q_id, SIZE_RING, rte_eth_dev_socket_id(port), &txq_conf);
        if (retval < 0)
            rte_exit(EXIT_FAILURE, "set up tx %hu failed\n", q_id);
    }
    rte_eth_promiscuous_enable(port);
    retval = rte_eth_dev_start(port);
    if (retval < 0)
        rte_exit(EXIT_FAILURE, "Cannot start port %hu\n", port);

    struct rte_ether_addr addr;
    rte_eth_macaddr_get(port, &addr);
    RTE_LOG(INFO, APP, "Using Port %u, MAC = %02x:%02x:%02x:%02x:%02x:%02x\n",
            port, addr.addr_bytes[0], addr.addr_bytes[1], addr.addr_bytes[2],
            addr.addr_bytes[3], addr.addr_bytes[4], addr.addr_bytes[5]);

    assert_link_status(port);
    return 0;
}

#define MAX_PATTERN_NUM 4
static struct rte_flow *
geenrate_rule(uint16_t tx_port, uint16_t rx_q, struct rte_flow_error *error) {
    struct rte_flow_attr         attr;
    struct rte_flow_item         pattern[MAX_PATTERN_NUM];
    struct rte_flow_action       action[MAX_PATTERN_NUM];
    struct rte_flow             *flow  = NULL;
    struct rte_flow_action_queue queue = {.index = rx_q};
    struct rte_flow_item_eth     eth_spec;
    struct rte_flow_item_eth     eth_mask;
    struct rte_flow_item_ipv4    ip_spec;
    struct rte_flow_item_ipv4    ip_mask;

    memset(pattern, 0, sizeof(pattern));
    memset(action, 0, sizeof(action));
    memset(&attr, 0, sizeof(struct rte_flow_attr));
    attr.ingress = 1;

    action[0].type = RTE_FLOW_ACTION_TYPE_QUEUE;
    action[0].conf = &queue;
    action[1].type = RTE_FLOW_ACTION_TYPE_END;

    memset(&eth_spec, 0, sizeof(struct rte_flow_item_eth));
    memset(&eth_mask, 0, sizeof(struct rte_flow_item_eth));
    pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
    pattern[0].spec = &eth_spec;
    pattern[0].mask = &eth_mask;

    memset(&ip_spec, 0, sizeof(struct rte_flow_item_ipv4));
    memset(&ip_mask, 0, sizeof(struct rte_flow_item_ipv4));
    ip_spec.hdr.dst_addr = htonl((uint32_t)rx_q);
    ip_mask.hdr.dst_addr = htonl(0x000000ff);
    pattern[1].type      = RTE_FLOW_ITEM_TYPE_IPV4;
    pattern[1].spec      = &ip_spec;
    pattern[1].mask      = &ip_mask;

    pattern[2].type = RTE_FLOW_ITEM_TYPE_END;

    int res = rte_flow_validate(tx_port, &attr, pattern, action, error);
    if (!res)
        flow = rte_flow_create(tx_port, &attr, pattern, action, error);
    return flow;
}

static void
flow_init(void) {
    struct conn_arr_t *conn_arr = get_connarr();

    struct rte_flow_error error;
    struct rte_flow      *flow;
    for (int loop = 0; loop < conn_arr->num_lcore; loop++) {
        flow = geenrate_rule(conn_arr->conn[loop].port_id, conn_arr->conn[loop].queue_id, &error);
        if (!flow)
            rte_exit(EXIT_FAILURE, "Flow can't be created %d message: %s\n", error.type, error.message ? error.message : "(no stated reason)");
    }
}

void env_init(void) {
    port_init();
    flow_init();
}

void env_exit(void) {
    struct conf_t     *conf     = get_conf();
    struct conn_arr_t *conn_arr = get_connarr();

    RTE_LOG(INFO, APP, "Closing and releasing resources ...\n");
    for (int loop = 0; loop < conn_arr->num_lcore; loop++) {
        struct conn_t conn = conn_arr->conn[loop];
        if (conn.mbuf_pool != NULL)
            rte_mempool_free(conn.mbuf_pool);
    }
    struct rte_flow_error error;
    printf("here!\n");

    rte_flow_flush(conf->port_id, &error);
    rte_eth_dev_stop(conf->port_id);
    rte_eth_dev_close(conf->port_id);
}