#include "pager.h"
#include "gmv.h"

static int hand = 0;
int find_victim_2nd(void) {
    while (1) {
        PTE *p = frames[hand].pte_ptr;
        if (!p->referenced)
            return hand;
        p->referenced = 0;
        hand = (hand + 1) % N_FRAMES;
    }
}
