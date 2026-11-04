#ifndef CLI_H
#define CLI_H

#include <stddef.h>
#include "genetic.h"

typedef struct {
    unsigned long long seed;
    int periods;
    int runs;
    GeneticConfig genetic;
} CliOptions;

int cli_parse(CliOptions *options, int argc, char **argv, char *error, size_t error_size);

#endif
