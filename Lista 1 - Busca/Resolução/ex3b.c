#include "ex3.h"

#define NUM_JARROS 3
static const int CAPACIDADE[NUM_JARROS] = {12, 8, 3};

// Estado = quantidade de agua em cada jarro (12,8,3)
typedef struct {
    int agua[NUM_JARROS];
} Estado;

#define MAX_ESTADOS (13 * 9 * 4) // 0..12 x 0..8 x 0..3 

static int visitado[MAX_ESTADOS];
static int pai[MAX_ESTADOS];
static char acao[MAX_ESTADOS][40];

static int codifica(Estado e) {
    return e.agua[0] * (9 * 4) + e.agua[1] * 4 + e.agua[2];
}

static Estado decodifica(int codigo) {
    Estado e;
    e.agua[0] = codigo / (9 * 4);
    int resto = codigo % (9 * 4);
    e.agua[1] = resto / 4;
    e.agua[2] = resto % 4;
    return e;
}

// funcao-sucessor: gera todos os estados alcancaveis a partir de "atual"
// (encher um jarro, esvaziar um jarro, ou despejar de um jarro noutro)
static int geraSucessores(Estado atual, Estado sucessores[], char acoes[][40]) {
    int n = 0;

    for (int i = 0; i < NUM_JARROS; i++) {
        // encher jarro i 
        if (atual.agua[i] < CAPACIDADE[i]) {
            sucessores[n] = atual;
            sucessores[n].agua[i] = CAPACIDADE[i];
            sprintf(acoes[n], "Encher jarro %dL", CAPACIDADE[i]);
            n++;
        }
        // esvaziar jarro i 
        if (atual.agua[i] > 0) {
            sucessores[n] = atual;
            sucessores[n].agua[i] = 0;
            sprintf(acoes[n], "Esvaziar jarro %dL", CAPACIDADE[i]);
            n++;
        }
        // despejar jarro i em jarro j 
        for (int j = 0; j < NUM_JARROS; j++) {
            if (i == j) continue;
            if (atual.agua[i] > 0 && atual.agua[j] < CAPACIDADE[j]) {
                int quantidade = atual.agua[i];
                int espaco = CAPACIDADE[j] - atual.agua[j];
                if (quantidade > espaco) quantidade = espaco;

                sucessores[n] = atual;
                sucessores[n].agua[i] -= quantidade;
                sucessores[n].agua[j] += quantidade;
                sprintf(acoes[n], "Despejar jarro %dL no jarro %dL",
                        CAPACIDADE[i], CAPACIDADE[j]);
                n++;
            }
        }
    }
    return n;
}

static int ehObjetivo(Estado e) {
    return e.agua[0] == 1 || e.agua[1] == 1 || e.agua[2] == 1;
}

static void imprimeEstado(Estado e) {
    printf("(%dL=%d, %dL=%d, %dL=%d)",
           CAPACIDADE[0], e.agua[0], CAPACIDADE[1], e.agua[1], CAPACIDADE[2], e.agua[2]);
}

static void imprimeCaminho(int codigoFinal) {
    int caminho[MAX_ESTADOS], n = 0;
    int atual = codigoFinal;
    while (atual != -1) {
        caminho[n++] = atual;
        atual = pai[atual];
    }
    for (int i = n - 1; i >= 0; i--) {
        imprimeEstado(decodifica(caminho[i]));
        if (i > 0) printf("\n   --[%s]-->\n   ", acao[caminho[i - 1]]);
    }
    printf("\n");
}

void run_ex3b(void) {
    memset(visitado, 0, sizeof(visitado));
    for (int i = 0; i < MAX_ESTADOS; i++) pai[i] = -1;

    printf("\n===== Questao 3b: Jarros 12L, 8L, 3L -> medir 1L =====\n\n");

    Estado inicial = {{0, 0, 0}};
    int fila[MAX_ESTADOS], ini = 0, fim = 0;
    int codigoInicial = codifica(inicial);

    fila[fim++] = codigoInicial;
    visitado[codigoInicial] = 1;

    int expandidos = 0, encontrado = -1;
    clock_t t0 = clock();

    while (ini < fim) {
        int codigoAtual = fila[ini++];
        Estado atual = decodifica(codigoAtual);

        if (ehObjetivo(atual)) { encontrado = codigoAtual; break; }

        expandidos++;
        Estado sucessores[20];
        char acoes[20][40];
        int n = geraSucessores(atual, sucessores, acoes);

        for (int i = 0; i < n; i++) {
            int codigoSuc = codifica(sucessores[i]);
            if (!visitado[codigoSuc]) {
                visitado[codigoSuc] = 1;
                pai[codigoSuc] = codigoAtual;
                strcpy(acao[codigoSuc], acoes[i]);
                fila[fim++] = codigoSuc;
            }
        }
    }
    clock_t t1 = clock();

    if (encontrado != -1) {
        printf("Solucao encontrada! Caminho:\n\n   ");
        imprimeCaminho(encontrado);
    } else {
        printf("Nenhuma solucao encontrada (nao deveria acontecer aqui).\n");
    }

    printf("\nEstados expandidos: %d\n", expandidos);
    printf("Tempo de execucao: %.8f s\n", (double)(t1 - t0) / CLOCKS_PER_SEC);
}
