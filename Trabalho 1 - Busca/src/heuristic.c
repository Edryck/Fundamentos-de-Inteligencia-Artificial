#include "heuristic.h"

#include <stdlib.h>

HeurCfg heuristic_default_config(void) {
    HeurCfg config = {HEURISTIC_DEFAULT_BONUS, HEURISTIC_DEFAULT_RADIUS, HEURISTIC_DEFAULT_LIMIT};
    return config;
}

int heuristic_manhattan(const Graph *graph, int node_a, int node_b) {
    const Node *a = &graph->nodes[node_a];
    const Node *b = &graph->nodes[node_b];
    return abs(a->row - b->row) + abs(a->col - b->col);
}

static bool path_contains(const int *parent, int node, int target) {
    for (int step = node; step >= 0; step = parent[step]) {
        if (step == target) {
            return true;
        }
    }
    return false;
}

/* Cada recompensa ainda não coletada dentro do raio pesa mais quanto mais perto está: o peso
   cai de (raio + 1) em cima dela até 1 na borda. Sem esse gradiente, todas as células do raio
   valeriam o mesmo e nada puxaria o agente para cima da recompensa. A distância é medida na
   grade, sem considerar paredes: é uma estimativa barata do que está "por perto". */
static int remaining_reward_weight(const Graph *graph, const int *parent, int node, int radius) {
    const Node *center = &graph->nodes[node];
    int weight = 0;

    for (int row_offset = -radius; row_offset <= radius; row_offset++) {
        int col_span = radius - abs(row_offset);
        for (int col_offset = -col_span; col_offset <= col_span; col_offset++) {
            int neighbor = graph_node_at(graph, center->row + row_offset, center->col + col_offset);
            if (neighbor < 0 || !graph->nodes[neighbor].has_reward || path_contains(parent, node, neighbor)) {
                continue;
            }
            weight += radius - (abs(row_offset) + abs(col_offset)) + 1;
        }
    }
    return weight;
}

static long reward_attraction(const Graph *graph, const int *parent, int node, const HeurCfg *config) {
    int unit = config->radius + 1;
    int weight = remaining_reward_weight(graph, parent, node, config->radius);
    int cap = config->reward_limit * unit;
    if (weight > cap) {
        weight = cap;
    }
    return (long)config->bonus * weight / unit;
}

/* O teto em zero evita heurística negativa na distância até o prêmio. */
long heuristic_estimate(const Graph *graph, const int *parent, int node, int goal, const HeurCfg *config) {
    long estimate = heuristic_manhattan(graph, node, goal);
    if (config->bonus > 0) {
        estimate -= reward_attraction(graph, parent, node, config);
    }
    return estimate > 0 ? estimate : 0;
}

long heuristic_collected_credit(const Graph *graph, const int *parent, int node, const HeurCfg *config) {
    long collected = 0;
    if (config->bonus == 0) {
        return 0;
    }
    for (int step = node; step >= 0; step = parent[step]) {
        collected += graph->nodes[step].has_reward;
    }
    return collected * config->bonus;
}
