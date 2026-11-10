#ifndef CLI_H
#define CLI_H

#include <stddef.h>
#include "genetic.h"

typedef struct {
    unsigned long long seed;
    int seed_given;
    int periods;
    int runs;
    int live;
    int live_period;
    int live_period_given;
    int delay_ms;
    int delay_given;
    GeneticConfig genetic;
} CliOptions;

int cli_parse(CliOptions *options, int argc, char **argv, char *error, size_t error_size);

#endif
