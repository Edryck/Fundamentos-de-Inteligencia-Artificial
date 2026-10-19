#ifndef SEARCH_H
#define SEARCH_H

#include <stdbool.h>

#include "graph.h"
#include "heuristic.h"

typedef struct {
    int   *path;
    int   *expansion_order;
    int    path_len;
    long   cost;
    int    expanded;
    int    generated;
    int    peak_frontier;
    int    rewards_collected;
    int    rewards_seen;
    double time_us;
    bool   found;
} SearchResult;

SearchResult bfs(const Graph *graph, int start, int goal);
SearchResult dfs(const Graph *graph, int start, int goal);
SearchResult uniform_cost(const Graph *graph, int start, int goal);
SearchResult greedy(const Graph *graph, int start, int goal, HeurCfg config);
SearchResult astar(const Graph *graph, int start, int goal, HeurCfg config);

void search_result_free(SearchResult *result);

#endif
