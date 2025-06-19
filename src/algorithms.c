#include <limits.h>
#include "algorithms.h"
#include "gmv.h"
#include "utils.h"

// NRU: Not Recently Used
int find_victim_nru(void) {
    int best = -1, best_class = 4;
    for (int f = 0; f < N_FRAMES; ++f) {
        PTE *p = frames[f].pte_ptr;
        if (!p) continue;
        int cls = (!p->referenced << 1) | p->modified;
        if (cls < best_class) {
            best_class = cls;
            best = f;
        }
        if (best_class == 0) break;
    }
    if (best == -1) die("Erro NRU: Nenhum quadro elegível para substituição!");
    return best;
}

// Segunda Chance (Second Chance)
static int hand = 0;
int find_victim_2nd(void) {
    int tentativas = 0;
    while (tentativas < N_FRAMES * 2) { // Limita número de voltas
        PTE *p = frames[hand].pte_ptr;
        if (p && !p->referenced)
            return hand;
        if (p) p->referenced = 0;
        hand = (hand + 1) % N_FRAMES;
        tentativas++;
    }
    die("Erro 2ND: Nenhum quadro elegível para substituição!");
    return -1; // nunca chega aqui
}

// LRU: Least Recently Used (usando Aging)
int find_victim_lru(int pid) {
    int oldest = -1;
    unsigned int oldest_age = UINT_MAX;
    for (int f = 0; f < N_FRAMES; ++f) {
        PTE *p = frames[f].pte_ptr;
        if (!p || !p->present) continue;
        if (frames[f].owner_pid != pid) continue;
        if (p->age < oldest_age) {
            oldest_age = p->age;
            oldest = f;
        }
    }
    if (oldest == -1) die("Erro LRU: Nenhum quadro elegível para substituição para pid %d!", pid);
    return oldest;
}

extern int ws_k; // Variável global para o parâmetro do WS
// Working Set
int find_victim_ws(int pid) {
    unsigned cur = stats.page_faults;
    for (int f = 0; f < N_FRAMES; ++f) {
        if (frames[f].owner_pid == pid) {
            PTE *p = frames[f].pte_ptr;
            if (p && (cur - p->last_ref > ws_k))
                return f;
        }
    }
    return find_victim_nru();
}