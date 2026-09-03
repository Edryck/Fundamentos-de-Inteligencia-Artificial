#include <stdio.h>
#include "ex3.h"
#include "ex8.h"
#include "ex9.h"

int main(void) {
    int opcao;

    do {
        printf("========================================\n");
        printf(" Lista 1 - Busca - Menu\n");
        printf("========================================\n");
        printf(" 1 - Questao 3a (Coloracao de mapa)\n");
        printf(" 2 - Questao 3b (Jarros de agua)\n");
        printf(" 3 - Questao 3c (Macaco e as bananas)\n");
        printf(" 4 - Questao 8 (School->Museum)\n");
        printf(" 5 - Questao 9 (Prof. Limitada x Aprof. Iterativo)\n");
        printf(" 0 - Sair\n");
        printf("========================================\n");
        printf("Escolha uma opcao: ");

        if (scanf("%d", &opcao) != 1) {
            printf("Entrada invalida.\n");
            break;
        }

        switch (opcao) {
            case 1:
                run_ex3a();
                break;
            case 2:
                run_ex3b();
                break;
            case 3:
                run_ex3c();
                break;
            case 4:
                run_ex8();
                break;
            case 5:
                run_ex9();
                break;
            case 0:
                printf("Encerrando...\n");
                break;
            default:
                printf("Opcao invalida.\n");
        }

    } while (opcao != 0);

    return 0;
}
