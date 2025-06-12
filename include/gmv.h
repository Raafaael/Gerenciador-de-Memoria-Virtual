#ifndef GMV_H
#define GMV_H
#include "pager.h"

// Parâmetros fixos
#define N_PROCS 4
#define VPAGES 32
#define N_FRAMES 16

typedef enum { R_OP, W_OP } AccessType;

typedef struct {
    int page_idx;
    AccessType op;
    int pid;
} Access;

typedef struct {
    unsigned present:1, modified:1, referenced:1;
    unsigned frame;
    unsigned age;
    unsigned last_ref;
} PTE;

typedef struct {
    int owner_pid;
    int vpage;
    PTE *pte_ptr;
} Frame;

typedef struct {
    int page_faults;
    int dirty_writes;
} Stats;

extern PTE page_table[N_PROCS][VPAGES];
extern Frame frames[N_FRAMES];
extern Stats stats;
extern int extra_slots[N_PROCS];
extern int total_rounds;
extern PagerType current_pager;

// Interface
void init_gmv(PagerType pager, int rounds);
void run_gmv(void);
void dump_stats(void);
void dump_page_tables(void);

#endif
