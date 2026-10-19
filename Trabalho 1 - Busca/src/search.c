#include "search.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "metrics.h"
#include "min_heap.h"
#include "queue.h"
#include "stack.h"

#define NO_PARENT (-1)
#define UNREACHED_COST LONG_MAX
#define INITIAL_STACK_CAPACITY 64
#define ENTRY_SLOTS 2

static void exit_out_of_memory(void) {
    fprintf(stderr, "erro: sem memória durante a busca\n");
    exit(EXIT_FAILURE);
}

static void *checked_alloc(size_t count, size_t size) {
    void *block = calloc(count, size);
    if (block == NULL) {
        exit_out_of_memory();
    }
    return block;
}

static int *create_parent_table(int node_count) {
    int *parent = checked_alloc((size_t)node_count, sizeof(int));
    for (int i = 0; i < node_count; i++) {
        parent[i] = NO_PARENT;
    }
    return parent;
}

static long *create_cost_table(int node_count) {
    long *costs = checked_alloc((size_t)node_count, sizeof(long));
    for (int i = 0; i < node_count; i++) {
        costs[i] = UNREACHED_COST;
    }
    return costs;
}

/* Sobe pela tabela de pais a partir do prêmio e monta o caminho na ordem início → prêmio.
   O custo soma só os vértices em que o agente entra, então o início não conta. */
static void build_path(const Graph *graph, const int *parent, int start, int goal, SearchResult *result) {
    int length = 1;
    for (int node = goal; node != start; node = parent[node]) {
        length++;
    }

    result->path = checked_alloc((size_t)length, sizeof(int));
    result->path_len = length;
    int node = goal;
    for (int i = length - 1; i >= 0; i--) {
        result->path[i] = node;
        if (node != start) {
            result->cost += graph->nodes[node].entry_cost;
        }
        if (graph->nodes[node].has_reward) {
            result->rewards_collected++;
        }
        node = parent[node];
    }
    result->found = true;
}

/* `expansion_order` guarda, por vértice, em que posição ele foi expandido (0 = nunca). Serve para
   desenhar o que cada algoritmo explorou. */
static SearchResult create_result(int node_count) {
    SearchResult result = {0};
    result.expansion_order = checked_alloc((size_t)node_count, sizeof(int));
    return result;
}

/* Conta o nó como expandido ao sair da fronteira. Recompensas vistas são as dos nós expandidos. */
static void register_expansion(const Graph *graph, int node, SearchResult *result) {
    result->expanded++;
    result->expansion_order[node] = result->expanded;
    if (graph->nodes[node].has_reward) {
        result->rewards_seen++;
    }
}

/* Mede o uso de memória da fronteira: quantas entradas já foram criadas e o maior número
   que ficou guardado ao mesmo tempo. Entradas repetidas (DFS, A*) também ocupam memória. */
static void register_insertion(SearchResult *result, int frontier_size) {
    result->generated++;
    if (frontier_size > result->peak_frontier) {
        result->peak_frontier = frontier_size;
    }
}

SearchResult bfs(const Graph *graph, int start, int goal) {
    SearchResult result = create_result(graph->node_count);
    double began_at = metrics_now_us();

    int *parent = create_parent_table(graph->node_count);
    bool *visited = checked_alloc((size_t)graph->node_count, sizeof(bool));
    Queue frontier;
    if (!queue_init(&frontier, graph->node_count)) {
        exit_out_of_memory();
    }

    /* Marcar ao enfileirar impede o mesmo vértice de entrar duas vezes na fila. */
    visited[start] = true;
    queue_push(&frontier, start);
    register_insertion(&result, frontier.size);
    while (!queue_is_empty(&frontier)) {
        int current = queue_pop(&frontier);
        register_expansion(graph, current, &result);
        if (current == goal) {
            build_path(graph, parent, start, goal, &result);
            break;
        }
        for (int e = 0; e < graph->nodes[current].degree; e++) {
            int neighbor = graph->nodes[current].edges[e].to;
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                parent[neighbor] = current;
                queue_push(&frontier, neighbor);
                register_insertion(&result, frontier.size);
            }
        }
    }

    queue_free(&frontier);
    free(visited);
    free(parent);
    result.time_us = metrics_now_us() - began_at;
    return result;
}

static void push_or_exit(Stack *stack, int value) {
    if (!stack_push(stack, value)) {
        exit_out_of_memory();
    }
}

/* Cada entrada da pilha guarda o vértice e quem o descobriu (nessa ordem, empilhados em par).
   O vértice só é marcado como visitado ao sair da pilha: marcar ao empilhar faria o caminho
   deixar de ser o de uma busca em profundidade de verdade. */
SearchResult dfs(const Graph *graph, int start, int goal) {
    SearchResult result = create_result(graph->node_count);
    double began_at = metrics_now_us();

    int *parent = create_parent_table(graph->node_count);
    bool *visited = checked_alloc((size_t)graph->node_count, sizeof(bool));
    Stack frontier;
    if (!stack_init(&frontier, INITIAL_STACK_CAPACITY)) {
        exit_out_of_memory();
    }

    push_or_exit(&frontier, start);
    push_or_exit(&frontier, NO_PARENT);
    register_insertion(&result, frontier.size / ENTRY_SLOTS);
    while (!stack_is_empty(&frontier)) {
        int discovered_by = stack_pop(&frontier);
        int current = stack_pop(&frontier);
        if (visited[current]) {
            continue;
        }
        visited[current] = true;
        parent[current] = discovered_by;
        register_expansion(graph, current, &result);
        if (current == goal) {
            build_path(graph, parent, start, goal, &result);
            break;
        }
        /* Empilha na ordem inversa para o primeiro vizinho ser o primeiro a ser explorado. */
        for (int e = graph->nodes[current].degree - 1; e >= 0; e--) {
            int neighbor = graph->nodes[current].edges[e].to;
            if (!visited[neighbor]) {
                push_or_exit(&frontier, neighbor);
                push_or_exit(&frontier, current);
                register_insertion(&result, frontier.size / ENTRY_SLOTS);
            }
        }
    }

    stack_free(&frontier);
    free(visited);
    free(parent);
    result.time_us = metrics_now_us() - began_at;
    return result;
}

static void push_node_or_exit(MinHeap *heap, int node, long key, long tie_break) {
    if (!min_heap_push(heap, node, key, tie_break)) {
        exit_out_of_memory();
    }
}

/* Busca de custo uniforme (Dijkstra com parada no prêmio): expande sempre o vértice de menor custo
   acumulado. É a referência de otimalidade em grafo ponderado, a que o A* seguro chega expandindo menos.
   O custo de uma entrada antiga fica no heap depois que o vértice melhora e é descartado ao sair. */
SearchResult uniform_cost(const Graph *graph, int start, int goal) {
    SearchResult result = create_result(graph->node_count);
    double began_at = metrics_now_us();

    int *parent = create_parent_table(graph->node_count);
    long *cost_so_far = create_cost_table(graph->node_count);
    bool *closed = checked_alloc((size_t)graph->node_count, sizeof(bool));
    MinHeap frontier;
    if (!min_heap_init(&frontier, graph->node_count)) {
        exit_out_of_memory();
    }

    cost_so_far[start] = 0;
    push_node_or_exit(&frontier, start, 0, 0);
    register_insertion(&result, frontier.size);
    while (!min_heap_is_empty(&frontier)) {
        int current = min_heap_pop(&frontier).node;
        if (closed[current]) {
            continue;
        }
        closed[current] = true;
        register_expansion(graph, current, &result);
        if (current == goal) {
            build_path(graph, parent, start, goal, &result);
            break;
        }
        for (int e = 0; e < graph->nodes[current].degree; e++) {
            Edge edge = graph->nodes[current].edges[e];
            long new_cost = cost_so_far[current] + edge.weight;
            if (!closed[edge.to] && new_cost < cost_so_far[edge.to]) {
                cost_so_far[edge.to] = new_cost;
                parent[edge.to] = current;
                push_node_or_exit(&frontier, edge.to, new_cost, 0);
                register_insertion(&result, frontier.size);
            }
        }
    }

    min_heap_free(&frontier);
    free(closed);
    free(cost_so_far);
    free(parent);
    result.time_us = metrics_now_us() - began_at;
    return result;
}

/* A prioridade é h'(n) menos o crédito das recompensas já pegas no caminho: sem esse desconto,
   pisar numa recompensa faria a atração dela sumir e o agente não teria motivo para ir até lá. */
static long greedy_priority(const Graph *graph, const int *parent, int node, int goal, const HeurCfg *config) {
    return heuristic_estimate(graph, parent, node, goal, config) -
           heuristic_collected_credit(graph, parent, node, config);
}

/* A fronteira é ordenada só pela prioridade acima. Cada vértice é marcado ao ser descoberto e nunca
   reaberto: a busca gulosa não olha o custo já pago, então um segundo caminho não a faria mudar de ideia. */
SearchResult greedy(const Graph *graph, int start, int goal, HeurCfg config) {
    SearchResult result = create_result(graph->node_count);
    double began_at = metrics_now_us();

    int *parent = create_parent_table(graph->node_count);
    bool *discovered = checked_alloc((size_t)graph->node_count, sizeof(bool));
    MinHeap frontier;
    if (!min_heap_init(&frontier, graph->node_count)) {
        exit_out_of_memory();
    }

    discovered[start] = true;
    push_node_or_exit(&frontier, start, greedy_priority(graph, parent, start, goal, &config), 0);
    register_insertion(&result, frontier.size);
    while (!min_heap_is_empty(&frontier)) {
        int current = min_heap_pop(&frontier).node;
        register_expansion(graph, current, &result);
        if (current == goal) {
            build_path(graph, parent, start, goal, &result);
            break;
        }
        for (int e = 0; e < graph->nodes[current].degree; e++) {
            int neighbor = graph->nodes[current].edges[e].to;
            if (!discovered[neighbor]) {
                discovered[neighbor] = true;
                parent[neighbor] = current;
                push_node_or_exit(&frontier, neighbor, greedy_priority(graph, parent, neighbor, goal, &config), 0);
                register_insertion(&result, frontier.size);
            }
        }
    }

    min_heap_free(&frontier);
    free(discovered);
    free(parent);
    result.time_us = metrics_now_us() - began_at;
    return result;
}

typedef struct {
    int   *parent;
    long  *cost_so_far;
    long  *best_priority;
    long  *expanded_at_cost;
} AstarTables;

static AstarTables create_astar_tables(int node_count) {
    AstarTables tables;
    tables.parent = create_parent_table(node_count);
    tables.cost_so_far = create_cost_table(node_count);
    tables.best_priority = create_cost_table(node_count);
    tables.expanded_at_cost = create_cost_table(node_count);
    return tables;
}

static void free_astar_tables(AstarTables *tables) {
    free(tables->parent);
    free(tables->cost_so_far);
    free(tables->best_priority);
    free(tables->expanded_at_cost);
}

/* Reabrir nós é necessário porque, com bônus de recompensas, h' deixa de ser consistente:
   um nó já expandido pode ser alcançado depois por um caminho mais barato.
   Com bônus zero (A* clássico) o heap nunca devolve um nó melhorado e o comportamento é o usual. */
static bool path_contains(const int *parent, int node, int target) {
    for (int step = node; step >= 0; step = parent[step]) {
        if (step == target) {
            return true;
        }
    }
    return false;
}

/* Com bônus, o custo acumulado é líquido: pegar uma recompensa nova abate `bonus` do custo, então
   um desvio é aceito quando custa menos que a recompensa. Para a recompensa só contar uma vez e a
   árvore de pais não formar ciclos, o caminho nunca passa duas vezes pelo mesmo vértice. */
static void relax_neighbors(const Graph *graph, int current, int goal, const HeurCfg *config,
                            AstarTables *tables, MinHeap *frontier, SearchResult *result) {
    for (int e = 0; e < graph->nodes[current].degree; e++) {
        Edge edge = graph->nodes[current].edges[e];
        long step_cost = edge.weight;
        if (config->bonus > 0) {
            if (path_contains(tables->parent, current, edge.to)) {
                continue;
            }
            step_cost -= graph->nodes[edge.to].has_reward ? config->bonus : 0;
        }
        long new_cost = tables->cost_so_far[current] + step_cost;
        if (new_cost >= tables->cost_so_far[edge.to]) {
            continue;
        }
        tables->cost_so_far[edge.to] = new_cost;
        tables->parent[edge.to] = current;

        long estimate = heuristic_estimate(graph, tables->parent, edge.to, goal, config);
        tables->best_priority[edge.to] = new_cost + estimate;
        push_node_or_exit(frontier, edge.to, new_cost + estimate, estimate);
        register_insertion(result, frontier->size);
    }
}

/* f(n) = g(n) + h'(n), desempatando pelo menor h' (o nó mais perto do prêmio sai primeiro).
   Entradas antigas de um nó que já melhorou ficam no heap e são descartadas ao sair (remoção preguiçosa). */
SearchResult astar(const Graph *graph, int start, int goal, HeurCfg config) {
    SearchResult result = create_result(graph->node_count);
    double began_at = metrics_now_us();

    AstarTables tables = create_astar_tables(graph->node_count);
    MinHeap frontier;
    if (!min_heap_init(&frontier, graph->node_count)) {
        exit_out_of_memory();
    }

    long start_estimate = heuristic_estimate(graph, tables.parent, start, goal, &config);
    tables.cost_so_far[start] = 0;
    tables.best_priority[start] = start_estimate;
    push_node_or_exit(&frontier, start, start_estimate, start_estimate);
    register_insertion(&result, frontier.size);

    while (!min_heap_is_empty(&frontier)) {
        HeapItem item = min_heap_pop(&frontier);
        int current = item.node;
        bool outdated = item.key != tables.best_priority[current];
        bool already_expanded_at_this_cost = tables.expanded_at_cost[current] == tables.cost_so_far[current];
        if (outdated || already_expanded_at_this_cost) {
            continue;
        }
        if (tables.expanded_at_cost[current] == UNREACHED_COST) {
            register_expansion(graph, current, &result);
        } else {
            result.expanded++;
        }
        tables.expanded_at_cost[current] = tables.cost_so_far[current];
        if (current == goal) {
            build_path(graph, tables.parent, start, goal, &result);
            break;
        }
        relax_neighbors(graph, current, goal, &config, &tables, &frontier, &result);
    }

    min_heap_free(&frontier);
    free_astar_tables(&tables);
    result.time_us = metrics_now_us() - began_at;
    return result;
}

void search_result_free(SearchResult *result) {
    free(result->path);
    free(result->expansion_order);
    result->expansion_order = NULL;
    result->path = NULL;
    result->path_len = 0;
}
