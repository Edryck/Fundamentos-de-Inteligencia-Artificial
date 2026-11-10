#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include "live.h"

#define SLOTS_PER_DAY 12
#define SLOTS_PER_HALF 6
#define DAY_COUNT 6
#define BAR_WIDTH 40
#define MIN_DRAW_INTERVAL_MS 33
#define RESET "\033[0m"
#define BOLD "\033[1m"
#define DIM "\033[2m"
#define RED "\033[1;37;41m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define CYAN "\033[1;36m"
#define LINE_END "\033[K\n"

static const char *const DAY_NAMES[DAY_COUNT] = {"SEG", "TER", "QUA", "QUI", "SEX", "SAB"};
static const char *const SPARKS[8] = {"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
static const int COURSE_COLORS[6] = {31, 32, 33, 34, 35, 36};

static long long now_ms(void)
{
    struct timespec time_now;

    clock_gettime(CLOCK_MONOTONIC, &time_now);
    return (long long)time_now.tv_sec * 1000 + time_now.tv_nsec / 1000000;
}

static void restore_and_exit(int signal_number)
{
    static const char restore[] = "\033[0m\033[?25h\n";
    ssize_t written;

    (void)signal_number;
    written = write(STDOUT_FILENO, restore, sizeof(restore) - 1);
    (void)written;
    _exit(130);
}

int live_available(void)
{
    struct winsize size;

    if (!isatty(STDOUT_FILENO) || ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) != 0) {
        return 0;
    }
    return size.ws_col >= LIVE_MIN_COLUMNS && size.ws_row >= LIVE_MIN_LINES;
}

void live_init(LiveView *view, const BlockList *list, const Dataset *dataset, int period, int delay_ms, int runs,
               const GeneticConfig *config)
{
    memset(view, 0, sizeof(*view));
    view->list = list;
    view->dataset = dataset;
    view->period = period;
    view->delay_ms = delay_ms;
    view->runs = runs;
    view->population_size = config->population_size;
    view->crossover_rate = config->crossover_rate;
    view->mutation_rate = config->mutation_rate;
    view->min_generations = config->min_generations;
    view->max_generations = config->max_generations;
}

void live_start(void)
{
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = restore_and_exit;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    fputs("\033[2J\033[?25l", stdout);
    fflush(stdout);
}

void live_begin_run(LiveView *view, int run, unsigned long long seed)
{
    view->run = run;
    view->seed = seed;
    view->generation = 0;
    view->improvements = 0;
    view->last_improvement = 0;
    view->window_count = 0;
    view->last_draw_ms = 0;
}

void live_update(LiveView *view, const GeneticProgress *progress)
{
    if (view->generation == 0) {
        view->first_penalty = progress->best_penalty;
    } else if (progress->best_penalty < view->best_penalty) {
        view->improvements++;
        view->last_improvement = progress->generation;
    }
    view->generation = progress->generation;
    view->best_penalty = progress->best_penalty;
    view->mean_penalty = progress->mean_penalty;
    view->best = *progress->best;
    if (view->window_count == LIVE_WINDOW) {
        memmove(view->window, view->window + 1, sizeof(int) * (LIVE_WINDOW - 1));
        view->window_count--;
    }
    view->window[view->window_count++] = progress->best_penalty;
}

static void render_bar(const LiveView *view, FILE *output)
{
    int target = view->generation < view->min_generations ? view->min_generations : view->max_generations;
    int filled;
    int index;

    if (view->best_penalty == 0 && view->generation >= view->min_generations) {
        target = view->generation;
    }
    filled = target > 0 ? view->generation * BAR_WIDTH / target : 0;
    fprintf(output, " Geracao %7d/%-7d " CYAN, view->generation, target);
    for (index = 0; index < BAR_WIDTH; index++) {
        fputs(index < filled ? "█" : "░", output);
    }
    fputs(RESET LINE_END, output);
}

static void render_sparkline(const LiveView *view, FILE *output)
{
    int highest = 1;
    int index;

    for (index = 0; index < view->window_count; index++) {
        if (view->window[index] > highest) {
            highest = view->window[index];
        }
    }
    fputs(" Evolucao " YELLOW, output);
    for (index = 0; index < view->window_count; index++) {
        fputs(SPARKS[view->window[index] * 7 / highest], output);
    }
    fputs(RESET LINE_END, output);
}

static void render_grid(const LiveView *view, FILE *output)
{
    int cells[SLOTS_PER_DAY][DAY_COUNT];
    int counts[SLOTS_PER_DAY][DAY_COUNT];
    int block_index;
    int slot;
    int offset;
    int day;
    int course;

    memset(cells, 0, sizeof(cells));
    memset(counts, 0, sizeof(counts));
    for (block_index = 0; block_index < view->list->block_count; block_index++) {
        const Block *block = &view->list->blocks[block_index];

        if (view->dataset->courses[block->course].period != view->period) {
            continue;
        }
        for (slot = view->best.genes[block_index]; slot < view->best.genes[block_index] + block->length; slot++) {
            cells[slot % SLOTS_PER_DAY][slot / SLOTS_PER_DAY] = block->course;
            counts[slot % SLOTS_PER_DAY][slot / SLOTS_PER_DAY]++;
        }
    }
    fprintf(output, BOLD " Periodo %d" RESET DIM "  melhor individuo; vermelho = choque na turma" RESET LINE_END, view->period);
    fprintf(output, BOLD "    ");
    for (day = 0; day < DAY_COUNT; day++) {
        fprintf(output, " %-10s", DAY_NAMES[day]);
    }
    fputs(RESET LINE_END, output);
    for (offset = 0; offset < SLOTS_PER_DAY; offset++) {
        fprintf(output, DIM " %c%d " RESET, offset < SLOTS_PER_HALF ? 'M' : 'T', offset % SLOTS_PER_HALF + 1);
        for (day = 0; day < DAY_COUNT; day++) {
            if (counts[offset][day] == 0) {
                fprintf(output, DIM " %-10s" RESET, ".");
                continue;
            }
            course = cells[offset][day];
            if (counts[offset][day] > 1) {
                fprintf(output, " " RED "%-10s" RESET, view->dataset->courses[course].code);
            } else {
                fprintf(output, " \033[%dm%-10s" RESET, COURSE_COLORS[course % 6], view->dataset->courses[course].code);
            }
        }
        fputs(LINE_END, output);
    }
}

void live_render(const LiveView *view, FILE *output)
{
    const char *color = view->best_penalty == 0 ? GREEN : YELLOW;

    fputs("\033[H", output);
    fprintf(output, BOLD CYAN " ALGORITMO GENETICO - GRADE DE HORARIOS" RESET LINE_END);
    fprintf(output, " Execucao %d/%d   seed %llu" LINE_END, view->run, view->runs, view->seed);
    fprintf(output, DIM " populacao %d   crossover %.2f   mutacao %.2f" RESET LINE_END,
            view->population_size, view->crossover_rate, view->mutation_rate);
    render_bar(view, output);
    fprintf(output, " Penalidade %s%d" RESET "   media %.1f   inicial %d" LINE_END,
            color, view->best_penalty, view->mean_penalty, view->first_penalty);
    fprintf(output, " Melhorias %d   ultima na geracao %d" LINE_END, view->improvements, view->last_improvement);
    render_sparkline(view, output);
    fputs(LINE_END, output);
    render_grid(view, output);
    fputs("\033[J", output);
}

void live_draw(LiveView *view)
{
    live_render(view, stdout);
    fflush(stdout);
    view->last_draw_ms = now_ms();
}

void live_observe(const GeneticProgress *progress, void *context)
{
    LiveView *view = context;
    struct timespec pause;

    live_update(view, progress);
    if (view->delay_ms > 0 || now_ms() - view->last_draw_ms >= MIN_DRAW_INTERVAL_MS) {
        live_draw(view);
    }
    if (view->delay_ms > 0) {
        pause.tv_sec = view->delay_ms / 1000;
        pause.tv_nsec = (long)(view->delay_ms % 1000) * 1000000L;
        nanosleep(&pause, NULL);
    }
}

void live_finish(LiveView *view)
{
    (void)view;
    fputs("\033[?25h\n", stdout);
    fflush(stdout);
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
}
