#ifndef FITNESS_H
#define FITNESS_H

#include "data.h"
#include "chromosome.h"

typedef struct {
    int professor;
    int prerequisite;
    int group;
    int blocked;
} ConflictCount;

ConflictCount conflicts_count(const Individual *individual, const BlockList *list, const Dataset *dataset);
int penalty_compute(const Individual *individual, const BlockList *list, const Dataset *dataset);
double fitness_value(int penalty);

#endif
