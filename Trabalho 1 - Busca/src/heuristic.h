#ifndef HEURISTIC_H
#define HEURISTIC_H

#include "graph.h"

#define HEURISTIC_DEFAULT_BONUS  10
#define HEURISTIC_DEFAULT_RADIUS 3
#define HEURISTIC_DEFAULT_LIMIT  2

/* `bonus` é quanto vale uma recompensa, em unidades de custo de terreno: o agente só compensa
   um desvio se ele custar menos que isso. Com bonus = 0 a heurística é só a distância Manhattan. */
typedef struct {
    int bonus;
    int radius;
    int reward_limit;
} HeurCfg;

HeurCfg heuristic_default_config(void);
int     heuristic_manhattan(const Graph *graph, int node_a, int node_b);

/* h'(n) = max(0, manhattan(n, goal) - atração das recompensas restantes perto de n).
   A atração de cada recompensa cresce conforme o agente chega mais perto dela (vale `bonus`
   em cima dela e uma fração dele no limite do raio), com teto de `reward_limit` recompensas.
   `parent` descreve o caminho já percorrido até n, usado para ignorar o que já foi coletado. */
long heuristic_estimate(const Graph *graph, const int *parent, int node, int goal, const HeurCfg *config);

/* Crédito já ganho: `bonus` por recompensa coletada no caminho até o nó (inclusive ele). */
long heuristic_collected_credit(const Graph *graph, const int *parent, int node, const HeurCfg *config);

#endif
