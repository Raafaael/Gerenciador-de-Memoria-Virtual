#include "pager.h"
#include "gmv.h"

int find_victim_nru(void) {
    int best = -1, best_class = 4;
    for (int f = 0; f < N_FRAMES; ++f) {
        PTE *p = frames[f].pte_ptr;
        int cls = (!p->referenced << 1) | p->modified;
        if (cls < best_class) {
            best_class = cls;
            best = f;
        }
        if (best_class == 0) break;
    }
    return best;
}
