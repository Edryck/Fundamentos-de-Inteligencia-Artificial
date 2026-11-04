#include "rng.h"

#define SPLITMIX_INCREMENT 0x9E3779B97F4A7C15ULL
#define SPLITMIX_MIX_1 0xBF58476D1CE4E5B9ULL
#define SPLITMIX_MIX_2 0x94D049BB133111EBULL
#define RANGE_32 0x100000000ULL

void rng_seed(Rng *rng, unsigned long long seed)
{
    rng->state = seed;
}

unsigned int rng_next(Rng *rng)
{
    unsigned long long z;

    rng->state += SPLITMIX_INCREMENT;
    z = rng->state;
    z = (z ^ (z >> 30)) * SPLITMIX_MIX_1;
    z = (z ^ (z >> 27)) * SPLITMIX_MIX_2;
    z = z ^ (z >> 31);
    return (unsigned int)(z >> 32);
}

int rng_int(Rng *rng, int limit)
{
    unsigned int value;
    unsigned int threshold;

    if (limit <= 0) {
        return 0;
    }
    threshold = (unsigned int)(RANGE_32 % (unsigned long long)limit);
    do {
        value = rng_next(rng);
    } while (value < threshold);
    return (int)(value % (unsigned int)limit);
}

double rng_double(Rng *rng)
{
    return (double)rng_next(rng) / (double)RANGE_32;
}
