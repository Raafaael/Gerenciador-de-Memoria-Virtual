#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "algorithms.h"
#include "gmv.h"

// NRU: Not Recently Used
int find_victim_nru(void) {
    int best = -1, best_class = 4;
    for (int f = 0; f < N_FRAMES; ++f) {
        TableEntry *p = frames[f].TableEntry_ptr;
        if (!p) continue;

        /* Classe 0 = (R=0, M=0); 1 = (0,1); 2 = (1,0); 3 = (1,1) */
        int cls = (p->referenced ? 2 : 0) + (p->modified ? 1 : 0);

        printf("NRU: frame %2d  class %d  M=%d R=%d\n", f, cls, p->modified, p->referenced);

        if (cls < best_class) {
            best_class = cls;
            best = f;
            if (best_class == 0) break;
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
    int turns = 0;

    while (turns < 2 * N_FRAMES) {
        TableEntry *p = frames[hand].TableEntry_ptr;

        /* 1. Encontrou quadro válido e R=0 -> é a vítima */
        if (p && !p->referenced) {
            int victim = hand;   /* Guarda o índice escolhido */
            hand = (hand + 1) % N_FRAMES;  /* Avança para o próximo quadro */
            return victim;
        }

        /* 2. Se houver página, zera R e dá a “segunda chance” */
        if (p) p->referenced = 0;

        /* 3. Avança ponteiro para o próximo frame */
        hand = (hand + 1) % N_FRAMES;
        turns++;
    }
    fprintf(stderr, "Erro 2ND: Nenhum quadro elegível para substituição!\n");
    exit(1);
}

// LRU: Least Recently Used (usando Aging)
int find_victim_lru(int pid) {
    /* 1. Se tiver quadros livros, fica com ele */
    for (int f = 0; f < N_FRAMES; ++f)
        if (frames[f].TableEntry_ptr == NULL)
            return f;

    /* 2. Procura o quadro mais antigo do próprio processo */
    int victim = -1;
    unsigned min_age = UINT_MAX;

    for (int f = 0; f < N_FRAMES; ++f) {
        if (frames[f].owner_pid != pid)
            continue;

        TableEntry *p = frames[f].TableEntry_ptr;
        if (!p) continue;

        printf("DEBUG LRU: frame %2d  vpage %2d  age=%3u (menor até agora=%3u)\n", f, frames[f].vpage, p->age, min_age);

        if (p && p->age < min_age) { /* quanto menor, mais velho */
            min_age = p->age;
            victim  = f;
        }
    }
    if (victim != -1)
        return victim;

    fprintf(stderr, "LRU local: pid %d ficou sem molduras.\n", pid);
    exit(1);
}

int ws_k = 0;
// Working Set
int find_victim_ws(int pid) {
    unsigned long long cur = tick;
    int oldest_frame = -1;
    unsigned long long oldest_time = ULONG_MAX;

    for (int f = 0; f < N_FRAMES; ++f) {
        if (frames[f].owner_pid == pid) {
            TableEntry *p = frames[f].TableEntry_ptr;
            if (p && (cur - p->last_ref > ws_k)) {
                return f;  // Retorna a vítima caso encontre uma página fora da janela de trabalho
            }

            // Se nenhuma página for substituível, procuramos a mais antiga
            if (p && p->last_ref < oldest_time) {
                oldest_time = p->last_ref;
                oldest_frame = f;
            }
        }
    }

    if (oldest_frame != -1) {
        return oldest_frame;
    }

    fprintf(stderr, "Erro WS: Nenhum quadro elegível para substituição!\n");
    exit(1);
}
