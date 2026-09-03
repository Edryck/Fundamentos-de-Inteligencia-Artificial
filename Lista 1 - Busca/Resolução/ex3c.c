#include "ex3.h"

#define NUM_LOCAIS 4
// 0 = onde o macaco comeca | 1 = onde esta o caixote A |
// 2 = onde esta o caixote B | 3 = embaixo da banana 
static const char *nomeLocal[NUM_LOCAIS] = {
    "Ponto A (inicio)", "Ponto B (caixote 1)", "Ponto C (caixote 2)", "Ponto D (embaixo da banana)"
};
#define LOCAL_BANANA 3

// Estado do problema: onde esta o macaco, onde estao os dois caixotes,
// se estao empilhados, se o macaco esta em cima deles e se ja pegou a banana. 
typedef struct {
    int macacoLoc;
    int caixaALoc;
    int caixaBLoc;
    int empilhado;   // 0/1 
    int emCima;      // 0/1: macaco esta em cima dos caixotes empilhados 
    int temBanana;   // 0/1 
} Estado;

// codifica o estado num inteiro para usar como indice de visitado[]/pai[] 
#define MAX_ESTADOS (NUM_LOCAIS * NUM_LOCAIS * NUM_LOCAIS * 2 * 2 * 2)

static int codifica(Estado e) {
    return e.macacoLoc * (NUM_LOCAIS * NUM_LOCAIS * 2 * 2 * 2)
         + e.caixaALoc * (NUM_LOCAIS * 2 * 2 * 2)
         + e.caixaBLoc * (2 * 2 * 2)
         + e.empilhado * 4
         + e.emCima * 2
         + e.temBanana;
}

static Estado decodifica(int codigo) {
    Estado e;
    e.macacoLoc = codigo / (NUM_LOCAIS * NUM_LOCAIS * 2 * 2 * 2);
    codigo %= (NUM_LOCAIS * NUM_LOCAIS * 2 * 2 * 2);
    e.caixaALoc = codigo / (NUM_LOCAIS * 2 * 2 * 2);
    codigo %= (NUM_LOCAIS * 2 * 2 * 2);
    e.caixaBLoc = codigo / (2 * 2 * 2);
    codigo %= (2 * 2 * 2);
    e.empilhado = codigo / 4;
    codigo %= 4;
    e.emCima = codigo / 2;
    e.temBanana = codigo % 2;
    return e;
}

// funcao-sucessor: gera todas as acoes possiveis (andar, empurrar caixote,
// empilhar, subir, descer, pegar banana) a partir do estado atual 
static int geraSucessores(Estado atual, Estado sucessores[], char acoes[][60]) {
    int n = 0;

    // andar (so pode andar se nao estiver em cima dos caixotes) 
    if (!atual.emCima) {
        for (int loc = 0; loc < NUM_LOCAIS; loc++) {
            if (loc == atual.macacoLoc) continue;
            sucessores[n] = atual;
            sucessores[n].macacoLoc = loc;
            sprintf(acoes[n], "Andar ate %s", nomeLocal[loc]);
            n++;
        }
    }

    // empurrar caixote A (precisa estar junto dele, e caixotes ainda nao empilhados) 
    if (!atual.emCima && !atual.empilhado && atual.macacoLoc == atual.caixaALoc) {
        for (int loc = 0; loc < NUM_LOCAIS; loc++) {
            if (loc == atual.macacoLoc) continue;
            sucessores[n] = atual;
            sucessores[n].macacoLoc = loc;
            sucessores[n].caixaALoc = loc;
            sprintf(acoes[n], "Empurrar caixote 1 ate %s", nomeLocal[loc]);
            n++;
        }
    }

    // empurrar caixote B 
    if (!atual.emCima && !atual.empilhado && atual.macacoLoc == atual.caixaBLoc) {
        for (int loc = 0; loc < NUM_LOCAIS; loc++) {
            if (loc == atual.macacoLoc) continue;
            sucessores[n] = atual;
            sucessores[n].macacoLoc = loc;
            sucessores[n].caixaBLoc = loc;
            sprintf(acoes[n], "Empurrar caixote 2 ate %s", nomeLocal[loc]);
            n++;
        }
    }

    // empilhar os dois caixotes (precisam estar no mesmo lugar que o macaco) 
    if (!atual.emCima && !atual.empilhado &&
        atual.macacoLoc == atual.caixaALoc && atual.macacoLoc == atual.caixaBLoc) {
        sucessores[n] = atual;
        sucessores[n].empilhado = 1;
        sprintf(acoes[n], "Empilhar os dois caixotes em %s", nomeLocal[atual.macacoLoc]);
        n++;
    }

    // subir nos caixotes empilhados 
    if (!atual.emCima && atual.empilhado && atual.macacoLoc == atual.caixaALoc) {
        sucessores[n] = atual;
        sucessores[n].emCima = 1;
        sprintf(acoes[n], "Subir nos caixotes");
        n++;
    }

    // descer dos caixotes 
    if (atual.emCima) {
        sucessores[n] = atual;
        sucessores[n].emCima = 0;
        sprintf(acoes[n], "Descer dos caixotes");
        n++;
    }

    // pegar a banana: precisa estar em cima dos caixotes, empilhados,
    // e tudo isso embaixo da banana 
    if (atual.emCima && atual.empilhado &&
        atual.macacoLoc == LOCAL_BANANA && atual.caixaALoc == LOCAL_BANANA) {
        sucessores[n] = atual;
        sucessores[n].temBanana = 1;
        sprintf(acoes[n], "Esticar o braco e pegar a banana");
        n++;
    }

    return n;
}

static int ehObjetivo(Estado e) { return e.temBanana; }

static void imprimeEstado(Estado e) {
    printf("[macaco em %s | caixote1 em %s | caixote2 em %s | empilhados=%s | em_cima=%s | banana=%s]",
           nomeLocal[e.macacoLoc], nomeLocal[e.caixaALoc], nomeLocal[e.caixaBLoc],
           e.empilhado ? "sim" : "nao", e.emCima ? "sim" : "nao", e.temBanana ? "SIM" : "nao");
}

static int visitado[MAX_ESTADOS];
static int pai[MAX_ESTADOS];
static char acaoQueChegouAqui[MAX_ESTADOS][60];

static void imprimeCaminho(int codigoFinal) {
    int caminho[MAX_ESTADOS], n = 0;
    int atual = codigoFinal;
    while (atual != -1) {
        caminho[n++] = atual;
        atual = pai[atual];
    }
    for (int i = n - 1; i >= 0; i--) {
        printf("%d) ", n - i);
        imprimeEstado(decodifica(caminho[i]));
        printf("\n");
        if (i > 0) printf("   --[%s]-->\n", acaoQueChegouAqui[caminho[i - 1]]);
    }
}

void run_ex3c(void) {
    memset(visitado, 0, sizeof(visitado));
    for (int i = 0; i < MAX_ESTADOS; i++) pai[i] = -1;

    printf("\n===== Questao 3c: Macaco e as bananas =====\n\n");
    printf("Cenario: macaco em A, caixote 1 em B, caixote 2 em C, banana em D.\n");
    printf("Bananas estao altas demais: precisa empurrar os dois caixotes\n");
    printf("ate debaixo da banana, empilhar e subir para alcancar.\n\n");

    Estado inicial = {0, 1, 2, 0, 0, 0}; // macaco=A, caixaA=B, caixaB=C 
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
        Estado sucessores[NUM_LOCAIS * 3 + 5];
        char acoes[NUM_LOCAIS * 3 + 5][60];
        int n = geraSucessores(atual, sucessores, acoes);

        for (int i = 0; i < n; i++) {
            int codigoSuc = codifica(sucessores[i]);
            if (!visitado[codigoSuc]) {
                visitado[codigoSuc] = 1;
                pai[codigoSuc] = codigoAtual;
                strcpy(acaoQueChegouAqui[codigoSuc], acoes[i]);
                fila[fim++] = codigoSuc;
            }
        }
    }
    clock_t t1 = clock();

    if (encontrado != -1) {
        printf("Solucao encontrada! Sequencia de acoes:\n\n");
        imprimeCaminho(encontrado);
    } else {
        printf("Nenhuma solucao encontrada.\n");
    }

    printf("\nEstados expandidos: %d\n", expandidos);
    printf("Tempo de execucao: %.8f s\n", (double)(t1 - t0) / CLOCKS_PER_SEC);
}
