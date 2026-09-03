#include "ex8.h"

#define MAX_NOS 10
#define MAX_NOME 20

// aresta da lista de adjacencia
typedef struct Aresta {
    int destino;
    struct Aresta *prox;
} Aresta;

// grafo simples: vetor de nomes + lista de adjacencia por indice
typedef struct {
    char nomes[MAX_NOS][MAX_NOME];
    Aresta *adj[MAX_NOS];
    int numNos;
} Grafo;

static void grafo_init(Grafo *g) {
    g->numNos = 0;
    for (int i = 0; i < MAX_NOS; i++) g->adj[i] = NULL;
}

static int grafo_addNo(Grafo *g, const char *nome) {
    for (int i = 0; i < g->numNos; i++)
        if (strcmp(g->nomes[i], nome) == 0) return i;
    strcpy(g->nomes[g->numNos], nome);
    return g->numNos++;
}

static void grafo_addAresta(Grafo *g, const char *a, const char *b) {
    int ia = grafo_addNo(g, a);
    int ib = grafo_addNo(g, b);

    Aresta *e1 = (Aresta *)malloc(sizeof(Aresta));
    e1->destino = ib;
    e1->prox = g->adj[ia];
    g->adj[ia] = e1;

    Aresta *e2 = (Aresta *)malloc(sizeof(Aresta));
    e2->destino = ia;
    e2->prox = g->adj[ib];
    g->adj[ib] = e2;
}

static void montaMapaEscolaMuseu(Grafo *g) {
    grafo_init(g);
    grafo_addAresta(g, "Home", "Club");
    grafo_addAresta(g, "Home", "Bank");
    grafo_addAresta(g, "Home", "Museum");
    grafo_addAresta(g, "Club", "School");
    grafo_addAresta(g, "Club", "Bank");
    grafo_addAresta(g, "School", "Bank");
    grafo_addAresta(g, "School", "Lake");
    grafo_addAresta(g, "Bank", "Lake");
    grafo_addAresta(g, "Bank", "Park");
    grafo_addAresta(g, "Bank", "Museum");
    grafo_addAresta(g, "Lake", "Park");
    grafo_addAresta(g, "Park", "Museum");
}

static void imprimeCaminho(Grafo *g, int pai[], int origem, int destino) {
    int caminho[MAX_NOS], n = 0;
    int atual = destino;
    while (atual != -1) {
        caminho[n++] = atual;
        if (atual == origem) break;
        atual = pai[atual];
    }
    for (int i = n - 1; i >= 0; i--) {
        printf("%s", g->nomes[caminho[i]]);
        if (i > 0) printf(" -> ");
    }
    printf("\n");
}

// Retorna numero de nos visitados e preenche pai[]
static int buscaLargura(Grafo *g, int origem, int destino, int pai[], int *expandidos) {
    int visitado[MAX_NOS] = {0};
    int fila[MAX_NOS], inicio = 0, fim = 0;
    int visitados = 0;
    *expandidos = 0;

    for (int i = 0; i < MAX_NOS; i++) pai[i] = -1;

    fila[fim++] = origem;
    visitado[origem] = 1;

    while (inicio < fim) {
        int atual = fila[inicio++];
        visitados++;
        if (atual == destino) return visitados;

        (*expandidos)++;
        for (Aresta *e = g->adj[atual]; e != NULL; e = e->prox) {
            if (!visitado[e->destino]) {
                visitado[e->destino] = 1;
                pai[e->destino] = atual;
                fila[fim++] = e->destino;
            }
        }
    }
    return visitados;
}

// Busca em profundidade recursiva: retorna 1 se achou o destino
static int dfsRec(Grafo *g, int atual, int destino, int visitado[], int pai[],
                   int *visitados, int *expandidos) {
    visitado[atual] = 1;
    (*visitados)++;
    if (atual == destino) return 1;

    (*expandidos)++;
    for (Aresta *e = g->adj[atual]; e != NULL; e = e->prox) {
        if (!visitado[e->destino]) {
            pai[e->destino] = atual;
            if (dfsRec(g, e->destino, destino, visitado, pai, visitados, expandidos))
                return 1;
        }
    }
    return 0;
}

static int buscaProfundidade(Grafo *g, int origem, int destino, int pai[], int *expandidos) {
    int visitado[MAX_NOS] = {0};
    int visitados = 0;
    *expandidos = 0;
    for (int i = 0; i < MAX_NOS; i++) pai[i] = -1;

    dfsRec(g, origem, destino, visitado, pai, &visitados, expandidos);
    return visitados;
}

void run_ex8(void) {
    Grafo g;
    montaMapaEscolaMuseu(&g);

    int origem = grafo_addNo(&g, "School");
    int destino = grafo_addNo(&g, "Museum");
    int pai[MAX_NOS];
    int visitados, expandidos;
    clock_t inicio, fim;
    double tempoBFS, tempoDFS;

    printf("\n===== Questao 8: BFS x DFS (School -> Museum) =====\n\n");

    inicio = clock();
    visitados = buscaLargura(&g, origem, destino, pai, &expandidos);
    fim = clock();
    tempoBFS = (double)(fim - inicio) / CLOCKS_PER_SEC;

    printf("[BFS] Caminho: ");
    imprimeCaminho(&g, pai, origem, destino);
    printf("[BFS] Nos visitados: %d | expandidos: %d\n", visitados, expandidos);
    printf("[BFS] Tempo de execucao: %.8f s\n\n", tempoBFS);

    inicio = clock();
    visitados = buscaProfundidade(&g, origem, destino, pai, &expandidos);
    fim = clock();
    tempoDFS = (double)(fim - inicio) / CLOCKS_PER_SEC;

    printf("[DFS] Caminho: ");
    imprimeCaminho(&g, pai, origem, destino);
    printf("[DFS] Nos visitados: %d | expandidos: %d\n", visitados, expandidos);
    printf("[DFS] Tempo de execucao: %.8f s\n\n", tempoDFS);

    if (tempoBFS < tempoDFS)
        printf(">> BFS foi mais rapida nesta execucao.\n");
    else if (tempoDFS < tempoBFS)
        printf(">> DFS foi mais rapida nesta execucao.\n");
    else
        printf(">> Empate no tempo medido (grafo pequeno, diferenca imperceptivel).\n");
    printf("(Obs: grafo tem so 7 nos, entao o tempo tende a ficar perto de 0;\n"
           " o numero de nos expandidos e uma medida mais confiavel de custo.)\n");
}
