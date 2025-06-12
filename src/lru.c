#include "pager.h"
#include "gmv.h"

int find_victim_lru(void) {
    int pid = frames[0].owner_pid;
    int best = -1;
    unsigned max_age = 0;
    for (int f = 0; f < N_FRAMES; ++f)
        if (frames[f].owner_pid == pid) {
            if (frames[f].pte_ptr->age > max_age) {
                max_age = frames[f].pte_ptr->age;
                best = f;
            }
        }
    return best;
}
