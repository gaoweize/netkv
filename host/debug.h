#ifndef _DEBUG_H_
#define _DEBUG_H_

#define DEBUG

#define CO_RED   "\033[1;31m"
#define CO_RESET "\033[0m"

#ifdef DEBUG
#define Dprintf(msg, ...) printf(msg, ##__VA_ARGS__)
#else
#define Dprintf(...)
#endif

#endif