#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cli.h"

static int parse_integer(const char *text, long long minimum, long long maximum, long long *value)
{
    char *end;
    long long parsed;

    if (text[0] == '\0' || text[0] == '-' || text[0] == '+') {
        if (!(text[0] == '-' && minimum < 0)) {
            return -1;
        }
    }
    errno = 0;
    parsed = strtoll(text, &end, 10);
    if (errno != 0 || *end != '\0' || parsed < minimum || parsed > maximum) {
        return -1;
    }
    *value = parsed;
    return 0;
}

static int parse_rate(const char *text, double *value)
{
    char *end;
    double parsed;

    if (text[0] == '\0') {
        return -1;
    }
    errno = 0;
    parsed = strtod(text, &end);
    if (errno != 0 || *end != '\0' || !(parsed >= 0.0 && parsed <= 1.0)) {
        return -1;
    }
    *value = parsed;
    return 0;
}

static void error_set(char *error, size_t error_size, const char *format, const char *argument)
{
    if (error != NULL && error_size > 0) {
        snprintf(error, error_size, format, argument);
    }
}

int cli_parse(CliOptions *options, int argc, char **argv, char *error, size_t error_size)
{
    int index;
    const char *name;
    const char *text;
    long long integer;
    double rate;

    options->seed = 1;
    options->seed_given = 0;
    options->periods = 4;
    options->runs = 1;
    options->genetic = genetic_default_config();

    for (index = 1; index < argc; index++) {
        name = argv[index];
        if (strncmp(name, "--", 2) != 0 || (strcmp(name, "--seed") != 0 && strcmp(name, "--periods") != 0
            && strcmp(name, "--runs") != 0 && strcmp(name, "--population") != 0
            && strcmp(name, "--crossover") != 0 && strcmp(name, "--mutation") != 0
            && strcmp(name, "--generations") != 0)) {
            error_set(error, error_size, "unknown option: %s", name);
            return -1;
        }
        if (index + 1 >= argc) {
            error_set(error, error_size, "missing value for %s", name);
            return -1;
        }
        text = argv[++index];
        if (strcmp(name, "--seed") == 0) {
            char *end;
            unsigned long long seed;

            if (text[0] < '0' || text[0] > '9') {
                error_set(error, error_size, "invalid value for %s", name);
                return -1;
            }
            errno = 0;
            seed = strtoull(text, &end, 10);
            if (errno != 0 || *end != '\0') {
                error_set(error, error_size, "invalid value for %s", name);
                return -1;
            }
            options->seed = seed;
            options->seed_given = 1;
        } else if (strcmp(name, "--crossover") == 0 || strcmp(name, "--mutation") == 0) {
            if (parse_rate(text, &rate) != 0) {
                error_set(error, error_size, "invalid value for %s", name);
                return -1;
            }
            if (strcmp(name, "--crossover") == 0) {
                options->genetic.crossover_rate = rate;
            } else {
                options->genetic.mutation_rate = rate;
            }
        } else {
            long long minimum = 1;
            long long maximum = 1000000;

            if (strcmp(name, "--periods") == 0) {
                minimum = 4;
                maximum = 8;
            } else if (strcmp(name, "--population") == 0) {
                minimum = 3;
            } else if (strcmp(name, "--generations") == 0) {
                minimum = 500;
            }
            if (parse_integer(text, minimum, maximum, &integer) != 0
                || (strcmp(name, "--periods") == 0 && integer != 4 && integer != 8)) {
                error_set(error, error_size, "invalid value for %s", name);
                return -1;
            }
            if (strcmp(name, "--periods") == 0) {
                options->periods = (int)integer;
            } else if (strcmp(name, "--runs") == 0) {
                options->runs = (int)integer;
            } else if (strcmp(name, "--population") == 0) {
                options->genetic.population_size = (int)integer;
            } else {
                options->genetic.max_generations = (int)integer;
            }
        }
    }
    return 0;
}
