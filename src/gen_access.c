#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 150
#define VPAGES 32

void generate_access_file(const char *filename, int process_id) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Erro ao abrir arquivo");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < N; i++) {
        int page = rand() % VPAGES;
        char op = (rand() % 2) == 0 ? 'R' : 'W';
        fprintf(f, "%02d %c\n", page, op);
    }
    fclose(f);
}

int main(void) {
    srand(time(NULL));
    for (int i = 1; i <= 4; i++) {
        char filename[20];
        snprintf(filename, sizeof(filename), "acessos_P%d", i);
        generate_access_file(filename, i);
    }
    printf("Arquivos de acesso gerados com sucesso.\n");
    return 0;
}
