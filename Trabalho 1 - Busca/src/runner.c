#include "runner.h"

static SearchResult execute_algorithm(Algorithm algorithm, const Graph *graph, const HeurCfg *heuristic) {
    HeurCfg safe_config = *heuristic;
    safe_config.bonus = 0;

    switch (algorithm) {
    case ALGO_BFS:
        return bfs(graph, graph->start, graph->goal);
    case ALGO_DFS:
        return dfs(graph, graph->start, graph->goal);
    case ALGO_UNIFORM_COST:
        return uniform_cost(graph, graph->start, graph->goal);
    case ALGO_GREEDY:
        return greedy(graph, graph->start, graph->goal, *heuristic);
    case ALGO_ASTAR:
        return astar(graph, graph->start, graph->goal, *heuristic);
    default:
        return astar(graph, graph->start, graph->goal, safe_config);
    }
}

/* Uma única execução dura poucos microssegundos e oscila com o estado do cache.
   A primeira execução serve de aquecimento e fornece o resultado; o tempo reportado
   é a média das repetições seguintes. */
SearchResult run_algorithm(Algorithm algorithm, const Graph *graph, const HeurCfg *heuristic, int repeat) {
    SearchResult result = execute_algorithm(algorithm, graph, heuristic);
    double total_us = 0;

    for (int i = 0; i < repeat; i++) {
        SearchResult repetition = execute_algorithm(algorithm, graph, heuristic);
        total_us += repetition.time_us;
        search_result_free(&repetition);
    }
    result.time_us = total_us / repeat;
    return result;
}
