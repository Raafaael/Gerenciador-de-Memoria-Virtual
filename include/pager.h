#ifndef PAGER_H
#define PAGER_H

typedef enum { PAGER_NRU, PAGER_2ND, PAGER_LRU, PAGER_WS3, PAGER_WS5 } PagerType;

/* o GMV definirá estas variáveis */
extern PagerType current_pager;   /* selecionado pelo usuário */
extern unsigned  ws_k;      /* k para working-set       */

int find_victim_nru(void);
int find_victim_2nd(void);
int find_victim_lru(void);
int find_victim_ws(void);

#endif
