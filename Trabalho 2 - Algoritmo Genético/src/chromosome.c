#include "chromosome.h"

#define SHIFT_SLOTS 6
#define SATURDAY_START 60

static int split_lessons(int lessons, int *lengths)
{
    int count = 0;
    int threes;
    int index;

    if (lessons % 3 == 0) {
        threes = lessons / 3;
        for (index = 0; index < threes; index++) {
            lengths[count++] = 3;
        }
    } else if (lessons % 3 == 2) {
        threes = lessons / 3;
        for (index = 0; index < threes; index++) {
            lengths[count++] = 3;
        }
        lengths[count++] = 2;
    } else if (lessons >= 4) {
        threes = (lessons - 4) / 3;
        for (index = 0; index < threes; index++) {
            lengths[count++] = 3;
        }
        lengths[count++] = 2;
        lengths[count++] = 2;
    } else {
        lengths[count++] = lessons;
    }
    return count;
}

int blocks_build(BlockList *list, const Dataset *dataset)
{
    int course_index;
    int block_index;
    int lengths[MAX_BLOCKS];
    int length_count;
    const Course *course;
    Block *block;

    list->block_count = 0;
    for (course_index = 0; course_index < dataset->course_count; course_index++) {
        course = &dataset->courses[course_index];
        length_count = split_lessons(course->weekly_lessons, lengths);
        if (list->block_count + length_count > MAX_BLOCKS) {
            return -1;
        }
        for (block_index = 0; block_index < length_count; block_index++) {
            block = &list->blocks[list->block_count];
            block->course = course_index;
            block->length = lengths[block_index];
            block->professor = course->professors[0];
            if (course->professor_count > 1 && block_index * 2 >= length_count) {
                block->professor = course->professors[1];
            }
            block->saturday = course->saturday;
            list->block_count++;
        }
    }
    return 0;
}

int start_is_valid(int start, int length, int saturday)
{
    if (start < 0 || start + length > SLOT_COUNT) {
        return 0;
    }
    if (start % SHIFT_SLOTS + length > SHIFT_SLOTS) {
        return 0;
    }
    if (start >= SATURDAY_START && !saturday) {
        return 0;
    }
    return 1;
}

int start_random(Rng *rng, int length, int saturday)
{
    int candidates[SLOT_COUNT];
    int count = 0;
    int start;

    for (start = 0; start < SLOT_COUNT; start++) {
        if (start_is_valid(start, length, saturday)) {
            candidates[count++] = start;
        }
    }
    if (count == 0) {
        return -1;
    }
    return candidates[rng_int(rng, count)];
}

void individual_randomize(Individual *individual, const BlockList *list, Rng *rng)
{
    int index;

    for (index = 0; index < list->block_count; index++) {
        individual->genes[index] = start_random(rng, list->blocks[index].length, list->blocks[index].saturday);
    }
    individual->penalty = -1;
}
