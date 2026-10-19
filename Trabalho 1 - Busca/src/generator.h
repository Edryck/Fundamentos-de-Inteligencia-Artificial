#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "map.h"

#define MAP_DEFAULT_ROWS 12
#define MAP_DEFAULT_COLS 20

typedef struct {
    int      rows;
    int      cols;
    uint64_t seed;
    bool     isolate_goal;
} GeneratorConfig;

/* Gera um mapa a partir da seed: a mesma configuração sempre produz o mesmo mapa. */
bool map_generate(const GeneratorConfig *config, Map *map, char *error, size_t error_size);

#endif
