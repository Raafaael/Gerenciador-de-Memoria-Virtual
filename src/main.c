#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "gmv.h"
#include "utils.h"
#include "algorithms.h"

int ws_k = 0; // global para o parâmetro do WS

static int ask_rounds(void) {
    int r;
    printf("Número de rodadas (mín.1): ");
    while (scanf("%d", &r)!=1 || r<1) {
        while (getchar()!='\n');
        printf("Valor inválido – tente de novo: ");
    }
    return r;
}

static int ask_ws_k(void) {
    int d;
    printf("Janela do Working Set (mín.1): ");
    while (scanf("%d", &d)!=1 || d<1) {
        while (getchar()!='\n');
        printf("Valor inválido – tente de novo: ");
    }
    return d;
}

static PagerType ask_algorithm(void) {
    char s[8];
    printf("Algoritmo [NRU|2ND|LRU|WS]: ");
    while (scanf("%7s", s)==1) {
        for (int i=0;s[i];i++) s[i]=toupper((unsigned char)s[i]);
        if (!strcmp(s,"NRU"))  return PAGER_NRU;
        if (!strcmp(s,"2ND"))  return PAGER_2ND;
        if (!strcmp(s,"LRU"))  return PAGER_LRU;
        if (!strcmp(s,"WS"))   return PAGER_WS;
        printf("Opção inválida – tente novamente: ");
    }
    return PAGER_NRU;
}

int main(void) {
    PagerType pager = ask_algorithm();
    int rounds = ask_rounds();
    if (pager == PAGER_WS) {
        ws_k = ask_ws_k();
    }

    if (pager == PAGER_WS)
        printf("Algoritmo de Substituição WS %d\n", ws_k);
    else if (pager == PAGER_NRU)
        printf("Algoritmo de Substituição NRU\n");
    else if (pager == PAGER_2ND)
        printf("Algoritmo de Substituição 2ND\n");
    else if (pager == PAGER_LRU)
        printf("Algoritmo de Substituição LRU\n");
    printf("Rodadas executadas: %d\n", rounds);

    // INICIA SIMULAÇÃO
    init_gmv(pager, rounds);
    run_gmv();
    dump_stats();
    dump_page_tables();
    return 0;
}
