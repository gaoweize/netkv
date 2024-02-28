#include <arpa/inet.h>
#include <net/if.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>

#include "common.h"
#include "conf.h"
#include "demo.h"
#include "inap.h"

/*----------------------------------------------------------------------------------------------*/
static struct timeval begin_t = {0};
static struct timeval end_t   = {0};

void set_start_time(void) {
    gettimeofday(&begin_t, NULL);
}
void set_end_time(void) {
    gettimeofday(&end_t, NULL);
}
double
get_elapsed(void) {
    gettimeofday(&end_t, NULL);
    double delta_time = end_t.tv_sec - begin_t.tv_sec + (end_t.tv_usec - begin_t.tv_usec) / 1000000.0;
    return delta_time;
}

/*----------------------------------------------------------------------------------------------*/
void print_mraw(struct rte_mbuf *buf) {
    RTE_LOG(INFO, APP, "Showing Packet Content (%u Bytes)...", buf->pkt_len);
    uint8_t *data = rte_pktmbuf_mtod(buf, uint8_t *);
    for (uint16_t loop = 0; loop < buf->data_len; loop++) {
        if (loop % 16 == 0)
            printf("\n%03x: ", loop);
        else if (loop % 4 == 0)
            printf(" ");
        printf("%02x ", data[loop]);
    }
    printf("\n");
}

/*----------------------------------------------------------------------------------------------*/
void print_stats(void) {
    struct rte_eth_stats port_stats;
    struct conf_t       *conf = get_conf();
    rte_eth_stats_get(conf->port_id, &port_stats);

    double delta_time = end_t.tv_sec - begin_t.tv_sec + (end_t.tv_usec - begin_t.tv_usec) / 1000000.0;
    printf("%ld %ld\n", end_t.tv_sec, begin_t.tv_sec);
    if (delta_time == 0) {
        delta_time = 1;
    }

    RTE_LOG(INFO, APP, "[Stat] Consume %.2f seconds, QPS=%02.2fMpps, BPS=%02.2fGbps\n",
            delta_time,
            NUM_REQ / (1000000 * delta_time),
            (NUM_REQ * (LEN_UPTO_MYH + 24)) / (125000000 * delta_time));

    RTE_LOG(INFO, APP, "[Stat] Received %10lu packets in %.2f seconds, PPS=%02.2fMpps, BPS=%02.2fGbps\n",
            port_stats.ipackets, delta_time,
            port_stats.ipackets / (1000000 * delta_time),
            (port_stats.ibytes + 24 * port_stats.ipackets) / (125000000 * delta_time));
    RTE_LOG(INFO, APP, "[Stat] Sent     %10lu packets in %.2f seconds, PPS=%02.2fMpps, BPS=%02.2fGbps\n",
            port_stats.opackets, delta_time,
            port_stats.opackets / (1000000 * delta_time),
            (port_stats.obytes + 24 * port_stats.opackets) / (125000000 * delta_time));
    RTE_LOG(INFO, APP, "[Stat] imissed = %lu, ierrors = %lu, oerrors = %lu, rx_nombuf = %lu\n",
            port_stats.imissed,
            port_stats.ierrors,
            port_stats.oerrors,
            port_stats.rx_nombuf);
}
