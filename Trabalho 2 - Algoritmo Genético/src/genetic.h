#ifndef GENETIC_H
#define GENETIC_H

#include "data.h"
#include "chromosome.h"
#include "rng.h"

typedef struct {
    int population_size;
    int tournament_size;
    double crossover_rate;
    double mutation_rate;
    int elite_count;
    int min_generations;
    int max_generations;
} GeneticConfig;

typedef struct {
    Individual best;
    int generations;
    int *best_penalty_history;
} GeneticResult;

typedef struct {
    int generation;
    int best_penalty;
    double mean_penalty;
    const Individual *best;
} GeneticProgress;

typedef void (*GeneticObserver)(const GeneticProgress *progress, void *context);

GeneticConfig genetic_default_config(void);
void crossover_uniform(const Individual *first, const Individual *second, Individual *child, const BlockList *list, Rng *rng);
void mutate(Individual *individual, const BlockList *list, double rate, Rng *rng);
int tournament_select(const Individual *population, int population_size, int tournament_size, Rng *rng);
int genetic_run(GeneticResult *result, const GeneticConfig *config, const BlockList *list, const Dataset *dataset, unsigned long long seed);
int genetic_run_observed(GeneticResult *result, const GeneticConfig *config, const BlockList *list, const Dataset *dataset,
                         unsigned long long seed, GeneticObserver observer, void *context);
void genetic_result_free(GeneticResult *result);

#endif
