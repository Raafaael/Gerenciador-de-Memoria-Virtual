#include "pager.h"
#include "gmv.h"

int find_victim_ws(void) {
    int pid = frames[0].owner_pid;
    unsigned cur = stats.page_faults;
    for (int f = 0; f < N_FRAMES; ++f)
        if (frames[f].owner_pid == pid) {
            PTE *p = frames[f].pte_ptr;
            if (cur - p->last_ref > 3)
                return f;
        }
    return find_victim_nru();
}
