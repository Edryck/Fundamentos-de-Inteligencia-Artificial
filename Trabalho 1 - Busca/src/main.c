#include <stdio.h>
#include <stdlib.h>

#include "bench.h"
#include "cli.h"
#include "generator.h"
#include "graph.h"
#include "map.h"
#include "metrics.h"
#include "render.h"
#include "runner.h"
#include "search.h"

static void fail(const char *message) {
    fprintf(stderr, "erro: %s\n", message);
    exit(EXIT_FAILURE);
}

static void apply_endpoint(Map *map, char symbol, const CellChoice *choice) {
    char error[MAP_ERROR_SIZE] = "";

    if (choice->set && !map_move_symbol(map, symbol, choice->row, choice->col, error, sizeof error)) {
        map_free(map);
        fail(error);
    }
}

static void prepare_map(const Options *options, Map *map) {
    char error[MAP_ERROR_SIZE] = "";

    if (options->map_path != NULL) {
        if (!map_load(options->map_path, map, error, sizeof error)) {
            fail(error);
        }
    } else {
        if (!map_generate(&options->generator, map, error, sizeof error)) {
            fail(error);
        }
        printf("seed: %llu\n", (unsigned long long)options->generator.seed);
    }

    apply_endpoint(map, CELL_START, &options->from);
    apply_endpoint(map, CELL_GOAL, &options->to);

    if (options->save_path != NULL && !map_save(map, options->save_path, error, sizeof error)) {
        map_free(map);
        fail(error);
    }
}

static int run_selected_algorithms(const Options *options, const Map *map, const Graph *graph,
                                   bool goal_reachable, SearchResult results[], MetricsRow rows[]) {
    int count = 0;

    for (int a = 0; a < ALGO_COUNT; a++) {
        if (!options->algorithms[a]) {
            continue;
        }
        results[count] = run_algorithm((Algorithm)a, graph, &options->heuristic, options->repeat);
        rows[count] = (MetricsRow){algorithm_label((Algorithm)a), &results[count]};
        if (goal_reachable) {
            render_route(map, graph, &results[count], rows[count].name, options->animate);
            if (options->show_explored) {
                render_exploration(map, graph, &results[count], rows[count].name);
            }
        }
        count++;
    }
    return count;
}

int main(int argc, char **argv) {
    Options options = cli_parse(argc, argv);
    if (options.bench) {
        bench_run(&options);
        return EXIT_SUCCESS;
    }

    Map map = {0};
    Graph graph;
    GraphAnalysis analysis;

    prepare_map(&options, &map);
    if (!graph_build(&map, &graph)) {
        map_free(&map);
        fail("sem memória para montar o grafo");
    }
    if (!graph_analyze(&graph, &analysis)) {
        graph_free(&graph);
        map_free(&map);
        fail("sem memória para analisar o grafo");
    }

    render_legend();
    render_map(&map);
    render_analysis(&graph, &analysis);

    SearchResult results[ALGO_COUNT];
    MetricsRow rows[ALGO_COUNT];
    int count = run_selected_algorithms(&options, &map, &graph, analysis.goal_reachable, results, rows);

    putchar('\n');
    metrics_print_table(rows, count);

    for (int i = 0; i < count; i++) {
        search_result_free(&results[i]);
    }
    graph_analysis_free(&analysis);
    graph_free(&graph);
    map_free(&map);
    return EXIT_SUCCESS;
}
