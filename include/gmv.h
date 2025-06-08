#ifndef GMV_H
#define GMV_H

#include "pager.h"

/* Parâmetros fixos ------------------------------------------------------- */
#define N_PROCS    4
#define VPAGES    32
#define N_FRAMES  16
#define SLOT_USEC 1000            /* 1 UT ≈ 1 segundo (sleep(1))  */
#define EXTRA_PF_SLOTS 2          /* bônus pós-page-fault             */

/* Estruturas ------------------------------------------------------------- */
typedef enum { R_OP, W_OP } AccessType;

typedef struct {
    int        page_idx;   /* 0-31 */
    AccessType op;         /* R_OP ou W_OP */
    int        pid;        /* quem gerou (via getpid()) */
} Access;

/* Entrada de TP */
typedef struct {
    unsigned present:1, modified:1, referenced:1;
    unsigned frame;               /* se present==1 */
    unsigned age;                 /* Aging (LRU)   */
    unsigned last_ref;            /* Working-set   */
} PTE;

/* Quadro físico */
typedef struct {
    int owner_pid;
    int vpage;
    PTE  *pte_ptr;
} Frame;

/* Estatísticas globais */
typedef struct {
    int page_faults;
    int dirty_writes;
} Stats;

/* globais vistos por todos os pagers ------------------------------------ */
extern PTE    page_table[N_PROCS][VPAGES];  /* Tabela de páginas */
extern Frame  frames[N_FRAMES];             /* Quadro de memória */
extern Stats  stats;                        /* Estatísticas */
extern int    extra_slots[N_PROCS];         /* Extra slots pós-página falha */

/* API principal ---------------------------------------------------------- */
void init_gmv(int pager, int rounds);  /* Corrigido para PagerType */
void run_gmv(void);
void dump_stats(void);
void dump_page_tables(void);

#endif
