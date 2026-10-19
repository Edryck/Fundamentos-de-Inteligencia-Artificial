#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

#include "generator.h"
#include "heuristic.h"
#include "map.h"

typedef enum {
    ALGO_BFS,
    ALGO_DFS,
    ALGO_UNIFORM_COST,
    ALGO_GREEDY,
    ALGO_ASTAR,
    ALGO_ASTAR_SAFE,
    ALGO_COUNT
} Algorithm;

typedef struct {
    bool set;
    int  row;
    int  col;
} CellChoice;

#define MAX_BENCH_SIZES 8

typedef struct {
    int rows;
    int cols;
} BenchSize;

typedef struct {
    const char     *map_path;
    const char     *save_path;
    GeneratorConfig generator;
    CellChoice      from;
    CellChoice      to;
    HeurCfg         heuristic;
    bool            algorithms[ALGO_COUNT];
    bool            animate;
    bool            show_explored;
    int             repeat;
    bool            bench;
    int             bench_seeds;
    BenchSize       bench_sizes[MAX_BENCH_SIZES];
    int             bench_size_count;
} Options;

/* Lê os argumentos. Em caso de erro ou de --help, imprime a mensagem e encerra o programa. */
Options cli_parse(int argc, char **argv);

const char *algorithm_label(Algorithm algorithm);

#endif
