#include "ex9.h"

#define MAX_NOS 15
#define MAX_NOME 20

typedef struct Aresta {
    int destino;
    struct Aresta *prox;
} Aresta;

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

// Apenas as cidades "do lado de Arad" (antes de Bucareste)
static void montaMapaRomenia(Grafo *g) {
    grafo_init(g);
    grafo_addAresta(g, "Oradea", "Zerind");
    grafo_addAresta(g, "Oradea", "Sibiu");
    grafo_addAresta(g, "Zerind", "Arad");
    grafo_addAresta(g, "Arad", "Sibiu");
    grafo_addAresta(g, "Arad", "Timisoara");
    grafo_addAresta(g, "Timisoara", "Lugoj");
    grafo_addAresta(g, "Lugoj", "Mehadia");
    grafo_addAresta(g, "Mehadia", "Dobreta");
    grafo_addAresta(g, "Dobreta", "Craiova");
    grafo_addAresta(g, "Sibiu", "Fagaras");
    grafo_addAresta(g, "Sibiu", "RimnicuVilcea");
    grafo_addAresta(g, "RimnicuVilcea", "Pitesti");
    grafo_addAresta(g, "RimnicuVilcea", "Craiova");
    grafo_addAresta(g, "Fagaras", "Bucareste");
    grafo_addAresta(g, "Pitesti", "Bucareste");
    grafo_addAresta(g, "Pitesti", "Craiova");
}

static void imprimeCaminho(Grafo *g, int caminho[], int tam) {
    for (int i = 0; i < tam; i++) {
        printf("%s", g->nomes[caminho[i]]);
        if (i < tam - 1) printf(" -> ");
    }
    printf("\n");
}

// Busca em profundidade limitada (recursiva).
// Retorna 1 se encontrou o destino dentro do limite. 
static int limitadaRec(Grafo *g, int atual, int destino, int limite,
                        int visitado[], int caminho[], int *tamCaminho,
                        int *expandidos) {
    visitado[atual] = 1;
    caminho[(*tamCaminho)++] = atual;

    if (atual == destino) return 1;
    if (limite == 0) { (*tamCaminho)--; return 0; }

    (*expandidos)++;
    for (Aresta *e = g->adj[atual]; e != NULL; e = e->prox) {
        if (!visitado[e->destino]) {
            if (limitadaRec(g, e->destino, destino, limite - 1,
                             visitado, caminho, tamCaminho, expandidos))
                return 1;
        }
    }
    (*tamCaminho)--;
    visitado[atual] = 0; // libera para outros ramos tentarem passar por aqui 
    return 0;
}

static int buscaProfundidadeLimitada(Grafo *g, int origem, int destino, int limite,
                                      int caminho[], int *tamCaminho, int *expandidos) {
    int visitado[MAX_NOS] = {0};
    *tamCaminho = 0;
    *expandidos = 0;
    return limitadaRec(g, origem, destino, limite, visitado, caminho, tamCaminho, expandidos);
}

// Aprofundamento iterativo: chama a busca limitada com limite = 0,1,2,...
// ate encontrar o destino ou atingir limiteMax. 
static int buscaIterativa(Grafo *g, int origem, int destino, int limiteMax,
                           int caminho[], int *tamCaminho, int *expandidosTotal,
                           int *limiteEncontrado) {
    *expandidosTotal = 0;
    for (int l = 0; l <= limiteMax; l++) {
        int expandidos;
        if (buscaProfundidadeLimitada(g, origem, destino, l, caminho, tamCaminho, &expandidos)) {
            *expandidosTotal += expandidos;
            *limiteEncontrado = l;
            return 1;
        }
        *expandidosTotal += expandidos;
    }
    *limiteEncontrado = -1;
    return 0;
}

static void testaLimite(Grafo *g, int origem, int destino, int limite) {
    int caminho[MAX_NOS], tamCaminho, expandidos;
    clock_t inicio, fim;

    printf("--- Limite = %d ---\n", limite);

    // Profundidade limitada isolada 
    inicio = clock();
    int achou = buscaProfundidadeLimitada(g, origem, destino, limite, caminho, &tamCaminho, &expandidos);
    fim = clock();
    printf("[Prof. Limitada] ");
    if (achou) {
        printf("Caminho: ");
        imprimeCaminho(g, caminho, tamCaminho);
    } else {
        printf("Falhou (nao encontrou dentro do limite)\n");
    }
    printf("[Prof. Limitada] Expandidos: %d | Tempo: %.8f s\n",
           expandidos, (double)(fim - inicio) / CLOCKS_PER_SEC);

    // Aprofundamento iterativo ate esse mesmo limite maximo 
    int expandidosTotal, limiteEncontrado;
    inicio = clock();
    achou = buscaIterativa(g, origem, destino, limite, caminho, &tamCaminho, &expandidosTotal, &limiteEncontrado);
    fim = clock();
    printf("[Aprof. Iterativo] ");
    if (achou) {
        printf("Caminho: ");
        imprimeCaminho(g, caminho, tamCaminho);
        printf("[Aprof. Iterativo] Encontrado no limite %d\n", limiteEncontrado);
    } else {
        printf("Falhou ate o limite maximo testado\n");
    }
    printf("[Aprof. Iterativo] Expandidos (somando todas as iteracoes): %d | Tempo: %.8f s\n\n",
           expandidosTotal, (double)(fim - inicio) / CLOCKS_PER_SEC);
}

void run_ex9(void) {
    Grafo g;
    montaMapaRomenia(&g);

    int origem = grafo_addNo(&g, "Arad");
    int destino = grafo_addNo(&g, "Bucareste");

    printf("\n===== Questao 9: Profundidade Limitada x Aprofundamento Iterativo =====\n");
    printf("(Arad -> Bucareste, mapa reduzido ao lado oeste da Romenia)\n\n");

    testaLimite(&g, origem, destino, 2);
    testaLimite(&g, origem, destino, 4);
    testaLimite(&g, origem, destino, 7);

    printf(">> Repare que com limite 2 ambos falham (Bucareste esta a 3 arestas\n"
           "   de distancia no minimo). A partir do limite 3 ja e possivel achar\n"
           "   o caminho Arad-Sibiu-Fagaras-Bucareste.\n");
}
