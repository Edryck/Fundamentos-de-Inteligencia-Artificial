#include <stdlib.h>
#include <string.h>
#include "test_support.h"
#include "../src/genetic.h"
#include "../src/live.h"

static Dataset dataset;
static BlockList list;
static LiveView view;

typedef struct {
    int calls;
    int last_generation;
    int ordered;
    int mean_valid;
} ObserverState;

static void count_progress(const GeneticProgress *progress, void *context)
{
    ObserverState *state = context;

    state->calls++;
    if (progress->generation != state->last_generation + 1) {
        state->ordered = 0;
    }
    if (progress->mean_penalty < progress->best_penalty) {
        state->mean_valid = 0;
    }
    state->last_generation = progress->generation;
}

static void test_observer_keeps_result(void)
{
    GeneticConfig config = genetic_default_config();
    GeneticResult plain;
    GeneticResult observed;
    ObserverState state = {0, 0, 1, 1};
    char error[256];

    config.population_size = 30;
    CHECK(dataset_load(&dataset, "data", 4, error, sizeof(error)) == 0);
    CHECK(blocks_build(&list, &dataset) == 0);
    CHECK(genetic_run(&plain, &config, &list, &dataset, 7) == 0);
    CHECK(genetic_run_observed(&observed, &config, &list, &dataset, 7, count_progress, &state) == 0);
    CHECK(plain.generations == observed.generations);
    CHECK(plain.best.penalty == observed.best.penalty);
    CHECK(memcmp(plain.best.genes, observed.best.genes, sizeof(int) * (size_t)list.block_count) == 0);
    CHECK(state.calls == observed.generations);
    CHECK(state.ordered == 1);
    CHECK(state.mean_valid == 1);
    genetic_result_free(&plain);
    genetic_result_free(&observed);
}

static void test_update_tracks_improvements(void)
{
    GeneticConfig config = genetic_default_config();
    Individual best;
    GeneticProgress progress = {1, 9, 12.0, &best};
    int generation;

    memset(&best, 0, sizeof(best));
    live_init(&view, &list, &dataset, 1, 0, 1, &config);
    live_begin_run(&view, 1, 5);
    live_update(&view, &progress);
    progress.generation = 2;
    progress.best_penalty = 4;
    live_update(&view, &progress);
    progress.generation = 3;
    live_update(&view, &progress);
    CHECK(view.first_penalty == 9);
    CHECK(view.improvements == 1);
    CHECK(view.last_improvement == 2);
    for (generation = 4; generation < 100; generation++) {
        progress.generation = generation;
        live_update(&view, &progress);
    }
    CHECK(view.window_count == LIVE_WINDOW);
    live_begin_run(&view, 2, 6);
    CHECK(view.window_count == 0);
    CHECK(view.improvements == 0);
}

static void test_render_content(void)
{
    GeneticConfig config = genetic_default_config();
    GeneticResult result;
    GeneticProgress progress;
    char *text;
    long size;
    FILE *output = tmpfile();

    config.population_size = 30;
    CHECK(output != NULL);
    CHECK(genetic_run(&result, &config, &list, &dataset, 3) == 0);
    progress.generation = 500;
    progress.best_penalty = result.best.penalty;
    progress.mean_penalty = 2.5;
    progress.best = &result.best;
    live_init(&view, &list, &dataset, 2, 0, 3, &config);
    live_begin_run(&view, 2, 3);
    live_update(&view, &progress);
    live_render(&view, output);
    size = ftell(output);
    rewind(output);
    text = calloc((size_t)size + 1, 1);
    CHECK(text != NULL);
    CHECK(fread(text, 1, (size_t)size, output) == (size_t)size);
    CHECK(strstr(text, "Execucao 2/3") != NULL);
    CHECK(strstr(text, "seed 3") != NULL);
    CHECK(strstr(text, "Periodo 2") != NULL);
    CHECK(strstr(text, "SAB") != NULL);
    CHECK(strstr(text, "CC2") != NULL || strstr(text, "OPH") != NULL);
    free(text);
    fclose(output);
    genetic_result_free(&result);
}

void test_live_run(void)
{
    test_observer_keeps_result();
    test_update_tracks_improvements();
    test_render_content();
}
