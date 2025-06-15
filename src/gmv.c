#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>
#include "gmv.h"
#include "algorithms.h"
#include "utils.h"

PTE page_table[N_PROCS][VPAGES];
Frame frames[N_FRAMES];
Stats stats = {0};
int extra_slots[N_PROCS] = {0};
int total_rounds;
PagerType current_pager;

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

// Print helper para operação
const char* op_str(AccessType op) {
    return (op == W_OP ? "W" : "R");
}

void load_page(int frame, int pid, int vpage) {
    // Proteção extra: nunca aceitar índices inválidos de frame
    if (frame < 0 || frame >= N_FRAMES) {
        die("BUG: Algoritmo retornou quadro inválido (%d)", frame);
    }

    PTE *vict = frames[frame].pte_ptr;

    if (vict && vict->present) {
        int px = pid2idx(frames[frame].owner_pid) + 1;
        int py = pid2idx(pid) + 1;
        printf("PF: P%d gerou PAGE FAULT e substituiu quadro %d (antes: P%d pág %d)",
               py, frame, px, frames[frame].vpage);
        if (vict->modified)
            printf(" [SUJA, gravada no disco]");
        printf("\n");
    }

    if (vict) {
        if (vict->modified) stats.dirty_writes++;
        vict->present = 0;
        vict->frame = -1;
        vict->modified = 0;
        vict->referenced = 0;
        vict->age = 0;
    }

    PTE *newpte = &page_table[pid2idx(pid)][vpage];
    newpte->present = 1;
    newpte->frame = frame;
    newpte->modified = 0;
    newpte->referenced = 1;
    newpte->age = 0;
    newpte->last_ref = stats.page_faults;

    frames[frame].owner_pid = pid;
    frames[frame].vpage = vpage;
    frames[frame].pte_ptr = newpte;

    printf("Quadro %d agora contém P%d pág %d\n", frame, pid2idx(pid)+1, vpage);
}

int find_victim(int pid) {
    switch (current_pager) {
        case PAGER_NRU: return find_victim_nru();
        case PAGER_2ND: return find_victim_2nd();
        case PAGER_LRU: return find_victim_lru();
        case PAGER_WS: return find_victim_ws(pid);
        default: return 0;
    }
}

void handle_access(const Access *a) {
    int idx = pid2idx(a->pid);
    if (idx == -1) die("pid desconhecido: %d", a->pid);
    PTE *pte = &page_table[pid2idx(a->pid)][a->page_idx];
    pte->referenced = 1;
    if (a->op == W_OP) pte->modified = 1;

    // Print do acesso
    printf("Processo %d: acesso %s na página %d\n", pid2idx(a->pid)+1, op_str(a->op), a->page_idx);

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
        extra_slots[pid2idx(a->pid)] = 2;
    }
    pte->last_ref = stats.page_faults;
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
        if (pipe(pfd[i]) < 0) die("pipe");
        if ((child_pid[i] = fork()) < 0) die("fork");
        if (child_pid[i] == 0) {
            close(pfd[i][0]);
            sprintf(file_name, "acessos_P%d", i + 1);
            char fdstr[8]; sprintf(fdstr, "%d", pfd[i][1]);
            execl("./processos", "processos", fdstr, file_name, NULL);
            die("exec processos");
        }
        close(pfd[i][1]);
        fcntl(pfd[i][0], F_SETFL, O_NONBLOCK);
        kill(child_pid[i], SIGSTOP);
    }
}

void run_gmv(void) {
    int accesses = 0;
    for (int rodada = 0; accesses < total_rounds * N_PROCS; rodada++) {
        printf("\n==== INÍCIO DA RODADA %d ====\n", rodada+1);
        for (int i = 0; i < N_PROCS && accesses < total_rounds * N_PROCS; i++) {
            printf("--> Executando processo P%d\n", i+1);
            kill(child_pid[i], SIGCONT);
            int slots = 1 + extra_slots[i];
            extra_slots[i] = 0;
            for (int s = 0; s < slots; s++) {
                poll_pipes_once();
                sleep(1);
            }
            kill(child_pid[i], SIGSTOP);
            accesses++;
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
        puts("VP | P M R | Frame | Age | LastRef");
        puts("------------------------------------------");
        for (int v = 0; v < VPAGES; v++) {
            PTE *t = &page_table[p][v];
            if (t->present == 0 && t->referenced == 0 && t->modified == 0) continue;
            printf("%3d | %d %d %d | %6d | %3u | %7u\n",
                   v, t->present, t->modified, t->referenced,
                   t->present ? t->frame : -1, t->age, t->last_ref);
        }
        putchar('\n');
    }
}
