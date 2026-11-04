#include <string.h>
#include "test_support.h"
#include "../src/data.h"
#include "../src/rng.h"
#include "../src/chromosome.h"

static Dataset dataset;
static BlockList block_list;

static void build_single_course(int lessons, int professor_count, int saturday)
{
    memset(&dataset, 0, sizeof(dataset));
    dataset.course_count = 1;
    dataset.professor_count = professor_count;
    strcpy(dataset.courses[0].code, "AAA1");
    dataset.courses[0].period = 1;
    dataset.courses[0].weekly_lessons = lessons;
    dataset.courses[0].professor_count = professor_count;
    dataset.courses[0].professors[0] = 0;
    dataset.courses[0].professors[1] = professor_count > 1 ? 1 : 0;
    dataset.courses[0].saturday = saturday;
}

static int block_length_matches(const int *expected, int expected_count)
{
    int index;

    if (block_list.block_count != expected_count) {
        return 0;
    }
    for (index = 0; index < expected_count; index++) {
        if (block_list.blocks[index].length != expected[index]) {
            return 0;
        }
    }
    return 1;
}

static void test_block_split(void)
{
    const int two[] = {2};
    const int three[] = {3};
    const int four[] = {2, 2};
    const int six[] = {3, 3};
    const int twelve[] = {3, 3, 3, 3};

    build_single_course(2, 1, 0);
    CHECK(blocks_build(&block_list, &dataset) == 0);
    CHECK(block_length_matches(two, 1));

    build_single_course(3, 1, 0);
    CHECK(blocks_build(&block_list, &dataset) == 0);
    CHECK(block_length_matches(three, 1));

    build_single_course(4, 1, 0);
    CHECK(blocks_build(&block_list, &dataset) == 0);
    CHECK(block_length_matches(four, 2));

    build_single_course(6, 1, 0);
    CHECK(blocks_build(&block_list, &dataset) == 0);
    CHECK(block_length_matches(six, 2));

    build_single_course(12, 1, 0);
    CHECK(blocks_build(&block_list, &dataset) == 0);
    CHECK(block_length_matches(twelve, 4));
}

static void test_valid_start(void)
{
    CHECK(start_is_valid(4, 2, 0) == 1);
    CHECK(start_is_valid(5, 2, 0) == 0);
    CHECK(start_is_valid(6, 3, 0) == 1);
    CHECK(start_is_valid(10, 3, 0) == 0);
    CHECK(start_is_valid(60, 3, 1) == 1);
    CHECK(start_is_valid(60, 3, 0) == 0);
    CHECK(start_is_valid(64, 3, 1) == 0);
}

static void test_block_professors(void)
{
    build_single_course(12, 2, 0);
    CHECK(blocks_build(&block_list, &dataset) == 0);
    CHECK(block_list.block_count == 4);
    CHECK(block_list.blocks[0].professor == 0);
    CHECK(block_list.blocks[1].professor == 0);
    CHECK(block_list.blocks[2].professor == 1);
    CHECK(block_list.blocks[3].professor == 1);

    build_single_course(6, 1, 0);
    CHECK(blocks_build(&block_list, &dataset) == 0);
    CHECK(block_list.block_count == 2);
    CHECK(block_list.blocks[0].professor == 0);
    CHECK(block_list.blocks[1].professor == 0);

    build_single_course(3, 1, 1);
    CHECK(blocks_build(&block_list, &dataset) == 0);
    CHECK(block_list.blocks[0].saturday == 1);
}

static void test_random_valid(void)
{
    Rng rng;
    int iteration;
    int lengths[2] = {2, 3};
    int saturdays[2] = {0, 1};
    int length_index;
    int saturday_index;
    int start;
    int valid = 1;
    int below_sixty = 1;

    rng_seed(&rng, 42ULL);
    for (length_index = 0; length_index < 2; length_index++) {
        for (saturday_index = 0; saturday_index < 2; saturday_index++) {
            for (iteration = 0; iteration < 10000; iteration++) {
                start = start_random(&rng, lengths[length_index], saturdays[saturday_index]);
                if (!start_is_valid(start, lengths[length_index], saturdays[saturday_index])) {
                    valid = 0;
                }
                if (saturdays[saturday_index] == 0 && start >= 60) {
                    below_sixty = 0;
                }
            }
        }
    }
    CHECK(valid == 1);
    CHECK(below_sixty == 1);
}

static void test_rng_repeat(void)
{
    Rng first;
    Rng second;
    int index;
    int same = 1;

    rng_seed(&first, 12345ULL);
    rng_seed(&second, 12345ULL);
    for (index = 0; index < 100; index++) {
        if (rng_next(&first) != rng_next(&second)) {
            same = 0;
        }
    }
    CHECK(same == 1);
}

static void test_individual_randomize(void)
{
    Individual individual;
    Rng rng;
    int index;
    int valid = 1;

    build_single_course(12, 2, 0);
    CHECK(blocks_build(&block_list, &dataset) == 0);
    rng_seed(&rng, 7ULL);
    individual_randomize(&individual, &block_list, &rng);
    CHECK(individual.penalty == -1);
    for (index = 0; index < block_list.block_count; index++) {
        if (!start_is_valid(individual.genes[index], block_list.blocks[index].length, block_list.blocks[index].saturday)) {
            valid = 0;
        }
    }
    CHECK(valid == 1);
}

void test_chromosome_run(void)
{
    test_block_split();
    test_valid_start();
    test_block_professors();
    test_random_valid();
    test_rng_repeat();
    test_individual_randomize();
}
