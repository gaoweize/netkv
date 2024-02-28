#ifndef _INAP_H_
#define _INAP_H_

#define CLIENT_SIZE_BURST_RX 32 // The maximum number of packets to retrieve. The value must be divisible by 8 in order to work with any driver.
#define CLIENT_SIZE_BURST_TX 32
#define SERVER_SIZE_BURST_RX 32
#define SERVER_SIZE_BURST_TX 32

#define LEN_IP4_HDR  sizeof(struct rte_ipv4_hdr)
#define LEN_MYH_HDR  sizeof(struct myh_hdr)
#define LEN_UPTO_IP4 (RTE_ETHER_HDR_LEN + LEN_IP4_HDR)
#define LEN_UPTO_MYH (RTE_ETHER_HDR_LEN + LEN_IP4_HDR + LEN_MYH_HDR)

#define PROTO_MYH 0x54

#define MAX_WND  0x100000
#define WND_MASK (2 * MAX_WND - 1)

#define WND_1      0x1
#define WND_MASK_1 (2 * WND_1 - 1)

struct kv_pair_t {
    uint8_t key[16];
    uint8_t value[16];
};

#define KEY_IN_SWITCH 10000
#define NUM_KEY       100000
#define NUM_REQ       1

struct seq_state_c_t {
    struct {
        uint64_t ts;
        uint32_t seq;
    } state[2 * MAX_WND];
    uint32_t last_sent;
    uint32_t last_acked;
    uint32_t window_size;
    uint32_t window_mask;
};

struct myh_hdr {
    uint32_t psn;
    uint32_t gpsn;
    uint32_t index;
    uint32_t reuse_distance;
    uint8_t  reserved : 4;
    uint8_t  cached : 1;
    uint8_t  op : 2;
    uint8_t  is_req : 1;
    uint8_t  key[16];
    uint8_t  value[16];

#ifdef DEBUG
    uint32_t process_index_mod;
    uint32_t process_pre_reuse_index_diff_mod;
    uint32_t process_pre_w_index_diff_mod;

    uint32_t process_index_add_a;
    uint32_t process_pre_reuse_index;
    uint32_t process_pre_w_index;
    uint32_t process_index;

    uint8_t process_pre_reuse_valid;
    uint8_t process_valid;
    uint8_t seq_changed;
#endif 
} __attribute__((__packed__));

struct task_t {
    int      thread_id;
    char    *addr;
    uint8_t *op;
};

int  lcore_client(void *arg);
int  lcore_server(void *arg);
void inap_init(void);
void inap_terminate(void);
int  push_task(struct task_t new_task);
int  check_task(void);

#endif