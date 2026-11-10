#ifndef LIVE_H
#define LIVE_H

#include <stdio.h>
#include "chromosome.h"
#include "data.h"
#include "genetic.h"

#define LIVE_WINDOW 60
#define LIVE_MIN_COLUMNS 80
#define LIVE_MIN_LINES 24

typedef struct {
    const BlockList *list;
    const Dataset *dataset;
    int period;
    int delay_ms;
    int runs;
    int run;
    unsigned long long seed;
    int population_size;
    double crossover_rate;
    double mutation_rate;
    int min_generations;
    int max_generations;
    int generation;
    int best_penalty;
    double mean_penalty;
    int first_penalty;
    int improvements;
    int last_improvement;
    int window[LIVE_WINDOW];
    int window_count;
    Individual best;
    long long last_draw_ms;
} LiveView;

int live_available(void);
void live_init(LiveView *view, const BlockList *list, const Dataset *dataset, int period, int delay_ms, int runs,
               const GeneticConfig *config);
void live_start(void);
void live_begin_run(LiveView *view, int run, unsigned long long seed);
void live_update(LiveView *view, const GeneticProgress *progress);
void live_render(const LiveView *view, FILE *output);
void live_draw(LiveView *view);
void live_observe(const GeneticProgress *progress, void *context);
void live_finish(LiveView *view);

#endif
