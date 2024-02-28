#ifndef _COMMON_H_
#define _COMMON_H_

#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <stdbool.h>
#include <stdint.h>

#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1 /* Macros for printing using RTE_LOG */

// #define max(x, y) (((x) > (y)) ? (x) : (y))
// #define min(x, y) (((x) < (y)) ? (x) : (y))

void   print_mraw(struct rte_mbuf *buf);
void   set_start_time(void);
void   set_end_time(void);
double get_elapsed(void); // in seconds
void   print_stats(void);

#endif
