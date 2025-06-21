#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "gmv.h"
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

static Algorithm ask_algorithm(void) {
    char s[8];
    printf("Algoritmo [NRU|2ND|LRU|WS]: ");
    while (scanf("%7s", s)==1) {
        for (int i=0;s[i];i++) s[i]=toupper((unsigned char)s[i]);
        if (!strcmp(s,"NRU"))  return ALGORITHM_NRU;
        if (!strcmp(s,"2ND"))  return ALGORITHM_2ND;
        if (!strcmp(s,"LRU"))  return ALGORITHM_LRU;
        if (!strcmp(s,"WS"))   return ALGORITHM_WS;
        printf("Opção inválida – tente novamente: ");
    }
    return ALGORITHM_NRU;
}

int main(void) {
    Algorithm algorithm = ask_algorithm();
    int rounds = ask_rounds();
    if (algorithm == ALGORITHM_WS) {
        ws_k = ask_ws_k();
    }

    if (algorithm == ALGORITHM_WS)
        printf("Algoritmo de Substituição WS %d\n", ws_k);
    else if (algorithm == ALGORITHM_NRU)
        printf("Algoritmo de Substituição NRU\n");
    else if (algorithm == ALGORITHM_2ND)
        printf("Algoritmo de Substituição 2ND\n");
    else if (algorithm == ALGORITHM_LRU)
        printf("Algoritmo de Substituição LRU\n");
    printf("Rodadas executadas: %d\n", rounds);

    // INICIA SIMULAÇÃO
    init_gmv(algorithm, rounds);
    run_gmv();
    dump_stats();
    dump_page_tables();
    return 0;
}
