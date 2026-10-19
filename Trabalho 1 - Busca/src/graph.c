#include "graph.h"

#include <stdlib.h>
#include <string.h>

#include "queue.h"

static const int ROW_STEP[MAX_NEIGHBORS] = {-1, 1, 0, 0};
static const int COL_STEP[MAX_NEIGHBORS] = {0, 0, -1, 1};

int graph_node_at(const Graph *graph, int row, int col) {
    if (row < 0 || row >= graph->rows || col < 0 || col >= graph->cols) {
        return -1;
    }
    return graph->cell_to_id[row * graph->cols + col];
}

static int assign_node_ids(const Map *map, int *cell_to_id) {
    int count = 0;
    for (int row = 0; row < map->rows; row++) {
        for (int col = 0; col < map->cols; col++) {
            bool walkable = cell_is_walkable(map->cells[row][col]);
            cell_to_id[row * map->cols + col] = walkable ? count++ : -1;
        }
    }
    return count;
}

static void fill_node(Graph *graph, const Map *map, int row, int col) {
    char cell = map->cells[row][col];
    Node *node = &graph->nodes[graph_node_at(graph, row, col)];

    node->id = graph_node_at(graph, row, col);
    node->row = row;
    node->col = col;
    node->entry_cost = cell_entry_cost(cell);
    node->has_reward = (cell == CELL_REWARD);
    node->is_start = (cell == CELL_START);
    node->is_goal = (cell == CELL_GOAL);
    node->degree = 0;

    if (node->has_reward) {
        graph->reward_count++;
    }
    if (node->is_start) {
        graph->start = node->id;
    }
    if (node->is_goal) {
        graph->goal = node->id;
    }
}

/* O peso da aresta é o custo de entrar no vértice de destino, como define a spec. */
static void connect_node(Graph *graph, int node_id) {
    Node *node = &graph->nodes[node_id];
    for (int d = 0; d < MAX_NEIGHBORS; d++) {
        int neighbor = graph_node_at(graph, node->row + ROW_STEP[d], node->col + COL_STEP[d]);
        if (neighbor >= 0) {
            node->edges[node->degree].to = neighbor;
            node->edges[node->degree].weight = graph->nodes[neighbor].entry_cost;
            node->degree++;
        }
    }
}

bool graph_build(const Map *map, Graph *graph) {
    memset(graph, 0, sizeof *graph);
    graph->rows = map->rows;
    graph->cols = map->cols;
    graph->start = -1;
    graph->goal = -1;

    graph->cell_to_id = malloc((size_t)map->rows * (size_t)map->cols * sizeof(int));
    if (graph->cell_to_id == NULL) {
        return false;
    }
    graph->node_count = assign_node_ids(map, graph->cell_to_id);
    graph->nodes = calloc((size_t)graph->node_count, sizeof(Node));
    if (graph->nodes == NULL) {
        graph_free(graph);
        return false;
    }

    for (int row = 0; row < map->rows; row++) {
        for (int col = 0; col < map->cols; col++) {
            if (graph_node_at(graph, row, col) >= 0) {
                fill_node(graph, map, row, col);
            }
        }
    }
    for (int id = 0; id < graph->node_count; id++) {
        connect_node(graph, id);
    }
    return true;
}

void graph_free(Graph *graph) {
    free(graph->nodes);
    free(graph->cell_to_id);
    memset(graph, 0, sizeof *graph);
}

/* ---------- análise de conectividade ---------- */

static void explore_component(const Graph *graph, GraphAnalysis *analysis, Queue *queue,
                              int first_node, int component_id) {
    Component *component = &analysis->components[component_id];

    analysis->component_of[first_node] = component_id;
    queue_push(queue, first_node);
    while (!queue_is_empty(queue)) {
        const Node *node = &graph->nodes[queue_pop(queue)];
        component->size++;
        component->rewards += node->has_reward;
        component->has_start = component->has_start || node->is_start;
        component->has_goal = component->has_goal || node->is_goal;

        for (int e = 0; e < node->degree; e++) {
            int neighbor = node->edges[e].to;
            if (analysis->component_of[neighbor] < 0) {
                analysis->component_of[neighbor] = component_id;
                queue_push(queue, neighbor);
            }
        }
    }
}

static bool find_components(const Graph *graph, GraphAnalysis *analysis) {
    Queue queue;
    if (!queue_init(&queue, graph->node_count)) {
        return false;
    }
    for (int id = 0; id < graph->node_count; id++) {
        analysis->component_of[id] = -1;
    }
    for (int id = 0; id < graph->node_count; id++) {
        if (analysis->component_of[id] < 0) {
            explore_component(graph, analysis, &queue, id, analysis->component_count);
            analysis->component_count++;
        }
    }
    queue_free(&queue);
    return true;
}

static bool is_protected(const Node *node) {
    return node->is_start || node->is_goal;
}

/* Remove folhas repetidamente: o que sobra de ramos pendurados no grafo é beco
   sem saída, inclusive corredores compridos e não só a última célula.
   Início e prêmio nunca são removidos porque o agente precisa estar neles. */
static bool find_dead_ends(const Graph *graph, GraphAnalysis *analysis) {
    int *remaining = malloc((size_t)graph->node_count * sizeof(int));
    Queue queue;
    if (remaining == NULL) {
        return false;
    }
    if (!queue_init(&queue, graph->node_count)) {
        free(remaining);
        return false;
    }

    for (int id = 0; id < graph->node_count; id++) {
        remaining[id] = graph->nodes[id].degree;
        if (remaining[id] == 1 && !is_protected(&graph->nodes[id])) {
            queue_push(&queue, id);
        }
    }
    while (!queue_is_empty(&queue)) {
        int id = queue_pop(&queue);
        analysis->dead_end[id] = true;
        analysis->dead_end_count++;
        for (int e = 0; e < graph->nodes[id].degree; e++) {
            int neighbor = graph->nodes[id].edges[e].to;
            if (analysis->dead_end[neighbor]) {
                continue;
            }
            remaining[neighbor]--;
            if (remaining[neighbor] == 1 && !is_protected(&graph->nodes[neighbor])) {
                queue_push(&queue, neighbor);
            }
        }
    }
    queue_free(&queue);
    free(remaining);
    return true;
}

bool graph_analyze(const Graph *graph, GraphAnalysis *analysis) {
    memset(analysis, 0, sizeof *analysis);
    analysis->components = calloc((size_t)graph->node_count, sizeof(Component));
    analysis->component_of = malloc((size_t)graph->node_count * sizeof(int));
    analysis->dead_end = calloc((size_t)graph->node_count, sizeof(bool));

    bool ok = analysis->components != NULL && analysis->component_of != NULL &&
              analysis->dead_end != NULL && find_components(graph, analysis) &&
              find_dead_ends(graph, analysis);
    if (!ok) {
        graph_analysis_free(analysis);
        return false;
    }
    analysis->goal_reachable = analysis->component_of[graph->start] == analysis->component_of[graph->goal];
    return true;
}

void graph_analysis_free(GraphAnalysis *analysis) {
    free(analysis->components);
    free(analysis->component_of);
    free(analysis->dead_end);
    memset(analysis, 0, sizeof *analysis);
}
