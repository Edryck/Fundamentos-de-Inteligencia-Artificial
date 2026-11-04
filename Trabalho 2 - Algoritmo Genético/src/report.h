#ifndef REPORT_H
#define REPORT_H

#include <stdio.h>
#include "data.h"
#include "chromosome.h"
#include "genetic.h"
#include "real_schedule.h"

typedef struct {
    int best_penalty;
    double mean_penalty;
    double standard_deviation;
    double conflict_free_rate;
    int runs;
} RunSummary;

void summary_compute(RunSummary *summary, const int *penalties, int runs);
void report_print_schedule(FILE *output, const Individual *best, const BlockList *list, const Dataset *dataset, int periods);
int report_write_html(const char *path, const Individual *best, const BlockList *list, const Dataset *dataset,
                      int periods, const RealSchedule *real, unsigned long long seed);
int report_write_history(const char *path, const GeneticResult *result);
int report_write_summary(const char *path, const RunSummary *summary);
int report_ensure_directory(const char *path);

#endif
