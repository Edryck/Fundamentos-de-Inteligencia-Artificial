#include "bench.h"

#include <stdio.h>
#include <stdlib.h>

#include "generator.h"
#include "graph.h"
#include "map.h"
#include "metrics.h"
#include "runner.h"

#define BENCH_COLUMN_COUNT 9

static const char *const BENCH_HEADERS[BENCH_COLUMN_COUNT] = {
    "Algoritmo", "Resolvidos", "Passos", "Custo", "Expandidos", "Gerados", "Fronteira máx.",
    "$ coletados", "Tempo (μs)"
};

typedef struct {
    int    solved;
    double steps;
    double cost;
    double expanded;
    double generated;
    double peak_frontier;
    double rewards;
    double time_us;
} Totals;

static void fail(const char *message) {
    fprintf(stderr, "erro: %s\n", message);
    exit(EXIT_FAILURE);
}

/* Passos, custo e recompensas só existem quando há caminho; as demais métricas valem em qualquer caso. */
static void accumulate(Totals *totals, const SearchResult *result) {
    totals->expanded += result->expanded;
    totals->generated += result->generated;
    totals->peak_frontier += result->peak_frontier;
    totals->time_us += result->time_us;
    if (result->found) {
        totals->solved++;
        totals->steps += result->path_len;
        totals->cost += (double)result->cost;
        totals->rewards += result->rewards_collected;
    }
}

static void format_average(char *cell, double total, int count) {
    if (count == 0) {
        snprintf(cell, METRICS_CELL_SIZE, "-");
    } else {
        snprintf(cell, METRICS_CELL_SIZE, "%.1f", total / count);
    }
}

static void format_totals(char *cells, Algorithm algorithm, const Totals *totals, int runs) {
    char *cell = cells;

    snprintf(cell, METRICS_CELL_SIZE, "%s", algorithm_label(algorithm));
    cell += METRICS_CELL_SIZE;
    snprintf(cell, METRICS_CELL_SIZE, "%d/%d", totals->solved, runs);
    cell += METRICS_CELL_SIZE;
    format_average(cell, totals->steps, totals->solved);
    cell += METRICS_CELL_SIZE;
    format_average(cell, totals->cost, totals->solved);
    cell += METRICS_CELL_SIZE;
    format_average(cell, totals->expanded, runs);
    cell += METRICS_CELL_SIZE;
    format_average(cell, totals->generated, runs);
    cell += METRICS_CELL_SIZE;
    format_average(cell, totals->peak_frontier, runs);
    cell += METRICS_CELL_SIZE;
    format_average(cell, totals->rewards, totals->solved);
    cell += METRICS_CELL_SIZE;
    format_average(cell, totals->time_us, runs);
}

static void run_one_map(const Options *options, uint64_t seed, const BenchSize *size, Totals totals[],
                        double *vertex_total) {
    GeneratorConfig generator = {size->rows, size->cols, seed, false};
    Map map = {0};
    Graph graph;
    char error[MAP_ERROR_SIZE] = "";

    if (!map_generate(&generator, &map, error, sizeof error)) {
        fail(error);
    }
    if (!graph_build(&map, &graph)) {
        map_free(&map);
        fail("sem memória para montar o grafo");
    }
    *vertex_total += graph.node_count;

    for (int a = 0; a < ALGO_COUNT; a++) {
        if (!options->algorithms[a]) {
            continue;
        }
        SearchResult result = run_algorithm((Algorithm)a, &graph, &options->heuristic, options->repeat);
        accumulate(&totals[a], &result);
        search_result_free(&result);
    }
    graph_free(&graph);
    map_free(&map);
}

static void bench_one_size(const Options *options, const BenchSize *size) {
    Totals totals[ALGO_COUNT] = {{0}};
    double vertex_total = 0;
    int selected = 0;

    for (int i = 0; i < options->bench_seeds; i++) {
        run_one_map(options, options->generator.seed + (uint64_t)i, size, totals, &vertex_total);
    }

    char *cells = malloc((size_t)ALGO_COUNT * BENCH_COLUMN_COUNT * METRICS_CELL_SIZE);
    if (cells == NULL) {
        fail("sem memória para imprimir a tabela");
    }
    for (int a = 0; a < ALGO_COUNT; a++) {
        if (options->algorithms[a]) {
            format_totals(cells + (size_t)selected * BENCH_COLUMN_COUNT * METRICS_CELL_SIZE, (Algorithm)a,
                          &totals[a], options->bench_seeds);
            selected++;
        }
    }

    printf("\nmapa %dx%d | %d mapas | %.0f vértices em média\n", size->rows, size->cols, options->bench_seeds,
           vertex_total / options->bench_seeds);
    metrics_print_grid(BENCH_COLUMN_COUNT, BENCH_HEADERS, selected, cells);
    free(cells);
}

void bench_run(const Options *options) {
    printf("benchmark: seeds %llu a %llu | bônus %d, raio %d | tempo = média de %d execuções\n",
           (unsigned long long)options->generator.seed,
           (unsigned long long)options->generator.seed + (unsigned long long)options->bench_seeds - 1,
           options->heuristic.bonus, options->heuristic.radius, options->repeat);
    for (int i = 0; i < options->bench_size_count; i++) {
        bench_one_size(options, &options->bench_sizes[i]);
    }
}
