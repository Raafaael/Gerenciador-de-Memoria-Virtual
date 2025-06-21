#ifndef ALGORITHMS_H
#define ALGORITHMS_H

extern unsigned long long tick;

typedef enum {ALGORITHM_NRU, ALGORITHM_2ND, ALGORITHM_LRU, ALGORITHM_WS} Algorithm;

int find_victim_nru(void);
int find_victim_2nd(void);
int find_victim_lru(int pid);
int find_victim_ws(int pid);

#endif
