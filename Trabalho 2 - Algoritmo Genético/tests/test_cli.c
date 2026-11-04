#include <string.h>
#include "test_support.h"
#include "../src/cli.h"
#include "../src/data.h"
#include "../src/chromosome.h"
#include "../src/genetic.h"
#include "../src/fitness.h"

static int parse(CliOptions *options, int argc, char **argv, char *error)
{
    error[0] = '\0';
    return cli_parse(options, argc, argv, error, 256);
}

static void test_cli_defaults(void)
{
    CliOptions options;
    char error[256];
    char *argv[] = {"trabalho2"};

    CHECK(parse(&options, 1, argv, error) == 0);
    CHECK(options.seed == 1);
    CHECK(options.periods == 4);
    CHECK(options.runs == 1);
    CHECK(options.genetic.max_generations >= 500);
}

static void test_cli_values(void)
{
    CliOptions options;
    char error[256];
    char *argv[] = {"trabalho2", "--seed", "42", "--periods", "8", "--runs", "30",
                    "--population", "50", "--crossover", "0.8", "--mutation", "0.05",
                    "--generations", "700"};

    CHECK(parse(&options, 15, argv, error) == 0);
    CHECK(options.seed == 42);
    CHECK(options.periods == 8);
    CHECK(options.runs == 30);
    CHECK(options.genetic.population_size == 50);
    CHECK(options.genetic.crossover_rate == 0.8);
    CHECK(options.genetic.mutation_rate == 0.05);
    CHECK(options.genetic.max_generations == 700);
}

static void expect_invalid(int argc, char **argv)
{
    CliOptions options;
    char error[256];

    CHECK(parse(&options, argc, argv, error) == -1);
    CHECK(error[0] != '\0');
}

static void test_cli_invalid(void)
{
    char *periods[] = {"trabalho2", "--periods", "5"};
    char *seed[] = {"trabalho2", "--seed", "abc"};
    char *negative[] = {"trabalho2", "--seed", "-1"};
    char *runs[] = {"trabalho2", "--runs", "0"};
    char *mutation[] = {"trabalho2", "--mutation", "2"};
    char *population[] = {"trabalho2", "--population", "2"};
    char *generations[] = {"trabalho2", "--generations", "499"};
    char *unknown[] = {"trabalho2", "--bogus", "1"};
    char *missing[] = {"trabalho2", "--seed"};

    expect_invalid(3, periods);
    expect_invalid(3, seed);
    expect_invalid(3, negative);
    expect_invalid(3, runs);
    expect_invalid(3, mutation);
    expect_invalid(3, population);
    expect_invalid(3, generations);
    expect_invalid(3, unknown);
    expect_invalid(2, missing);
}

static Dataset dataset;
static BlockList list;

static void test_verify_real_data(void)
{
    char error[256];
    GeneticConfig config = genetic_default_config();
    GeneticResult result;

    error[0] = '\0';
    CHECK(dataset_load(&dataset, "data", 4, error, sizeof(error)) == 0);
    CHECK(blocks_build(&list, &dataset) == 0);
    CHECK(genetic_run(&result, &config, &list, &dataset, 1) == 0);
    CHECK(verify_penalty(&result.best, &list, &dataset) == result.best.penalty);
    genetic_result_free(&result);
}

static void test_verify_hand_built(void)
{
    static Dataset hand;
    static BlockList hand_list;
    Individual individual;

    memset(&hand, 0, sizeof(hand));
    memset(&hand_list, 0, sizeof(hand_list));
    memset(&individual, 0, sizeof(individual));
    hand.course_count = 2;
    hand.professor_count = 1;
    hand.courses[0].period = 1;
    hand.courses[1].period = 1;
    hand_list.block_count = 2;
    hand_list.blocks[0].course = 0;
    hand_list.blocks[0].length = 2;
    hand_list.blocks[1].course = 1;
    hand_list.blocks[1].length = 3;
    individual.genes[0] = 0;
    individual.genes[1] = 1;
    CHECK(verify_penalty(&individual, &hand_list, &hand) == 2);
    individual.genes[1] = 6;
    CHECK(verify_penalty(&individual, &hand_list, &hand) == 0);
    hand.blocked[0][6] = 1;
    CHECK(verify_penalty(&individual, &hand_list, &hand) == 1);
}

static void test_verify_random_agreement(void)
{
    Rng rng;
    Individual individual;
    char error[256];
    int trial;

    error[0] = '\0';
    CHECK(dataset_load(&dataset, "data", 8, error, sizeof(error)) == 0);
    CHECK(blocks_build(&list, &dataset) == 0);
    rng_seed(&rng, 7);
    for (trial = 0; trial < 200; trial++) {
        individual_randomize(&individual, &list, &rng);
        CHECK(verify_penalty(&individual, &list, &dataset) == penalty_compute(&individual, &list, &dataset));
    }
}

void test_cli_run(void)
{
    test_cli_defaults();
    test_cli_values();
    test_cli_invalid();
    test_verify_real_data();
    test_verify_hand_built();
    test_verify_random_agreement();
}
