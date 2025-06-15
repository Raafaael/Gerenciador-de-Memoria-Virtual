#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gmv.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc != 3) die("uso: processos <fd_write> <arquivo_acesso>");
    int fd = atoi(argv[1]);
    char *path = argv[2];

    FILE *f = fopen(path, "r");
    if (!f) die("falha abrindo %s", path);

    char line[32];
    while (fgets(line, sizeof line, f)) {
        Access a;
        char op;
        sscanf(line, "%d %c", &a.page_idx, &op);
        a.op = (op == 'R') ? R_OP : W_OP;
        a.pid = getpid();
        safe_write(fd, &a, sizeof a);
        sleep(1);
    }
    fclose(f);
    return 0;
}
