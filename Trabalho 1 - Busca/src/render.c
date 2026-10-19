#include "render.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define ANSI_RESET "\033[0m"
#define ANSI_HIDE_CURSOR "\033[?25l"
#define ANSI_SHOW_CURSOR "\033[?25h"
#define ANSI_CLEAR_LINE "\033[K"
#define ANSI_LINES_UP_FORMAT "\033[%dF"

#define BG_SOLID "\033[48;5;22m"
#define BG_SAND  "\033[48;5;222m"
#define BG_ROCK  "\033[48;5;240m"
#define BG_SWAMP "\033[48;5;58m"
#define BG_WALL  "\033[48;5;16m"
#define BG_TRAIL "\033[48;5;238m"
#define BG_AGENT "\033[44m"

#define FG_AGENT  "\033[1;97m"
#define FG_GOAL   "\033[1;93m"
#define FG_REWARD "\033[1;92m"
#define FG_EXPLORED "\033[96m"
#define FG_ALERT  "\033[1;91m"

#define SYMBOL_AGENT "☺"
#define SYMBOL_GOAL  "★"
#define SYMBOL_TRAIL "·"
#define SYMBOL_EXPLORED "•"

#define ANIMATION_DELAY_MS 120
#define MAX_LISTED_DEAD_ENDS 30
#define NANOS_PER_MILLI    1000000L
#define MILLIS_PER_SECOND  1000L

typedef struct {
    const Map          *map;
    const Graph        *graph;
    const SearchResult *result;
    bool               *on_trail;
    const bool         *explored;
} Scene;

static const char *terrain_background(char cell) {
    switch (cell) {
    case CELL_SAND:
        return BG_SAND;
    case CELL_ROCK:
        return BG_ROCK;
    case CELL_SWAMP:
        return BG_SWAMP;
    case CELL_WALL:
        return BG_WALL;
    default:
        return BG_SOLID;
    }
}

/* Cada célula ocupa duas colunas para o mapa não ficar esticado na vertical. */
static void print_symbol_cell(const char *background, const char *foreground, const char *symbol) {
    printf("%s%s%s %s", background, foreground, symbol, ANSI_RESET);
}

static void print_cell(const Map *map, const bool *on_trail, const bool *explored, int row, int col,
                       bool agent_here) {
    char cell = map->cells[row][col];

    if (agent_here) {
        print_symbol_cell(BG_AGENT, FG_AGENT, SYMBOL_AGENT);
    } else if (cell == CELL_GOAL) {
        print_symbol_cell(BG_SOLID, FG_GOAL, SYMBOL_GOAL);
    } else if (on_trail != NULL && on_trail[row * map->cols + col]) {
        print_symbol_cell(BG_TRAIL, "", SYMBOL_TRAIL);
    } else if (cell == CELL_REWARD) {
        print_symbol_cell(BG_SOLID, FG_REWARD, "$");
    } else if (explored != NULL && explored[row * map->cols + col]) {
        print_symbol_cell(terrain_background(cell), FG_EXPLORED, SYMBOL_EXPLORED);
    } else {
        printf("%s  %s", terrain_background(cell), ANSI_RESET);
    }
}

void render_legend(void) {
    printf("Legenda: ");
    print_symbol_cell(BG_AGENT, FG_AGENT, SYMBOL_AGENT);
    printf(" agente  ");
    print_symbol_cell(BG_SOLID, FG_GOAL, SYMBOL_GOAL);
    printf(" prêmio  ");
    print_symbol_cell(BG_SOLID, FG_REWARD, "$");
    printf(" recompensa  ");
    print_symbol_cell(BG_TRAIL, "", SYMBOL_TRAIL);
    printf(" rastro  ");
    print_symbol_cell(BG_SOLID, FG_EXPLORED, SYMBOL_EXPLORED);
    printf(" expandido (--explored)\n");
    printf("         %s  %s sólido (1)  %s  %s arenoso (4)  %s  %s rochoso (10)  %s  %s pântano (20)  %s  %s parede\n",
           BG_SOLID, ANSI_RESET, BG_SAND, ANSI_RESET, BG_ROCK, ANSI_RESET, BG_SWAMP, ANSI_RESET,
           BG_WALL, ANSI_RESET);
}

static void print_grid(const Map *map, const bool *on_trail, const bool *explored, int agent_row,
                       int agent_col) {
    for (int row = 0; row < map->rows; row++) {
        for (int col = 0; col < map->cols; col++) {
            print_cell(map, on_trail, explored, row, col, row == agent_row && col == agent_col);
        }
        printf("%s\n", ANSI_CLEAR_LINE);
    }
}

void render_map(const Map *map) {
    int start_row = 0;
    int start_col = 0;
    map_find_symbol(map, CELL_START, &start_row, &start_col);
    print_grid(map, NULL, NULL, start_row, start_col);
}

static void print_dead_ends(const Graph *graph, const GraphAnalysis *analysis) {
    if (analysis->dead_end_count == 0) {
        return;
    }
    printf("  becos sem saída (linha,coluna):");
    int listed = 0;
    for (int id = 0; id < graph->node_count && listed < MAX_LISTED_DEAD_ENDS; id++) {
        if (analysis->dead_end[id]) {
            printf(" %d,%d", graph->nodes[id].row, graph->nodes[id].col);
            listed++;
        }
    }
    if (analysis->dead_end_count > listed) {
        printf(" ... (+%d)", analysis->dead_end_count - listed);
    }
    printf("\n");
}

static void print_component(int index, const Component *component) {
    const char *kind = (component->has_start || component->has_goal) ? "" : " (sala isolada, sem acesso do agente)";
    printf("  componente %d: %d vértices, %d recompensas%s%s%s\n", index, component->size, component->rewards,
           component->has_start ? " [início]" : "", component->has_goal ? " [prêmio]" : "", kind);
}

void render_analysis(const Graph *graph, const GraphAnalysis *analysis) {
    const Node *start = &graph->nodes[graph->start];
    const Node *goal = &graph->nodes[graph->goal];

    printf("\nvértices: %d | recompensas: %d | componentes: %d | células de beco: %d\n", graph->node_count,
           graph->reward_count, analysis->component_count, analysis->dead_end_count);
    printf("início (linha,coluna): %d,%d | prêmio: %d,%d\n", start->row, start->col, goal->row, goal->col);
    if (analysis->component_count > 1 || !analysis->goal_reachable) {
        for (int i = 0; i < analysis->component_count; i++) {
            print_component(i, &analysis->components[i]);
        }
    }
    print_dead_ends(graph, analysis);
    if (!analysis->goal_reachable) {
        printf("%sSEM CAMINHO ATÉ O PRÊMIO%s\n", FG_ALERT, ANSI_RESET);
    }
}

static void sleep_milliseconds(long milliseconds) {
    struct timespec pause = {milliseconds / MILLIS_PER_SECOND, (milliseconds % MILLIS_PER_SECOND) * NANOS_PER_MILLI};
    nanosleep(&pause, NULL);
}

/* O quadro tem uma linha de título, as linhas do mapa e uma de status. */
static int frame_height(const Map *map) {
    return map->rows + 2;
}

static void print_frame(const Scene *scene, const char *title, int step, long cost, int rewards) {
    const Node *agent = &scene->graph->nodes[scene->result->path[step]];

    printf("%s%s\n", title, ANSI_CLEAR_LINE);
    print_grid(scene->map, scene->on_trail, scene->explored, agent->row, agent->col);
    printf("passo %d/%d | custo %ld | recompensas %d%s\n", step, scene->result->path_len - 1, cost, rewards,
           ANSI_CLEAR_LINE);
}

static void mark_trail(const Scene *scene, int node_id) {
    const Node *node = &scene->graph->nodes[node_id];
    scene->on_trail[node->row * scene->map->cols + node->col] = true;
}

static void animate_route(const Scene *scene, const char *title) {
    long cost = 0;
    int rewards = 0;

    printf(ANSI_HIDE_CURSOR);
    for (int step = 0; step < scene->result->path_len; step++) {
        const Node *entered = &scene->graph->nodes[scene->result->path[step]];
        if (step > 0) {
            cost += entered->entry_cost;
            printf(ANSI_LINES_UP_FORMAT, frame_height(scene->map));
        }
        rewards += entered->has_reward;
        print_frame(scene, title, step, cost, rewards);
        fflush(stdout);
        sleep_milliseconds(ANIMATION_DELAY_MS);
        mark_trail(scene, scene->result->path[step]);
    }
    printf(ANSI_SHOW_CURSOR);
}

static void print_final_route(const Scene *scene, const char *title) {
    for (int step = 0; step < scene->result->path_len; step++) {
        mark_trail(scene, scene->result->path[step]);
    }
    print_frame(scene, title, scene->result->path_len - 1, scene->result->cost,
                scene->result->rewards_collected);
}

void render_route(const Map *map, const Graph *graph, const SearchResult *result, const char *title,
                  bool animate) {
    if (!result->found) {
        printf("%s\n", title);
        render_map(map);
        printf("SEM CAMINHO ATÉ O PRÊMIO\n");
        return;
    }

    Scene scene = {map, graph, result, calloc((size_t)map->rows * (size_t)map->cols, sizeof(bool)), NULL};
    if (scene.on_trail == NULL) {
        fprintf(stderr, "erro: sem memória para desenhar a rota\n");
        exit(EXIT_FAILURE);
    }
    if (animate && isatty(STDOUT_FILENO)) {
        animate_route(&scene, title);
    } else {
        print_final_route(&scene, title);
    }
    free(scene.on_trail);
}

/* Quais células o algoritmo expandiu: mostra o custo da busca em área, não só a rota final. */
static bool *mark_explored_cells(const Map *map, const Graph *graph, const SearchResult *result) {
    bool *explored = calloc((size_t)map->rows * (size_t)map->cols, sizeof(bool));
    if (explored == NULL) {
        fprintf(stderr, "erro: sem memória para desenhar a exploração\n");
        exit(EXIT_FAILURE);
    }
    for (int id = 0; id < graph->node_count; id++) {
        if (result->expansion_order[id] > 0) {
            explored[graph->nodes[id].row * map->cols + graph->nodes[id].col] = true;
        }
    }
    return explored;
}

void render_exploration(const Map *map, const Graph *graph, const SearchResult *result, const char *title) {
    if (!result->found) {
        return;
    }
    bool *explored = mark_explored_cells(map, graph, result);
    Scene scene = {map, graph, result, calloc((size_t)map->rows * (size_t)map->cols, sizeof(bool)), explored};
    if (scene.on_trail == NULL) {
        fprintf(stderr, "erro: sem memória para desenhar a exploração\n");
        exit(EXIT_FAILURE);
    }
    for (int step = 0; step < result->path_len; step++) {
        mark_trail(&scene, result->path[step]);
    }

    const Node *goal = &graph->nodes[result->path[result->path_len - 1]];
    printf("%s: células expandidas%s\n", title, ANSI_CLEAR_LINE);
    print_grid(map, scene.on_trail, explored, goal->row, goal->col);
    printf("expandidos %d | gerados %d | fronteira máx. %d\n", result->expanded, result->generated,
           result->peak_frontier);
    free(scene.on_trail);
    free(explored);
}
