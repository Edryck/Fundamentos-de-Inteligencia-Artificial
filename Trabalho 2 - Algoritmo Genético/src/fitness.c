#include "fitness.h"

static int overlap_length(int start_a, int length_a, int start_b, int length_b)
{
    int low = start_a > start_b ? start_a : start_b;
    int end_a = start_a + length_a;
    int end_b = start_b + length_b;
    int high = end_a < end_b ? end_a : end_b;

    return high > low ? high - low : 0;
}

static int prerequisite_related(const Dataset *dataset, int course_a, int course_b)
{
    int index;

    for (index = 0; index < dataset->prerequisite_count; index++) {
        if ((dataset->prerequisites[index][0] == course_a && dataset->prerequisites[index][1] == course_b) ||
            (dataset->prerequisites[index][0] == course_b && dataset->prerequisites[index][1] == course_a)) {
            return 1;
        }
    }
    return 0;
}

ConflictCount conflicts_count(const Individual *individual, const BlockList *list, const Dataset *dataset)
{
    ConflictCount count = {0, 0, 0, 0};
    int first;
    int second;
    int slot;
    int overlap;
    const Block *block_a;
    const Block *block_b;

    for (first = 0; first < list->block_count; first++) {
        block_a = &list->blocks[first];
        for (slot = individual->genes[first]; slot < individual->genes[first] + block_a->length; slot++) {
            if (dataset->blocked[block_a->professor][slot]) {
                count.blocked++;
            }
        }
        for (second = first + 1; second < list->block_count; second++) {
            block_b = &list->blocks[second];
            overlap = overlap_length(individual->genes[first], block_a->length,
                                     individual->genes[second], block_b->length);
            if (overlap == 0) {
                continue;
            }
            if (block_a->professor == block_b->professor) {
                count.professor += overlap;
            }
            if (dataset->courses[block_a->course].period == dataset->courses[block_b->course].period) {
                count.group += overlap;
            }
            if (block_a->course != block_b->course &&
                prerequisite_related(dataset, block_a->course, block_b->course)) {
                count.prerequisite += overlap;
            }
        }
    }
    return count;
}

int penalty_compute(const Individual *individual, const BlockList *list, const Dataset *dataset)
{
    ConflictCount count = conflicts_count(individual, list, dataset);

    return count.professor + count.prerequisite + count.group + count.blocked;
}

double fitness_value(int penalty)
{
    return 1.0 / (1.0 + penalty);
}
