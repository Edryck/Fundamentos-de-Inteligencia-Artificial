#include <stdlib.h>
#include <string.h>
#include "genetic.h"
#include "fitness.h"

GeneticConfig genetic_default_config(void)
{
    GeneticConfig config;

    config.population_size = 200;
    config.tournament_size = 3;
    config.crossover_rate = 0.9;
    config.mutation_rate = 0.02;
    config.elite_count = 2;
    config.min_generations = 500;
    config.max_generations = 2000;
    return config;
}

void crossover_uniform(const Individual *first, const Individual *second, Individual *child, const BlockList *list, Rng *rng)
{
    int index;

    for (index = 0; index < list->block_count; index++) {
        if (rng_int(rng, 2) == 0) {
            child->genes[index] = first->genes[index];
        } else {
            child->genes[index] = second->genes[index];
        }
    }
    child->penalty = -1;
}

static int valid_start_count(int length, int saturday)
{
    int start;
    int count = 0;

    for (start = 0; start < SLOT_COUNT; start++) {
        if (start_is_valid(start, length, saturday)) {
            count++;
        }
    }
    return count;
}

void mutate(Individual *individual, const BlockList *list, double rate, Rng *rng)
{
    int index;
    int current;
    int candidate;
    const Block *block;

    for (index = 0; index < list->block_count; index++) {
        if (rng_double(rng) >= rate) {
            continue;
        }
        block = &list->blocks[index];
        current = individual->genes[index];
        candidate = start_random(rng, block->length, block->saturday);
        if (valid_start_count(block->length, block->saturday) > 1) {
            while (candidate == current) {
                candidate = start_random(rng, block->length, block->saturday);
            }
        }
        individual->genes[index] = candidate;
        individual->penalty = -1;
    }
}

int tournament_select(const Individual *population, int population_size, int tournament_size, Rng *rng)
{
    int best = rng_int(rng, population_size);
    int round;
    int candidate;

    for (round = 1; round < tournament_size; round++) {
        candidate = rng_int(rng, population_size);
        if (population[candidate].penalty < population[best].penalty) {
            best = candidate;
        }
    }
    return best;
}

static void evaluate(Individual *population, int size, const BlockList *list, const Dataset *dataset)
{
    int index;

    for (index = 0; index < size; index++) {
        population[index].penalty = penalty_compute(&population[index], list, dataset);
    }
}

static int best_index(const Individual *population, int size)
{
    int index;
    int best = 0;

    for (index = 1; index < size; index++) {
        if (population[index].penalty < population[best].penalty) {
            best = index;
        }
    }
    return best;
}

static void copy_elites(const Individual *current, Individual *next, int size, int elite_count, unsigned char *used)
{
    int elite;
    int index;
    int best;

    memset(used, 0, (size_t)size);
    for (elite = 0; elite < elite_count; elite++) {
        best = -1;
        for (index = 0; index < size; index++) {
            if (!used[index] && (best < 0 || current[index].penalty < current[best].penalty)) {
                best = index;
            }
        }
        used[best] = 1;
        next[elite] = current[best];
    }
}

static void notify(GeneticObserver observer, void *context, int generation, const Individual *population, int size, int best)
{
    GeneticProgress progress;
    double total = 0.0;
    int index;

    for (index = 0; index < size; index++) {
        total += population[index].penalty;
    }
    progress.generation = generation;
    progress.best_penalty = population[best].penalty;
    progress.mean_penalty = total / size;
    progress.best = &population[best];
    observer(&progress, context);
}

int genetic_run(GeneticResult *result, const GeneticConfig *config, const BlockList *list, const Dataset *dataset, unsigned long long seed)
{
    return genetic_run_observed(result, config, list, dataset, seed, NULL, NULL);
}

int genetic_run_observed(GeneticResult *result, const GeneticConfig *config, const BlockList *list, const Dataset *dataset,
                         unsigned long long seed, GeneticObserver observer, void *context)
{
    Rng rng;
    Individual *current;
    Individual *next;
    Individual *swap;
    unsigned char *used;
    int generation = 0;
    int index;
    int first;
    int second;
    int best;
    int size = config->population_size;
    int elites = config->elite_count;

    result->generations = 0;
    result->best_penalty_history = NULL;
    if (config->population_size <= 0 || config->tournament_size <= 0 || config->elite_count < 0
        || config->elite_count > config->population_size || config->min_generations < 0
        || config->max_generations <= 0 || config->min_generations > config->max_generations) {
        return -1;
    }
    current = malloc(sizeof(Individual) * (size_t)size);
    next = malloc(sizeof(Individual) * (size_t)size);
    used = malloc((size_t)size);
    result->best_penalty_history = malloc(sizeof(int) * (size_t)config->max_generations);
    if (current == NULL || next == NULL || used == NULL || result->best_penalty_history == NULL) {
        free(current);
        free(next);
        free(used);
        free(result->best_penalty_history);
        result->best_penalty_history = NULL;
        return -1;
    }

    rng_seed(&rng, seed);
    for (index = 0; index < size; index++) {
        individual_randomize(&current[index], list, &rng);
    }
    evaluate(current, size, list, dataset);

    for (;;) {
        generation++;
        copy_elites(current, next, size, elites, used);
        for (index = elites; index < size; index++) {
            first = tournament_select(current, size, config->tournament_size, &rng);
            if (rng_double(&rng) < config->crossover_rate) {
                second = tournament_select(current, size, config->tournament_size, &rng);
                crossover_uniform(&current[first], &current[second], &next[index], list, &rng);
            } else {
                next[index] = current[first];
            }
            mutate(&next[index], list, config->mutation_rate, &rng);
        }
        for (index = elites; index < size; index++) {
            next[index].penalty = penalty_compute(&next[index], list, dataset);
        }
        swap = current;
        current = next;
        next = swap;
        best = best_index(current, size);
        result->best_penalty_history[generation - 1] = current[best].penalty;
        result->best = current[best];
        result->generations = generation;
        if (observer != NULL) {
            notify(observer, context, generation, current, size, best);
        }
        if ((generation >= config->min_generations && current[best].penalty == 0) || generation == config->max_generations) {
            break;
        }
    }

    free(current);
    free(next);
    free(used);
    return 0;
}

void genetic_result_free(GeneticResult *result)
{
    free(result->best_penalty_history);
    result->best_penalty_history = NULL;
}
