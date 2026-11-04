#include <string.h>
#include "test_support.h"

static int pairs(int count)
{
    return count * (count - 1) / 2;
}

int verify_penalty(const Individual *individual, const BlockList *list, const Dataset *dataset)
{
    static int professor_occupancy[MAX_PROFESSORS][SLOT_COUNT];
    static int period_occupancy[16][SLOT_COUNT];
    static int course_occupancy[MAX_COURSES][SLOT_COUNT];
    int total = 0;
    int index;
    int slot;
    int first;
    int second;
    int related;
    int link;
    const Block *block;

    memset(professor_occupancy, 0, sizeof(professor_occupancy));
    memset(period_occupancy, 0, sizeof(period_occupancy));
    memset(course_occupancy, 0, sizeof(course_occupancy));

    for (index = 0; index < list->block_count; index++) {
        block = &list->blocks[index];
        for (slot = individual->genes[index]; slot < individual->genes[index] + block->length; slot++) {
            professor_occupancy[block->professor][slot]++;
            period_occupancy[dataset->courses[block->course].period][slot]++;
            course_occupancy[block->course][slot]++;
            if (dataset->blocked[block->professor][slot]) {
                total++;
            }
        }
    }

    for (slot = 0; slot < SLOT_COUNT; slot++) {
        for (index = 0; index < dataset->professor_count; index++) {
            total += pairs(professor_occupancy[index][slot]);
        }
        for (index = 0; index < 16; index++) {
            total += pairs(period_occupancy[index][slot]);
        }
    }

    for (first = 0; first < dataset->course_count; first++) {
        for (second = first + 1; second < dataset->course_count; second++) {
            related = 0;
            for (link = 0; link < dataset->prerequisite_count; link++) {
                if ((dataset->prerequisites[link][0] == first && dataset->prerequisites[link][1] == second)
                    || (dataset->prerequisites[link][0] == second && dataset->prerequisites[link][1] == first)) {
                    related = 1;
                }
            }
            if (!related) {
                continue;
            }
            for (slot = 0; slot < SLOT_COUNT; slot++) {
                total += course_occupancy[first][slot] * course_occupancy[second][slot];
            }
        }
    }
    return total;
}
