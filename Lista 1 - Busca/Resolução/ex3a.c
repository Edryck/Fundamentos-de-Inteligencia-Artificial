
#include "ex3a.h"

#define NUM_REGIOES 7
#define NUM_CORES 4

// Mapa de exemplo (estilo Australia): cada indice e uma regiao.
// 0=WA 1=NT 2=SA 3=Q 4=NSW 5=V 6=T (Tasmania nao faz fronteira com ninguem) 
static const char *nomesRegioes[NUM_REGIOES] = {
    "WA", "NT", "SA", "Q", "NSW", "V", "T"
};

static const char *nomesCores[NUM_CORES] = {
    "Vermelho", "Verde", "Azul", "Amarelo"
};

// matriz de adjacencia: 1 se as regioes fazem fronteira 
static int adjacente[NUM_REGIOES][NUM_REGIOES] = {
    //        WA NT SA Q  NSW V  T 
    // WA    {0, 1, 1, 0, 0, 0, 0},
    // NT    {1, 0, 1, 1, 0, 0, 0},
    // SA    {1, 1, 0, 1, 1, 1, 0},
    // Q     {0, 1, 1, 0, 1, 0, 0},
    // NSW   {0, 0, 1, 1, 0, 1, 0},
    // V     {0, 0, 1, 0, 1, 0, 0},
    // T     {0, 0, 0, 0, 0, 0, 0}
};

static int cores[NUM_REGIOES];   // estado: cor atribuida a cada regiao (-1 = sem cor) 
static int tentativas;           // conta quantos "nos" a busca expandiu 

// funcao-sucessor / teste de validade: a cor pode ser usada nessa regiao? 
static int corValida(int regiao, int cor) {
    for (int vizinho = 0; vizinho < NUM_REGIOES; vizinho++) {
        if (adjacente[regiao][vizinho] && cores[vizinho] == cor)
            return 0;
    }
    return 1;
}

// busca com backtracking: tenta colorir a regiao atual e chama
// recursivamente para a proxima; se travar, desfaz (backtrack). 
static int colorir(int regiao) {
    if (regiao == NUM_REGIOES) return 1; // teste de objetivo: todas coloridas 

    for (int cor = 0; cor < NUM_CORES; cor++) {
        tentativas++;
        if (corValida(regiao, cor)) {
            cores[regiao] = cor;
            if (colorir(regiao + 1)) return 1;
            cores[regiao] = -1; // backtrack 
        }
    }
    return 0;
}

void run_ex3a(void) {
    for (int i = 0; i < NUM_REGIOES; i++) cores[i] = -1;
    tentativas = 0;

    printf("\n===== Questao 3a: Coloracao de mapa (4 cores) =====\n\n");

    clock_t inicio = clock();
    int achou = colorir(0);
    clock_t fim = clock();

    if (achou) {
        printf("Solucao encontrada:\n");
        for (int i = 0; i < NUM_REGIOES; i++)
            printf("  %-4s -> %s\n", nomesRegioes[i], nomesCores[cores[i]]);
    } else {
        printf("Nao foi possivel colorir o mapa com %d cores.\n", NUM_CORES);
    }

    printf("\nTentativas de atribuicao de cor (nos expandidos): %d\n", tentativas);
    printf("Tempo de execucao: %.8f s\n", (double)(fim - inicio) / CLOCKS_PER_SEC);
}
