#include <string.h>
#include "test_support.h"
#include "../src/data.h"
#include "../src/chromosome.h"
#include "../src/fitness.h"

static Dataset dataset;
static BlockList list;
static Individual individual;

static void course_set(int index, int period, int professor)
{
    memset(&dataset.courses[index], 0, sizeof(Course));
    dataset.courses[index].period = period;
    dataset.courses[index].professors[0] = professor;
    dataset.courses[index].professor_count = 1;
}

static void block_set(int index, int course, int length, int professor, int start)
{
    list.blocks[index].course = course;
    list.blocks[index].length = length;
    list.blocks[index].professor = professor;
    list.blocks[index].saturday = 0;
    individual.genes[index] = start;
}

static void setup(int courses, int blocks)
{
    memset(&dataset, 0, sizeof(dataset));
    memset(&list, 0, sizeof(list));
    memset(&individual, 0, sizeof(individual));
    dataset.course_count = courses;
    dataset.professor_count = 2;
    list.block_count = blocks;
}

static void test_professor_conflict(void)
{
    ConflictCount count;

    setup(2, 2);
    course_set(0, 1, 0);
    course_set(1, 2, 0);
    block_set(0, 0, 2, 0, 0);
    block_set(1, 1, 2, 0, 0);
    count = conflicts_count(&individual, &list, &dataset);
    CHECK(count.professor == 2);
    CHECK(count.group == 0);
    CHECK(count.prerequisite == 0);
    CHECK(count.blocked == 0);
}

static void test_prerequisite_conflict(void)
{
    ConflictCount count;

    setup(2, 2);
    course_set(0, 1, 0);
    course_set(1, 2, 1);
    dataset.prerequisite_count = 1;
    dataset.prerequisites[0][0] = 1;
    dataset.prerequisites[0][1] = 0;
    block_set(0, 0, 2, 0, 0);
    block_set(1, 1, 2, 1, 1);
    count = conflicts_count(&individual, &list, &dataset);
    CHECK(count.prerequisite == 1);
    CHECK(count.professor == 0);
    CHECK(count.group == 0);
}

static void test_group_conflict(void)
{
    ConflictCount count;

    setup(2, 2);
    course_set(0, 3, 0);
    course_set(1, 3, 1);
    block_set(0, 0, 2, 0, 6);
    block_set(1, 1, 2, 1, 6);
    count = conflicts_count(&individual, &list, &dataset);
    CHECK(count.group == 2);
    CHECK(count.professor == 0);
}

static void test_blocked_conflict(void)
{
    ConflictCount count;

    setup(1, 1);
    course_set(0, 1, 0);
    dataset.blocked[0][2] = 1;
    block_set(0, 0, 3, 0, 0);
    count = conflicts_count(&individual, &list, &dataset);
    CHECK(count.blocked == 1);
    CHECK(count.professor == 0);
}

static void test_two_professors(void)
{
    ConflictCount count;

    setup(2, 3);
    course_set(0, 1, 0);
    course_set(1, 2, 1);
    dataset.courses[1].professors[1] = 0;
    dataset.courses[1].professor_count = 2;
    block_set(0, 0, 2, 0, 0);
    block_set(1, 1, 2, 1, 0);
    block_set(2, 1, 2, 0, 12);
    count = conflicts_count(&individual, &list, &dataset);
    CHECK(count.professor == 0);
    block_set(2, 1, 2, 0, 0);
    count = conflicts_count(&individual, &list, &dataset);
    CHECK(count.professor == 2);
}

static void test_same_course_blocks(void)
{
    ConflictCount count;

    setup(1, 2);
    course_set(0, 1, 0);
    block_set(0, 0, 2, 0, 0);
    block_set(1, 0, 2, 0, 1);
    count = conflicts_count(&individual, &list, &dataset);
    CHECK(count.group == 1);
    CHECK(count.professor == 1);
}

static void test_multiple_rules(void)
{
    setup(2, 2);
    course_set(0, 1, 0);
    course_set(1, 1, 0);
    dataset.prerequisite_count = 1;
    dataset.prerequisites[0][0] = 0;
    dataset.prerequisites[0][1] = 1;
    block_set(0, 0, 2, 0, 0);
    block_set(1, 1, 2, 0, 1);
    CHECK(penalty_compute(&individual, &list, &dataset) == 3);
}

static void test_no_conflict(void)
{
    setup(2, 2);
    course_set(0, 1, 0);
    course_set(1, 1, 1);
    block_set(0, 0, 2, 0, 0);
    block_set(1, 1, 2, 1, 2);
    CHECK(penalty_compute(&individual, &list, &dataset) == 0);
    CHECK(fitness_value(0) == 1.0);
    CHECK(fitness_value(3) == 0.25);
}

void test_fitness_run(void)
{
    test_professor_conflict();
    test_prerequisite_conflict();
    test_group_conflict();
    test_blocked_conflict();
    test_two_professors();
    test_same_course_blocks();
    test_multiple_rules();
    test_no_conflict();
}
