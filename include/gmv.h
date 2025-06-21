#ifndef GMV_H
#define GMV_H
#include "algorithms.h"

// Parâmetros fixos
#define N_PROCS 4
#define VPAGES 32
#define N_FRAMES 16

typedef enum { R_OP, W_OP } AccessType;

typedef struct {
    int page_id;
    AccessType op;
    int pid;
} Access;

typedef struct {
    unsigned present:1, modified:1, referenced:1;
    unsigned frame;
    unsigned age;
    unsigned last_ref;
} TableEntry;

typedef struct {
    int owner_pid;
    int vpage;
    TableEntry *TableEntry_ptr;
} Frame;

typedef struct {
    int page_faults;
    int dirty_writes;
} Stats;

extern TableEntry page_table[N_PROCS][VPAGES];
extern Frame frames[N_FRAMES];
extern Stats stats;
extern int total_rounds;
extern Algorithm current_algorithm;

// Interface
void init_gmv(Algorithm algorithm, int rounds);
void run_gmv(void);
void dump_stats(void);
void dump_page_tables(void);

#endif
