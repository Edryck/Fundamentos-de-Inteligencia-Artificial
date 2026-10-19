#include "generator.h"

#include <stdlib.h>
#include <string.h>

#include "queue.h"

#define WALL_PERCENT         15
#define WALL_SEGMENT_MIN     2
#define WALL_SEGMENT_MAX     4
#define WALL_ATTEMPTS_FACTOR 10
#define PATCH_AREA           20
#define MIN_PATCHES          3
#define CELLS_PER_REWARD     25
#define PLACEMENT_ATTEMPTS   2000
#define GOAL_ENCLOSURE_CELLS 4
#define DIRECTION_COUNT      4

#define SOLID_CHANCE 40
#define SAND_CHANCE  65
#define ROCK_CHANCE  85
#define PERCENT_BASE 100

#define MIN_DIMENSION 5
#define LAYOUT_ATTEMPTS 20

static const int ROW_STEP[DIRECTION_COUNT] = {-1, 1, 0, 0};
static const int COL_STEP[DIRECTION_COUNT] = {0, 0, -1, 1};

typedef struct {
    uint64_t state;
} Rng;

typedef struct {
    int  row;
    int  col;
    char terrain;
} PatchSeed;

/* ---------- gerador pseudoaleatório (xorshift64*) ---------- */

/* O splitmix64 espalha seeds pequenas (0, 1, 2...) antes de entrar no xorshift,
   que fica fraco quando o estado inicial tem poucos bits ligados. */
static void rng_init(Rng *rng, uint64_t seed) {
    uint64_t z = seed + 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z ^= z >> 31;
    rng->state = (z != 0) ? z : 0x9E3779B97F4A7C15ULL;
}

static uint64_t rng_next(Rng *rng) {
    uint64_t x = rng->state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng->state = x;
    return x * 2685821657736338717ULL;
}

static int rng_below(Rng *rng, int limit) {
    return (int)(rng_next(rng) % (uint64_t)limit);
}

/* ---------- células ---------- */

static bool cell_is_plain_terrain(char cell) {
    return cell == CELL_SOLID || cell == CELL_SAND || cell == CELL_ROCK || cell == CELL_SWAMP;
}

static bool is_interior(const Map *map, int row, int col) {
    return row >= 1 && row <= map->rows - 2 && col >= 1 && col <= map->cols - 2;
}

/* ---------- geração procedural ---------- */

static char random_terrain(Rng *rng) {
    int roll = rng_below(rng, PERCENT_BASE);
    if (roll < SOLID_CHANCE) {
        return CELL_SOLID;
    }
    if (roll < SAND_CHANCE) {
        return CELL_SAND;
    }
    if (roll < ROCK_CHANCE) {
        return CELL_ROCK;
    }
    return CELL_SWAMP;
}

static int manhattan_distance(int row_a, int col_a, int row_b, int col_b) {
    return abs(row_a - row_b) + abs(col_a - col_b);
}

/* Cada célula herda o terreno da semente mais próxima, o que forma manchas
   contíguas de terreno em vez de um ruído sem estrutura. */
static bool fill_terrain_patches(Map *map, Rng *rng) {
    int interior_cells = (map->rows - 2) * (map->cols - 2);
    int seed_count = interior_cells / PATCH_AREA + MIN_PATCHES;
    PatchSeed *seeds = malloc((size_t)seed_count * sizeof(PatchSeed));
    if (seeds == NULL) {
        return false;
    }

    for (int i = 0; i < seed_count; i++) {
        seeds[i].row = 1 + rng_below(rng, map->rows - 2);
        seeds[i].col = 1 + rng_below(rng, map->cols - 2);
        seeds[i].terrain = random_terrain(rng);
    }

    for (int row = 1; row <= map->rows - 2; row++) {
        for (int col = 1; col <= map->cols - 2; col++) {
            int nearest = 0;
            int best = manhattan_distance(row, col, seeds[0].row, seeds[0].col);
            for (int i = 1; i < seed_count; i++) {
                int distance = manhattan_distance(row, col, seeds[i].row, seeds[i].col);
                if (distance < best) {
                    best = distance;
                    nearest = i;
                }
            }
            map->cells[row][col] = seeds[nearest].terrain;
        }
    }
    free(seeds);
    return true;
}

/* Paredes em segmentos curtos lembram obstáculos reais e criam becos e corredores. */
static void scatter_walls(Map *map, Rng *rng) {
    int interior_cells = (map->rows - 2) * (map->cols - 2);
    int target = interior_cells * WALL_PERCENT / PERCENT_BASE;
    int placed = 0;
    int attempts = target * WALL_ATTEMPTS_FACTOR;

    while (placed < target && attempts-- > 0) {
        bool horizontal = rng_below(rng, 2) == 0;
        int length = WALL_SEGMENT_MIN + rng_below(rng, WALL_SEGMENT_MAX - WALL_SEGMENT_MIN + 1);
        int row = 1 + rng_below(rng, map->rows - 2);
        int col = 1 + rng_below(rng, map->cols - 2);

        for (int i = 0; i < length && placed < target; i++) {
            int wall_row = horizontal ? row : row + i;
            int wall_col = horizontal ? col + i : col;
            if (is_interior(map, wall_row, wall_col) && map->cells[wall_row][wall_col] != CELL_WALL) {
                map->cells[wall_row][wall_col] = CELL_WALL;
                placed++;
            }
        }
    }
}

/* Sorteia uma célula de terreno comum; se houver máscara, só aceita células alcançáveis. */
static bool pick_free_cell(const Map *map, Rng *rng, const bool *reachable, int *row, int *col) {
    for (int attempt = 0; attempt < PLACEMENT_ATTEMPTS; attempt++) {
        int candidate_row = 1 + rng_below(rng, map->rows - 2);
        int candidate_col = 1 + rng_below(rng, map->cols - 2);
        bool allowed = reachable == NULL || reachable[candidate_row * map->cols + candidate_col];
        if (allowed && cell_is_plain_terrain(map->cells[candidate_row][candidate_col])) {
            *row = candidate_row;
            *col = candidate_col;
            return true;
        }
    }
    return false;
}

/* Início e prêmio ficam longe um do outro para a busca ter o que explorar. */
static bool place_start_and_goal(Map *map, Rng *rng, int *goal_row, int *goal_col) {
    int min_distance = (map->rows + map->cols) / 2;

    for (int attempt = 0; attempt < PLACEMENT_ATTEMPTS; attempt++) {
        int start_row;
        int start_col;
        int row;
        int col;
        if (!pick_free_cell(map, rng, NULL, &start_row, &start_col) ||
            !pick_free_cell(map, rng, NULL, &row, &col)) {
            return false;
        }
        if (manhattan_distance(start_row, start_col, row, col) >= min_distance) {
            map->cells[start_row][start_col] = CELL_START;
            map->cells[row][col] = CELL_GOAL;
            *goal_row = row;
            *goal_col = col;
            return true;
        }
    }
    return false;
}

static void visit_cell(const Map *map, bool *reachable, Queue *queue, int row, int col) {
    int index = row * map->cols + col;
    if (cell_is_walkable(map->cells[row][col]) && !reachable[index]) {
        reachable[index] = true;
        queue_push(queue, index);
    }
}

/* Busca em largura só para saber o que o início alcança; cada célula entra na
   fila uma única vez, então capacidade rows*cols nunca estoura. */
static bool mark_reachable_cells(const Map *map, bool *reachable) {
    Queue queue;
    if (!queue_init(&queue, map->rows * map->cols)) {
        return false;
    }
    memset(reachable, 0, (size_t)map->rows * (size_t)map->cols * sizeof(bool));

    for (int row = 0; row < map->rows; row++) {
        for (int col = 0; col < map->cols; col++) {
            if (map->cells[row][col] == CELL_START) {
                visit_cell(map, reachable, &queue, row, col);
            }
        }
    }
    while (!queue_is_empty(&queue)) {
        int current = queue_pop(&queue);
        for (int d = 0; d < DIRECTION_COUNT; d++) {
            int row = current / map->cols + ROW_STEP[d];
            int col = current % map->cols + COL_STEP[d];
            if (row >= 0 && row < map->rows && col >= 0 && col < map->cols) {
                visit_cell(map, reachable, &queue, row, col);
            }
        }
    }
    queue_free(&queue);
    return true;
}

/* Abre um corredor em L do início até o prêmio, derrubando só as paredes do caminho. */
static void carve_corridor(Map *map, int from_row, int from_col, int to_row, int to_col) {
    int col_direction = (to_col >= from_col) ? 1 : -1;
    int row_direction = (to_row >= from_row) ? 1 : -1;

    for (int col = from_col; col != to_col; col += col_direction) {
        if (map->cells[from_row][col] == CELL_WALL) {
            map->cells[from_row][col] = CELL_SOLID;
        }
    }
    for (int row = from_row; row != to_row; row += row_direction) {
        if (map->cells[row][to_col] == CELL_WALL) {
            map->cells[row][to_col] = CELL_SOLID;
        }
    }
}

/* Depois de abrir o corredor o alcance muda, então é recalculado para as
   recompensas só caírem em células que o agente realmente consegue visitar. */
static bool ensure_goal_reachable(Map *map, bool *reachable, int goal_row, int goal_col) {
    if (!mark_reachable_cells(map, reachable)) {
        return false;
    }
    if (reachable[goal_row * map->cols + goal_col]) {
        return true;
    }
    int start_row = 0;
    int start_col = 0;
    map_find_symbol(map, CELL_START, &start_row, &start_col);
    carve_corridor(map, start_row, start_col, goal_row, goal_col);
    return mark_reachable_cells(map, reachable);
}

/* Cerca o prêmio com paredes nas quatro direções, criando uma sala murada. */
static void enclose_goal(Map *map, int goal_row, int goal_col) {
    for (int d = 0; d < GOAL_ENCLOSURE_CELLS; d++) {
        map->cells[goal_row + ROW_STEP[d]][goal_col + COL_STEP[d]] = CELL_WALL;
    }
}

static bool place_rewards(Map *map, Rng *rng, const bool *reachable) {
    int wanted = map_count_walkable(map) / CELLS_PER_REWARD;
    if (wanted < MAP_MIN_REWARDS) {
        wanted = MAP_MIN_REWARDS;
    }
    for (int i = 0; i < wanted; i++) {
        int row;
        int col;
        if (!pick_free_cell(map, rng, reachable, &row, &col)) {
            return false;
        }
        map->cells[row][col] = CELL_REWARD;
    }
    return true;
}

static bool check_dimensions(const GeneratorConfig *config, char *error, size_t error_size) {
    if (config->rows < MIN_DIMENSION || config->rows > MAP_MAX_ROWS ||
        config->cols < MIN_DIMENSION || config->cols > MAP_MAX_COLS) {
        map_set_error(error, error_size, "dimensões inválidas: use linhas de %d a %d e colunas de %d a %d",
                  MIN_DIMENSION, MAP_MAX_ROWS, MIN_DIMENSION, MAP_MAX_COLS);
        return false;
    }
    /* Sobra de células depois das paredes precisa cobrir o mínimo de vértices
       mais as quatro células que podem ser fechadas ao redor do prêmio. */
    int interior_cells = (config->rows - 2) * (config->cols - 2);
    int usable_cells = interior_cells * (PERCENT_BASE - WALL_PERCENT) / PERCENT_BASE;
    if (usable_cells < MAP_MIN_VERTICES + GOAL_ENCLOSURE_CELLS) {
        map_set_error(error, error_size, "mapa %dx%d pequeno demais para ter %d vértices",
                  config->rows, config->cols, MAP_MIN_VERTICES);
        return false;
    }
    return true;
}

static bool prepare_reachability(Map *map, bool *reachable, bool isolate_goal, int goal_row, int goal_col) {
    if (isolate_goal) {
        enclose_goal(map, goal_row, goal_col);
        return mark_reachable_cells(map, reachable);
    }
    return ensure_goal_reachable(map, reachable, goal_row, goal_col);
}

static bool build_layout(Map *map, Rng *rng, bool isolate_goal, char *error, size_t error_size) {
    int goal_row = 0;
    int goal_col = 0;

    if (!fill_terrain_patches(map, rng)) {
        map_set_error(error, error_size, "sem memória para gerar o mapa");
        return false;
    }
    scatter_walls(map, rng);
    if (!place_start_and_goal(map, rng, &goal_row, &goal_col)) {
        map_set_error(error, error_size, "não foi possível posicionar o início e o prêmio");
        return false;
    }

    bool *reachable = malloc((size_t)map->rows * (size_t)map->cols * sizeof(bool));
    if (reachable == NULL) {
        map_set_error(error, error_size, "sem memória para gerar o mapa");
        return false;
    }
    bool ok = prepare_reachability(map, reachable, isolate_goal, goal_row, goal_col);
    if (!ok) {
        map_set_error(error, error_size, "sem memória para gerar o mapa");
    } else if (!place_rewards(map, rng, reachable)) {
        map_set_error(error, error_size, "não foi possível espalhar as recompensas");
        ok = false;
    }
    free(reachable);
    return ok;
}

/* Um layout pode deixar o início preso num bolsão sem espaço para as recompensas.
   Como o gerador é determinístico, refazer com o mesmo estado continua reprodutível pela seed. */
static bool build_valid_layout(Map *map, Rng *rng, bool isolate_goal, char *error, size_t error_size) {
    for (int attempt = 0; attempt < LAYOUT_ATTEMPTS; attempt++) {
        if (build_layout(map, rng, isolate_goal, error, error_size) &&
            map_validate(map, error, error_size)) {
            return true;
        }
    }
    return false;
}

bool map_generate(const GeneratorConfig *config, Map *map, char *error, size_t error_size) {
    if (!check_dimensions(config, error, error_size)) {
        return false;
    }
    if (!map_allocate(map, config->rows, config->cols)) {
        map_set_error(error, error_size, "sem memória para gerar o mapa");
        return false;
    }

    Rng rng;
    rng_init(&rng, config->seed);
    if (!build_valid_layout(map, &rng, config->isolate_goal, error, error_size)) {
        map_free(map);
        return false;
    }
    return true;
}
