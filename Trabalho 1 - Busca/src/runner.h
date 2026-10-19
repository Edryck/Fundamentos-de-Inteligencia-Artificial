#ifndef RUNNER_H
#define RUNNER_H

#include "cli.h"
#include "graph.h"
#include "search.h"

/* Roda o algoritmo `repeat` vezes além da execução que fornece o resultado, e devolve esse
   resultado com `time_us` igual à média das repetições. */
SearchResult run_algorithm(Algorithm algorithm, const Graph *graph, const HeurCfg *heuristic, int repeat);

#endif
