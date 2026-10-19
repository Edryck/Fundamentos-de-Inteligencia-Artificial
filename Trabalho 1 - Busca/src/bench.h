#ifndef BENCH_H
#define BENCH_H

#include "cli.h"

/* Gera vários mapas de cada tamanho, roda os algoritmos escolhidos em todos e imprime as médias. */
void bench_run(const Options *options);

#endif
