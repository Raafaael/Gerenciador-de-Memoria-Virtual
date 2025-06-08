#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "gmv.h"
#include "utils.h"
#include "pager.h"

/* --------------------------------------------------------------------- */
static int ask_rounds(void)
{
    int r;
    printf("Número de rodadas (mín.1): ");
    while (scanf("%d", &r)!=1 || r<1) {
        while (getchar()!='\n');
        printf("Valor inválido – tente de novo: ");
    }
    return r;
}

static PagerType ask_algorithm(void)
{
    char s[8];
    printf("Algoritmo [NRU|2ND|LRU|WS3|WS5]: ");
    while (scanf("%7s", s)==1) {
        for (int i=0;s[i];i++) s[i]=toupper((unsigned char)s[i]);
        if (!strcmp(s,"NRU"))  return PAGER_NRU;
        if (!strcmp(s,"2ND"))  return PAGER_2ND;
        if (!strcmp(s,"LRU"))  return PAGER_LRU;
        if (!strcmp(s,"WS3"))  return PAGER_WS3;
        if (!strcmp(s,"WS5"))  return PAGER_WS5;
        printf("Opção inválida – tente novamente: ");
    }
    return PAGER_NRU;
}

int main(void)
{
    PagerType pager = ask_algorithm();
    int rounds      = ask_rounds();

    init_gmv(pager, rounds);
    run_gmv();
    dump_stats();
    dump_page_tables();
    return 0;
}
