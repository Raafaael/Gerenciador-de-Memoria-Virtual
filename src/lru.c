#include "pager.h"
#include "gmv.h"

int find_victim_lru(void) {
    int pid = frames[0].owner_pid;  // O GMV garante que frames[] já está preenchido
    int best = -1;
    unsigned max_age = 0;

    for (int f = 0; f < 16; ++f) {
        if (frames[f].owner_pid == pid) {
            if (frames[f].pte_ptr->age > max_age) {
                max_age = frames[f].pte_ptr->age;
                best = f;
            }
        }
    }
    return best;
}
