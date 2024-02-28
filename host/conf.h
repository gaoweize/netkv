#ifndef _CONF_H_
#define _CONF_H_

#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <stdbool.h>
#include <stdint.h>

#define MODE_CLIENT 1
#define MODE_SERVER 2
struct conf_t {
    int      mode;
    int      worker_id;
    int      num_thread;
    uint16_t port_id;
    char     client_addr[20];
    char     server_addr[20];

    int    window;
    double loss_rate;
    char   trace_file[100];
    char   result_file[100];
    int    is_load;
};
struct conf_t *get_conf(void);
void           print_conf(void);

#define MAX_THREAD 16
struct conn_t {
    int      ID;
    int      num_thread;
    uint16_t port_id; /* Port ID (=default_port) */
    uint16_t queue_id;
    unsigned lcore_id; /* lcore ID for RX and TX */
    /* filtering rule */
    uint32_t              s_ip;
    uint32_t              d_ip;
    struct rte_ether_addr s_mac;
    struct rte_ether_addr d_mac;
    struct rte_mempool   *mbuf_pool;
};
struct conn_arr_t {
    int num_lcore;
    int num_thread;
    /* master + slaves */
    struct conn_t conn[MAX_THREAD + 1];
};
struct conn_arr_t *get_connarr(void);
void               print_conn(void);

void opt_parser(int argc, char *argv[]);
void para_init(void);

#endif