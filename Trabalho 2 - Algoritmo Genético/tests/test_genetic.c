#include <string.h>
#include "test_support.h"
#include "../src/data.h"
#include "../src/chromosome.h"
#include "../src/fitness.h"
#include "../src/genetic.h"

static Dataset dataset;
static BlockList list;

static void course_set(int index, int period, int lessons, int professor)
{
    memset(&dataset.courses[index], 0, sizeof(Course));
    dataset.courses[index].period = period;
    dataset.courses[index].weekly_lessons = lessons;
    dataset.courses[index].professors[0] = professor;
    dataset.courses[index].professor_count = 1;
    dataset.courses[index].saturday = 1;
}

static void easy_setup(void)
{
    memset(&dataset, 0, sizeof(dataset));
    dataset.professor_count = 1;
    dataset.course_count = 1;
    course_set(0, 1, 3, 0);
    blocks_build(&list, &dataset);
}

static void infeasible_setup(void)
{
    int index;

    memset(&dataset, 0, sizeof(dataset));
    dataset.professor_count = 1;
    dataset.course_count = 6;
    for (index = 0; index < 6; index++) {
        course_set(index, index + 1, 12, 0);
    }
    blocks_build(&list, &dataset);
}

static GeneticConfig small_config(void)
{
    GeneticConfig config = genetic_default_config();

    config.population_size = 10;
    return config;
}

static void test_default_config(void)
{
    GeneticConfig config = genetic_default_config();

    CHECK(config.population_size == 200);
    CHECK(config.tournament_size == 3);
    CHECK(config.crossover_rate == 0.9);
    CHECK(config.mutation_rate == 0.02);
    CHECK(config.elite_count == 2);
    CHECK(config.min_generations == 500);
    CHECK(config.max_generations == 2000);
}

static void test_crossover_genes(void)
{
    Individual first;
    Individual second;
    Individual child;
    Rng rng;
    int index;
    int ok = 1;

    infeasible_setup();
    rng_seed(&rng, 5);
    individual_randomize(&first, &list, &rng);
    individual_randomize(&second, &list, &rng);
    crossover_uniform(&first, &second, &child, &list, &rng);
    for (index = 0; index < list.block_count; index++) {
        if (child.genes[index] != first.genes[index] && child.genes[index] != second.genes[index]) {
            ok = 0;
        }
    }
    CHECK(ok);
}

static void test_mutation_valid(void)
{
    Individual individual;
    Individual original;
    Rng rng;
    int index;
    int ok = 1;
    int changed = 0;

    infeasible_setup();
    rng_seed(&rng, 6);
    individual_randomize(&individual, &list, &rng);
    original = individual;
    mutate(&individual, &list, 1.0, &rng);
    for (index = 0; index < list.block_count; index++) {
        if (!start_is_valid(individual.genes[index], list.blocks[index].length, list.blocks[index].saturday)) {
            ok = 0;
        }
        if (individual.genes[index] != original.genes[index]) {
            changed++;
        }
    }
    CHECK(ok);
    CHECK(changed == list.block_count);
}

static void test_tournament_best(void)
{
    Individual population[20];
    Rng rng;
    int index;
    int winner;

    rng_seed(&rng, 7);
    for (index = 0; index < 20; index++) {
        memset(&population[index], 0, sizeof(Individual));
        population[index].penalty = 100 + index;
    }
    population[13].penalty = 3;
    winner = tournament_select(population, 20, 2000, &rng);
    CHECK(winner == 13);
}

static void test_minimum_generations(void)
{
    GeneticResult result;
    GeneticConfig config = small_config();

    easy_setup();
    CHECK(genetic_run(&result, &config, &list, &dataset, 11) == 0);
    CHECK(result.generations == 500);
    CHECK(result.best.penalty == 0);
    CHECK(result.best_penalty_history[498] == 0);
    genetic_result_free(&result);
    CHECK(result.best_penalty_history == NULL);
}

static void test_maximum_generations(void)
{
    GeneticResult result;
    GeneticConfig config = small_config();

    infeasible_setup();
    CHECK(genetic_run(&result, &config, &list, &dataset, 12) == 0);
    CHECK(result.generations == 2000);
    CHECK(result.best.penalty > 0);
    genetic_result_free(&result);
}

static void test_elitism_monotonic(void)
{
    GeneticResult result;
    GeneticConfig config = small_config();
    int index;
    int ok = 1;

    infeasible_setup();
    CHECK(genetic_run(&result, &config, &list, &dataset, 13) == 0);
    for (index = 1; index < result.generations; index++) {
        if (result.best_penalty_history[index] > result.best_penalty_history[index - 1]) {
            ok = 0;
        }
    }
    CHECK(ok);
    CHECK(result.best_penalty_history[result.generations - 1] == result.best.penalty);
    genetic_result_free(&result);
}

static void test_same_seed(void)
{
    GeneticResult first;
    GeneticResult second;
    GeneticConfig config = small_config();

    infeasible_setup();
    CHECK(genetic_run(&first, &config, &list, &dataset, 14) == 0);
    CHECK(genetic_run(&second, &config, &list, &dataset, 14) == 0);
    CHECK(first.generations == second.generations);
    CHECK(memcmp(first.best.genes, second.best.genes, sizeof(int) * (size_t)list.block_count) == 0);
    genetic_result_free(&first);
    genetic_result_free(&second);
}

static void test_invalid_config(void)
{
    GeneticResult result;
    GeneticConfig config;
    int variant;

    easy_setup();
    for (variant = 0; variant < 7; variant++) {
        config = small_config();
        if (variant == 0) config.population_size = 0;
        if (variant == 1) config.tournament_size = 0;
        if (variant == 2) config.elite_count = -1;
        if (variant == 3) config.elite_count = 11;
        if (variant == 4) config.min_generations = -1;
        if (variant == 5) config.max_generations = 0;
        if (variant == 6) config.min_generations = config.max_generations + 1;
        CHECK(genetic_run(&result, &config, &list, &dataset, 1) == -1);
        CHECK(result.best_penalty_history == NULL);
    }
}

void test_genetic_run(void)
{
    test_invalid_config();
    test_default_config();
    test_crossover_genes();
    test_mutation_valid();
    test_tournament_best();
    test_minimum_generations();
    test_maximum_generations();
    test_elitism_monotonic();
    test_same_seed();
}
