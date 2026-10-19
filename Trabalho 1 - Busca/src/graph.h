#ifndef GRAPH_H
#define GRAPH_H

#include <stdbool.h>

#include "map.h"

#define MAX_NEIGHBORS 4

typedef struct {
    int to;
    int weight;
} Edge;

typedef struct {
    int     id;
    int     row;
    int     col;
    int     entry_cost;
    bool    has_reward;
    bool    is_goal;
    bool    is_start;
    int     degree;
    Edge    edges[MAX_NEIGHBORS];
} Node;

typedef struct {
    Node *nodes;
    int   node_count;
    int   rows;
    int   cols;
    int   start;
    int   goal;
    int   reward_count;
    int  *cell_to_id;
} Graph;

typedef struct {
    int  size;
    int  rewards;
    bool has_start;
    bool has_goal;
} Component;

typedef struct {
    Component *components;
    int        component_count;
    int       *component_of;
    bool      *dead_end;
    int        dead_end_count;
    bool       goal_reachable;
} GraphAnalysis;

bool graph_build(const Map *map, Graph *graph);
void graph_free(Graph *graph);
int  graph_node_at(const Graph *graph, int row, int col);

bool graph_analyze(const Graph *graph, GraphAnalysis *analysis);
void graph_analysis_free(GraphAnalysis *analysis);

#endif
