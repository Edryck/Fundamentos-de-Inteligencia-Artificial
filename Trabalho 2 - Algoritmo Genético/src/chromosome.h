#ifndef CHROMOSOME_H
#define CHROMOSOME_H

#include "data.h"
#include "rng.h"

#define MAX_BLOCKS 256

typedef struct {
    int course;
    int length;
    int professor;
    int saturday;
} Block;

typedef struct {
    Block blocks[MAX_BLOCKS];
    int block_count;
} BlockList;

typedef struct {
    int genes[MAX_BLOCKS];
    int penalty;
} Individual;

int blocks_build(BlockList *list, const Dataset *dataset);
int start_is_valid(int start, int length, int saturday);
int start_random(Rng *rng, int length, int saturday);
void individual_randomize(Individual *individual, const BlockList *list, Rng *rng);

#endif
