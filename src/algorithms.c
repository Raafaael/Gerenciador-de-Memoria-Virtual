#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "algorithms.h"
#include "gmv.h"

// NRU: Not Recently Used
/* ---------- algorithms.c ---------- */
int find_victim_nru(void) {
    int best = -1, best_class = 4;
    for (int f = 0; f < N_FRAMES; ++f) {
        PTE *p = frames[f].pte_ptr;
        if (!p) continue;

        /* classe 0 = (R=0, M=0); 1 = (0,1); 2 = (1,0); 3 = (1,1) */
        int cls = (p->referenced ? 2 : 0) + (p->modified ? 1 : 0);

        printf("NRU: frame %2d  class %d  M=%d R=%d\n", f, cls, p->modified, p->referenced);

        if (cls < best_class) {
            best_class = cls;
            best = f;
            if (best_class == 0) break;   /* não há melhor que a classe-0 */
        }
    }
    if (best == -1) {
        fprintf(stderr, "Erro NRU: Nenhum quadro elegível!\n");
        exit(1);
    }
    return best;
}

// Segunda Chance (Second Chance)
static int hand = 0;
int find_victim_2nd(void) {
    int voltas = 0;  /* quantos quadros já examinamos */

    while (voltas < 2 * N_FRAMES) {   /* evita loop infinito improvável */
        PTE *p = frames[hand].pte_ptr;

        /* 1. Encontrou quadro válido e R=0 → é a vítima */
        if (p && !p->referenced) {
            int victim = hand;   /* guarda o índice escolhido */
            hand = (hand + 1) % N_FRAMES;  /* AVANÇA para o próximo quadro */
            return victim;
        }

        /* 2. Se houver página, zera R e dá a “segunda chance” */
        if (p) p->referenced = 0;

        /* 3. Avança ponteiro para o próximo frame */
        hand = (hand + 1) % N_FRAMES;
        voltas++;
    }
    fprintf(stderr, "Erro 2ND: Nenhum quadro elegível para substituição!\n");
    exit(1);
}

// LRU: Least Recently Used (usando Aging)
int find_victim_lru(int pid) {
    /* 1. Há quadro livre? Fica com ele. */
    for (int f = 0; f < N_FRAMES; ++f)
        if (frames[f].pte_ptr == NULL)
            return f;

    /* 2. Procura o quadro MAIS antigo (menor age) do próprio processo */
    int victim = -1;
    unsigned min_age = UINT_MAX;

    for (int f = 0; f < N_FRAMES; ++f) {
        if (frames[f].owner_pid != pid) /* só páginas do mesmo proc. */
            continue;

        PTE *p = frames[f].pte_ptr;
        if (!p) continue;

        printf("LRU-scan: frame %2d  vpage %2d  age=%3u (menor até agora=%3u)\n", f, frames[f].vpage, p->age, min_age);

        if (p && p->age < min_age) { /* quanto menor, mais velho */
            min_age = p->age;
            victim  = f;
        }
    }
    if (victim != -1)
        return victim;

    /* 3. NÃO deveria acontecer, mas... faz um fallback global para não travar */
    fprintf(stderr, "LRU local: pid %d ficou sem molduras.\n", pid);
    exit(1);
}

extern int ws_k; // Variável global para o parâmetro do WS
// Working Set
int find_victim_ws(int pid) {
    unsigned long long cur = tick; 
    for (int f = 0; f < N_FRAMES; ++f) {
        if (frames[f].owner_pid == pid) {
            PTE *p = frames[f].pte_ptr;
            if (p && (cur - p->last_ref > ws_k))
                return f;
        }
    }
    fprintf(stderr, "Erro WS: Nenhum quadro elegível para substituição!\n");
    exit(1);
}
