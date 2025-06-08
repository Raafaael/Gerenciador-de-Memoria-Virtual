#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>
#include "gmv.h"
#include "pager.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>
#include "gmv.h"
#include "pager.h"
#include "utils.h"

/* variáveis globais ---------------------------------------------------- */
PTE    page_table[4][32];   /* 4 processos, 32 páginas por processo */
Frame  frames[16];          /* 16 quadros de memória */
Stats  stats = {0};         /* Inicializar as estatísticas */
int current_pager;
int extra_slots[4] = {0};   /* extra slots para cada processo */
int total_rounds;           /* total de rodadas */

/* filhos */
int pfd[4][2];
int child_pid[4];


/* utilidades ----------------------------------------------------------- */
int pid2idx(int pid)
{
    int i;
    for (i = 0; i < 4; i++) {
        if (child_pid[i] == pid) {
            return i;
        }
    }
    return 0;  /* retornar 0 por padrão se não encontrar */
}

/* Aging tick a cada acesso */
void aging_tick(void)
{
    if (current_pager != PAGER_LRU) {
        return;
    }

    int f;
    for (f = 0; f < 16; ++f) {
        PTE *p = frames[f].pte_ptr;
        p->age >>= 1;
        if (p->referenced) {
            p->age |= 0x80;
        }
        p->referenced = 0;
    }
}

/* carregar página ------------------------------------------------------ */
void load_page(int frame, int pid, int vpage)
{
    PTE *vict = frames[frame].pte_ptr;

    /* Verifique se o ponteiro pte_ptr não é NULL */
    if (vict == NULL) {
        die("Erro: ponteiro inválido para a tabela de páginas no quadro %d", frame);
    }

    if (vict->modified) {  // Aqui o erro ocorria, agora verificamos se está inicializado corretamente
        stats.dirty_writes++;
    }

    /* Atualiza entrada vítima */
    vict->present = 0;
    vict->modified = 0;
    vict->referenced = 0;

    /* Nova entrada */
    PTE *newpte = &page_table[pid2idx(pid)][vpage];
    newpte->present = 1;
    newpte->modified = 0;
    newpte->referenced = 1;
    newpte->frame = frame;
    newpte->age = 0;
    newpte->last_ref = stats.page_faults;

    /* Quadro físico */
    frames[frame].owner_pid = pid;
    frames[frame].vpage = vpage;
    frames[frame].pte_ptr = newpte;
}

/* escolher vítima conforme algoritmo ----------------------------------- */
int find_victim(int pid)
{
    switch (current_pager) {
        case PAGER_NRU:
            return find_victim_nru();
        case PAGER_2ND:
            return find_victim_2nd();
        case PAGER_LRU:
            return find_victim_lru();
        case PAGER_WS3:
        case PAGER_WS5:
            return find_victim_ws();
        default:
            return 0;
    }
}

/* tratar acesso -------------------------------------------------------- */
void handle_access(const Access *a)
{
    PTE *pte = &page_table[pid2idx(a->pid)][a->page_idx];
    pte->referenced = 1;
    if (a->op == W_OP) {
        pte->modified = 1;
    }

    if (pte->present == 0) {
        stats.page_faults++;
        int frame = -1;
        /* há espaço livre? */
        int f;
        for (f = 0; f < 16; ++f) {
            if (frames[f].pte_ptr == NULL) {
                frame = f;
                break;
            }
        }
        if (frame == -1) {
            frame = find_victim(a->pid);
        }
        load_page(frame, a->pid, a->page_idx);

        /* ponto-extra: 2 slots extras */
        extra_slots[pid2idx(a->pid)] = 2;
    }
    pte->last_ref = stats.page_faults;
    aging_tick();
}

/* leitura não bloqueante de todos os pipes ----------------------------- */
void poll_pipes_once(void)
{
    fd_set set;
    FD_ZERO(&set);
    int maxfd = -1;
    int i;
    for (i = 0; i < 4; i++) {
        FD_SET(pfd[i][0], &set);
        if (pfd[i][0] > maxfd) {
            maxfd = pfd[i][0];
        }
    }
    struct timeval tv = {0, 0};  /* sem espera */
    if (select(maxfd + 1, &set, NULL, NULL, &tv) <= 0) {
        return;
    }

    for (i = 0; i < 4; i++) {
        if (FD_ISSET(pfd[i][0], &set)) {
            Access a;
            if (read(pfd[i][0], &a, sizeof(a)) == sizeof(a)) {
                handle_access(&a);
            }
        }
    }
}

/* API pública ---------------------------------------------------------- */
void init_gmv(int pager, int rounds)
{
    current_pager = pager;
    total_rounds = rounds;

    /* cria filhos + pipes */
    char file_name[32];
    int i;
    for (i = 0; i < 4; i++) {
        if (pipe(pfd[i]) < 0) {
            die("pipe");
        }
        if ((child_pid[i] = fork()) < 0) {
            die("fork");
        }
        if (child_pid[i] == 0) {  /* filho */
            close(pfd[i][0]);
            sprintf(file_name, "acessos_P%d", i + 1);
            char fdstr[8];
            sprintf(fdstr, "%d", pfd[i][1]);
            execl("./processos", "processos", fdstr, file_name, NULL);
            die("exec processos");
        }
        close(pfd[i][1]);  /* pai só lê */
        fcntl(pfd[i][0], F_SETFL, O_NONBLOCK);
        kill(child_pid[i], SIGSTOP);  /* iniciam parados */
    }
}

void run_gmv(void)
{
    int accesses = 0;
    while (accesses < total_rounds * 4) {
        int i;
        for (i = 0; i < 4 && accesses < total_rounds * 4; i++) {
            kill(child_pid[i], SIGCONT);

            int slots = 1 + extra_slots[i];
            extra_slots[i] = 0;

            int s;
            for (s = 0; s < slots; s++) {
                sleep(1);  /* 1 segundo */
                poll_pipes_once();
            }
            kill(child_pid[i], SIGSTOP);
            accesses++;
        }
    }
}

void dump_stats(void)
{
    puts("\n========= Estatísticas =========");
    printf("Page-faults.............: %d\n", stats.page_faults);
    printf("Páginas sujas gravadas..: %d\n", stats.dirty_writes);
}

void dump_page_tables(void)
{
    puts("\n========= Tabelas de Página =========");
    int p;
    for (p = 0; p < 4; p++) {
        printf("Processo P%d\n", p + 1);
        puts("VPg | P M R | Quadro | Age | LastRef");
        puts("------------------------------------------");
        int v;
        for (v = 0; v < 32; v++) {
            PTE *t = &page_table[p][v];
            if (t->present == 0 && t->referenced == 0 && t->modified == 0) {
                continue;
            }
            printf("%3d | %d %d %d | %6d | %3d | %7d\n",
                   v, t->present, t->modified, t->referenced,
                   t->present ? t->frame : -1, t->age, t->last_ref);
        }
        putchar('\n');
    }
}
