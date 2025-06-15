#ifndef ALGORITHMS_H
#define ALGORITHMS_H

typedef enum { PAGER_NRU, PAGER_2ND, PAGER_LRU, PAGER_WS } PagerType;

int find_victim_nru(void);
int find_victim_2nd(void);
int find_victim_lru(void);
int find_victim_ws(void);

#endif
