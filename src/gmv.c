#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>
#include <stdlib.h>
#include "gmv.h"
#include "algorithms.h"
#define NRU_RESET_TICKS 8

PTE page_table[N_PROCS][VPAGES];
Frame frames[N_FRAMES];
Stats stats = {0};
int pf_delay[N_PROCS] = {0}; 
int total_rounds;
PagerType current_pager;
static int acessos_por_proc = 150;
static int acessos_lidos[N_PROCS] = {0};
static int total_lidos = 0;
unsigned long long tick = 0;

int pfd[N_PROCS][2];
int child_pid[N_PROCS];

int pid2idx(int pid) {
    for (int i = 0; i < N_PROCS; i++) {
        if (child_pid[i] == pid) return i;
    }
    return -1;
}

void aging_tick(void) {
    if (current_pager != PAGER_LRU) return;
    for (int f = 0; f < N_FRAMES; ++f) {
        PTE *p = frames[f].pte_ptr;
        if (p) {
            p->age >>= 1;
            if (p->referenced) p->age |= 0x80;
            p->referenced = 0;
        }
    }
}

const char* op_str(AccessType op) {
    return (op == W_OP ? "W" : "R");
}

void load_page(int frame, int pid, int vpage) {
    if (frame < 0 || frame >= N_FRAMES) {
        fprintf(stderr,"BUG: Algoritmo retornou quadro inválido (%d)\n", frame);
        exit(1);
    }

    PTE *vict = frames[frame].pte_ptr;

    // 1. Limpe a vítima e contabilize ANTES de sobrescrever o frame
    if (vict && vict->present) {
        printf("DEBUG: Antes de remover, modified=%d, frame=%d, vpage=%d\n", vict->modified, frame, frames[frame].vpage);
        int px = pid2idx(frames[frame].owner_pid) + 1;
        int py = pid2idx(pid) + 1;
        printf("PF: P%d gerou PAGE FAULT e substituiu quadro %d (antes: P%d pág %d)",
               py, frame, px, frames[frame].vpage);
        if (vict->modified) {
            printf(" [SUJA, gravada no disco]");
            stats.dirty_writes++;
        }
        printf("\n");

        vict->present = 0;
        vict->frame = -1;
        vict->modified = 0;
        vict->referenced = 0;
        vict->age = 0;
    }

    // 2. Só depois, atualize o frame para o novo PTE
    PTE *newpte = &page_table[pid2idx(pid)][vpage];
    newpte->present = 1;
    newpte->frame = frame;
    newpte->modified = 0;
    newpte->referenced = 1;
    newpte->age = 0;
    newpte->last_ref = tick;

    frames[frame].owner_pid = pid;
    frames[frame].vpage = vpage;
    frames[frame].pte_ptr = newpte;

    printf("Quadro %d agora contém P%d pág %d\n", frame, pid2idx(pid)+1, vpage);
}

int find_victim(int pid) {
    switch (current_pager) {
        case PAGER_NRU: return find_victim_nru();
        case PAGER_2ND: return find_victim_2nd();
        case PAGER_LRU: return find_victim_lru(pid);
        case PAGER_WS: return find_victim_ws(pid);
        default: return 0;
    }
}

void handle_access(const Access *a) {
    tick++; 
    int idx = pid2idx(a->pid);
    if (idx == -1) {
        fprintf(stderr,"pid desconhecido: %d\n", a->pid);
        exit(1);
    }
    PTE *pte = &page_table[idx][a->page_idx];
    pte->referenced = 1;
    if (a->op == W_OP) {
        pte->modified = 1;
    }

    // Print do acesso
    printf("Processo %d: acesso %s na página %d\n", idx+1, op_str(a->op), a->page_idx);

    if (pte->present == 0) {
        stats.page_faults++;
        int frame = -1;
        for (int f = 0; f < N_FRAMES; ++f) {
            if (frames[f].pte_ptr == NULL) {
                frame = f;
                break;
            }
        }
        if (frame == -1) frame = find_victim(a->pid);

        printf("  >> PAGE FAULT! (total PFs: %d)\n", stats.page_faults);
        load_page(frame, a->pid, a->page_idx);
        pf_delay[idx] = 2;
    }
    pte->last_ref = tick;
    aging_tick();
}

void poll_pipes_once(void) {
    fd_set set;
    FD_ZERO(&set);
    int maxfd = -1;
    for (int i = 0; i < N_PROCS; i++) {
        FD_SET(pfd[i][0], &set);
        if (pfd[i][0] > maxfd) maxfd = pfd[i][0];
    }
    struct timeval tv = {0, 0};
    if (select(maxfd + 1, &set, NULL, NULL, &tv) <= 0) return;

    for (int i = 0; i < N_PROCS; i++) {
        if (FD_ISSET(pfd[i][0], &set)) {
            Access a;
            if (read(pfd[i][0], &a, sizeof(a)) == sizeof(a)) handle_access(&a);
        }
    }
}

void init_gmv(PagerType pager, int rounds) {
    current_pager = pager;
    total_rounds = rounds;
    for (int i = 0; i < N_FRAMES; ++i) {
        frames[i].pte_ptr = NULL;
        frames[i].owner_pid = -1;
        frames[i].vpage = -1;
    }

    char file_name[32];
    for (int i = 0; i < N_PROCS; i++) {
        if (pipe(pfd[i]) < 0) {
            fprintf(stderr,"pipe error\n");
            exit(1);
        }
        if ((child_pid[i] = fork()) < 0) {
            fprintf(stderr,"fork error\n");
            exit(1);
        }
        if (child_pid[i] == 0) {
            close(pfd[i][0]);
            sprintf(file_name, "acessos_P%d.txt", i + 1);

            FILE *f = fopen(file_name, "r");
            if (!f) {
                fprintf(stderr,"falha abrindo %s\n", file_name);
                exit(1);
            }

            char line[32];
            while (fgets(line, sizeof line, f)) {
                Access a;
                char op;
                sscanf(line, "%d %c", &a.page_idx, &op);
                a.op = (op == 'R') ? R_OP : W_OP;
                a.pid = getpid();
                write(pfd[i][1], &a, sizeof a);
                sleep(1);
            }
            fclose(f);
            exit(0);
        }
        close(pfd[i][1]);
        fcntl(pfd[i][0], F_SETFL, O_NONBLOCK);
        kill(child_pid[i], SIGSTOP);
    }
}

void run_gmv(void) {
    int total_acessos = acessos_por_proc * N_PROCS;
    int rodada = 0;

    while (total_lidos < total_acessos && /* ainda há linhas */
           rodada < total_rounds) { /* não passou rodadas */ 
        printf("\n==== INÍCIO DA RODADA %d ====\n", ++rodada);

        /* -------- Round-Robin: P1..P4 -------- */
        for (int i = 0; i < N_PROCS && total_lidos < total_acessos; ++i) {

            if (acessos_lidos[i] >= acessos_por_proc)
                continue; /* filho P i já terminou      */

            printf("--> Executando processo P%d\n", i + 1);
            kill(child_pid[i], SIGCONT);

            /* ---------- quantum “normal” de 1 s ---------- */
            if (acessos_lidos[i] < acessos_por_proc) {
                fd_set set;
                FD_ZERO(&set);
                FD_SET(pfd[i][0], &set);
                struct timeval tv = {0, 200000};   /* 0,2 s timeout */

                if (select(pfd[i][0] + 1, &set, NULL, NULL, &tv) > 0) {
                    Access a;
                    if (read(pfd[i][0], &a, sizeof a) == sizeof a) {
                        handle_access(&a);
                        acessos_lidos[i]++;
                        total_lidos++;
                    }
                }
                sleep(1);
            }

            /* ---------- atraso extra por page-fault ---------- */
            if (pf_delay[i] > 0) {
                printf("(delay de %d s para I/O de page-fault)\n", pf_delay[i]);
                sleep(pf_delay[i]);
                pf_delay[i] = 0;  /* já “pagou” o disco */
            }

            kill(child_pid[i], SIGSTOP);
        }

        /* zera o bit R entre rodadas se o pager for NRU */
        if (current_pager == PAGER_NRU && (tick % NRU_RESET_TICKS) == 0){
            for (int f = 0; f < N_FRAMES; ++f)
                if (frames[f].pte_ptr)
                    frames[f].pte_ptr->referenced = 0;
        }
    }
}

void dump_stats(void) {
    puts("\n========= Estatísticas =========");
    printf("Page-faults.............: %d\n", stats.page_faults);
    printf("Páginas sujas gravadas..: %d\n", stats.dirty_writes);
}

void dump_page_tables(void) {
    puts("\n========= Tabelas de Página =========");
    for (int p = 0; p < N_PROCS; p++) {
        printf("Processo P%d\n", p + 1);
        puts("VP  | P M R | Frame | Age | LastRef");
        puts("------------------------------------------");
        for (int v = 0; v < VPAGES; v++) {
            PTE *t = &page_table[p][v];
            if (t->present == 0 && t->referenced == 0 && t->modified == 0) continue;
            printf("%3d | %d %d %d | %5d | %3u | %7u\n",
                   v, t->present, t->modified, t->referenced,
                   t->present ? t->frame : -1, t->age, t->last_ref);
        }
        putchar('\n');
    }
}
