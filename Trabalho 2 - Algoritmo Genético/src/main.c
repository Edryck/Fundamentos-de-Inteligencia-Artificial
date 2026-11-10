#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "cli.h"
#include "data.h"
#include "chromosome.h"
#include "genetic.h"
#include "report.h"
#include "real_schedule.h"
#include "live.h"

static Dataset dataset;
static BlockList list;
static RealSchedule real;
static LiveView live;

int main(int argc, char **argv)
{
    CliOptions options;
    char error[256];
    int run;
    int *penalties;
    unsigned long long best_seed = 0;
    unsigned long long seed;
    GeneticResult result;
    GeneticResult best_result = {{{0}, 0}, 0, NULL};
    RunSummary summary;
    struct timespec start;
    struct timespec finish;
    double elapsed;
    int found = 0;

    error[0] = '\0';
    if (cli_parse(&options, argc, argv, error, sizeof(error)) != 0) {
        fprintf(stderr, "error: %s\n", error);
        return 1;
    }
    if (!options.seed_given) {
        options.seed = (unsigned long long)time(NULL) * 1000003ULL + (unsigned long long)getpid();
        printf("no --seed given, using seed %llu\n", options.seed);
    }
    if (options.periods == 8 && real_schedule_load(&real, "data", error, sizeof(error)) != 0) {
        fprintf(stderr, "error: cannot load real schedule data: %s\n", error);
        return 1;
    }
    if (dataset_load(&dataset, "data", options.periods, error, sizeof(error)) != 0) {
        fprintf(stderr, "error: %s\n", error);
        return 1;
    }
    if (blocks_build(&list, &dataset) != 0) {
        fprintf(stderr, "error: too many blocks\n");
        return 1;
    }
    penalties = malloc(sizeof(int) * (size_t)options.runs);
    if (penalties == NULL) {
        fprintf(stderr, "error: out of memory\n");
        return 1;
    }

    if (options.live && !live_available()) {
        fprintf(stderr, "warning: --live needs a terminal of at least %d columns and %d lines, using plain output\n",
                LIVE_MIN_COLUMNS, LIVE_MIN_LINES);
        options.live = 0;
    }
    if (options.live) {
        live_init(&live, &list, &dataset, options.live_period, options.delay_ms, options.runs, &options.genetic);
        live_start();
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (run = 0; run < options.runs; run++) {
        seed = options.seed + (unsigned long long)run;
        if (options.live) {
            live_begin_run(&live, run + 1, seed);
        }
        if (genetic_run_observed(&result, &options.genetic, &list, &dataset, seed,
                                 options.live ? live_observe : NULL, &live) != 0) {
            if (options.live) {
                live_finish(&live);
            }
            fprintf(stderr, "error: genetic run failed\n");
            if (found) {
                genetic_result_free(&best_result);
            }
            free(penalties);
            return 1;
        }
        penalties[run] = result.best.penalty;
        if (options.live) {
            live_draw(&live);
        } else {
            printf("run %d: seed %llu, generations %d, penalty %d\n", run + 1, seed,
                   result.generations, result.best.penalty);
        }
        if (!found || result.best.penalty < best_result.best.penalty) {
            if (found) {
                genetic_result_free(&best_result);
            }
            best_result = result;
            best_seed = seed;
            found = 1;
        } else {
            genetic_result_free(&result);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
    if (options.live) {
        live_finish(&live);
    }
    elapsed = (double)(finish.tv_sec - start.tv_sec) + (double)(finish.tv_nsec - start.tv_nsec) / 1e9;
    summary_compute(&summary, penalties, options.runs);
    free(penalties);

    printf("seed: %llu\n", best_seed);
    printf("generations: %d\n", best_result.generations);
    printf("final penalty: %d\n", best_result.best.penalty);
    printf("elapsed seconds: %.3f\n", elapsed);
    report_print_schedule(stdout, &best_result.best, &list, &dataset, options.periods);

    if (best_result.best.penalty > 0) {
        fprintf(stderr, "warning: final penalty is above 0\n");
    }
    if (report_ensure_directory("results") != 0) {
        fprintf(stderr, "error: could not create results directory\n");
        genetic_result_free(&best_result);
        return 1;
    }
    if (options.periods == 8
        && report_write_html("results/schedule.html", &best_result.best, &list, &dataset, options.periods,
                             &real, best_seed) != 0) {
        fprintf(stderr, "error: could not write results files\n");
        genetic_result_free(&best_result);
        return 1;
    }
    if (report_write_history("results/history.csv", &best_result) != 0
        || report_write_summary("results/summary.csv", &summary) != 0) {
        fprintf(stderr, "error: could not write results files\n");
        genetic_result_free(&best_result);
        return 1;
    }
    genetic_result_free(&best_result);
    if (options.periods == 8) {
        printf("html: results/schedule.html\n");
    } else {
        printf("results/schedule.html is only generated with --periods 8\n");
    }
    return 0;
}
