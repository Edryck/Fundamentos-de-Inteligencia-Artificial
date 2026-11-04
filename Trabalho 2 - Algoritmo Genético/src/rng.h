#ifndef RNG_H
#define RNG_H

typedef struct {
    unsigned long long state;
} Rng;

void rng_seed(Rng *rng, unsigned long long seed);
unsigned int rng_next(Rng *rng);
int rng_int(Rng *rng, int limit);
double rng_double(Rng *rng);

#endif
