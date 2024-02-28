#ifndef _ENV_H_
#define _ENV_H_

#define SIZE_RING        1024
#define SIZE_N           4095 // The number of elements in the mbuf pool. The optimum size (in terms of memory usage) for a mempool is when n is a power of two minus one: n = (2^q - 1)
#define SIZE_MBUF_CACHE  128
// #define SIZE_RING        2048
// #define SIZE_N           8191 // The number of elements in the mbuf pool. The optimum size (in terms of memory usage) for a mempool is when n is a power of two minus one: n = (2^q - 1)
// #define SIZE_MBUF_CACHE  512

void env_init(void);
void env_exit(void);

#endif