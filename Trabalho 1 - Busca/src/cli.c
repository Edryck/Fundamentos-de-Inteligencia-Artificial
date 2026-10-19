#include "cli.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_REPEAT        50
#define DEFAULT_BENCH_REPEAT  5
#define DEFAULT_BENCH_SEEDS   50
#define BENCH_SIZES_TEXT_SIZE 96
#define STRINGIFY_VALUE(x)    #x
#define STRINGIFY(x)          STRINGIFY_VALUE(x)
#define ALGO_LIST_SIZE   64

typedef struct {
    const char *key;
    const char *label;
} AlgorithmInfo;

static const AlgorithmInfo ALGORITHMS[ALGO_COUNT] = {
    [ALGO_BFS]        = {"bfs", "BFS"},
    [ALGO_DFS]        = {"dfs", "DFS"},
    [ALGO_UNIFORM_COST] = {"ucs", "Custo Uniforme"},
    [ALGO_GREEDY]     = {"greedy", "Guloso"},
    [ALGO_ASTAR]      = {"astar", "A*"},
    [ALGO_ASTAR_SAFE] = {"astar-safe", "A* seguro"},
};

const char *algorithm_label(Algorithm algorithm) {
    return ALGORITHMS[algorithm].label;
}

static void print_usage(FILE *out) {
    fputs("uso: ./trabalho1 [opções]\n"
          "\n"
          "mapa:\n"
          "  --map ARQUIVO          carrega o mapa de um arquivo em vez de gerar\n"
          "  --save-map ARQUIVO     grava o mapa usado em um arquivo\n"
          "  --seed N               seed do gerador (padrão: horário atual)\n"
          "  --rows R --cols C      dimensões do mapa gerado (padrão: 12x20)\n"
          "  --from LINHA,COLUNA    move o início para outra célula (contagem a partir de 0)\n"
          "  --to LINHA,COLUNA      move o prêmio para outra célula (contagem a partir de 0)\n"
          "  --allow-disconnected   cerca o prêmio com paredes (demonstra \"SEM CAMINHO\")\n"
          "\n"
          "busca:\n"
          "  --algo LISTA           bfs, dfs, ucs, greedy, astar, astar-safe ou all, separados por vírgula\n"
          "  --bonus N              valor de cada recompensa, em custo de terreno (padrão: 10)\n"
          "  --radius N             raio de busca das recompensas, em células (padrão: 3)\n"
          "\n"
          "execução:\n"
          "  --no-anim              mostra só o resultado final de cada rota\n"
          "  --explored             mostra também as células expandidas por cada algoritmo\n"
          "  --repeat N             execuções usadas na média de tempo (padrão: 50; 5 no --bench)\n"
          "\n"
          "benchmark:\n"
          "  --bench                compara os algoritmos em vários mapas gerados (usa --seed como inicial)\n"
          "  --bench-seeds N        mapas por tamanho (padrão: 50)\n"
          "  --bench-sizes LISTA    tamanhos LINHASxCOLUNAS separados por vírgula (padrão: 12x20,24x40,48x80)\n"
          "\n"
          "  -h, --help             mostra esta ajuda\n",
          out);
}

__attribute__((format(printf, 1, 2)))
static void exit_with_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "erro: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\nuse --help para ver as opções\n");
    va_end(args);
    exit(EXIT_FAILURE);
}

static unsigned long long parse_unsigned(const char *flag, const char *text, unsigned long long max) {
    char *end = NULL;
    errno = 0;
    unsigned long long value = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || text[0] == '-' || value > max) {
        exit_with_error("valor inválido para %s", flag);
    }
    return value;
}

static int parse_int(const char *flag, const char *text) {
    return (int)parse_unsigned(flag, text, INT_MAX);
}

static CellChoice parse_cell(const char *flag, const char *text) {
    char *separator = NULL;
    errno = 0;
    unsigned long row = strtoul(text, &separator, 10);
    if (errno != 0 || separator == text || *separator != ',' || text[0] == '-' || row > INT_MAX) {
        exit_with_error("use %s LINHA,COLUNA (ex.: 3,7)", flag);
    }
    CellChoice choice = {true, (int)row, parse_int(flag, separator + 1)};
    return choice;
}

typedef struct {
    int    count;
    char **values;
    int    index;
} Args;

/* Devolve o argumento seguinte à opção atual e avança o cursor. */
static const char *take_value(Args *args) {
    if (args->index + 1 >= args->count) {
        exit_with_error("a opção %s precisa de um valor", args->values[args->index]);
    }
    return args->values[++args->index];
}

static void select_algorithms(Options *options, const char *list) {
    char buffer[ALGO_LIST_SIZE];
    if (strlen(list) >= sizeof buffer) {
        exit_with_error("lista de algoritmos longa demais: %s", list);
    }
    strcpy(buffer, list);
    memset(options->algorithms, 0, sizeof options->algorithms);

    char *save = NULL;
    for (char *token = strtok_r(buffer, ",", &save); token != NULL; token = strtok_r(NULL, ",", &save)) {
        if (strcmp(token, "all") == 0) {
            for (int a = 0; a < ALGO_COUNT; a++) {
                options->algorithms[a] = true;
            }
            continue;
        }
        bool known = false;
        for (int a = 0; a < ALGO_COUNT; a++) {
            if (strcmp(token, ALGORITHMS[a].key) == 0) {
                options->algorithms[a] = true;
                known = true;
            }
        }
        if (!known) {
            exit_with_error("algoritmo desconhecido: %s", token);
        }
    }
}

static BenchSize parse_bench_size(const char *token) {
    char *separator = NULL;
    errno = 0;
    unsigned long rows = strtoul(token, &separator, 10);
    if (errno != 0 || separator == token || *separator != 'x' || token[0] == '-' || rows > INT_MAX) {
        exit_with_error("tamanho inválido em --bench-sizes: %s (use LINHASxCOLUNAS)", token);
    }
    BenchSize size = {(int)rows, parse_int("--bench-sizes", separator + 1)};
    return size;
}

static void select_bench_sizes(Options *options, const char *list) {
    char buffer[BENCH_SIZES_TEXT_SIZE];
    if (strlen(list) >= sizeof buffer) {
        exit_with_error("lista de tamanhos longa demais: %s", list);
    }
    strcpy(buffer, list);
    options->bench_size_count = 0;

    char *save = NULL;
    for (char *token = strtok_r(buffer, ",", &save); token != NULL; token = strtok_r(NULL, ",", &save)) {
        if (options->bench_size_count == MAX_BENCH_SIZES) {
            exit_with_error("tamanhos demais em --bench-sizes (limite %s)", STRINGIFY(MAX_BENCH_SIZES));
        }
        options->bench_sizes[options->bench_size_count++] = parse_bench_size(token);
    }
    if (options->bench_size_count == 0) {
        exit_with_error("%s precisa de pelo menos um tamanho", "--bench-sizes");
    }
}

static void set_default_bench_sizes(Options *options) {
    static const BenchSize defaults[] = {{12, 20}, {24, 40}, {48, 80}};
    options->bench_size_count = (int)(sizeof defaults / sizeof defaults[0]);
    for (int i = 0; i < options->bench_size_count; i++) {
        options->bench_sizes[i] = defaults[i];
    }
}

static Options default_options(void) {
    Options options = {0};
    options.generator.rows = MAP_DEFAULT_ROWS;
    options.generator.cols = MAP_DEFAULT_COLS;
    options.generator.seed = (uint64_t)time(NULL);
    options.heuristic = heuristic_default_config();
    options.animate = true;
    options.bench_seeds = DEFAULT_BENCH_SEEDS;
    set_default_bench_sizes(&options);
    for (int a = 0; a < ALGO_COUNT; a++) {
        options.algorithms[a] = true;
    }
    return options;
}

static int parse_at_least_one(const char *flag, const char *text) {
    int value = parse_int(flag, text);
    if (value < 1) {
        exit_with_error("%s precisa ser pelo menos 1", flag);
    }
    return value;
}

static bool parse_map_flag(Options *options, Args *args, const char *flag) {
    if (strcmp(flag, "--map") == 0) {
        options->map_path = take_value(args);
    } else if (strcmp(flag, "--save-map") == 0) {
        options->save_path = take_value(args);
    } else if (strcmp(flag, "--seed") == 0) {
        options->generator.seed = parse_unsigned(flag, take_value(args), ULLONG_MAX);
    } else if (strcmp(flag, "--rows") == 0) {
        options->generator.rows = parse_int(flag, take_value(args));
    } else if (strcmp(flag, "--cols") == 0) {
        options->generator.cols = parse_int(flag, take_value(args));
    } else if (strcmp(flag, "--from") == 0) {
        options->from = parse_cell(flag, take_value(args));
    } else if (strcmp(flag, "--to") == 0) {
        options->to = parse_cell(flag, take_value(args));
    } else if (strcmp(flag, "--allow-disconnected") == 0) {
        options->generator.isolate_goal = true;
    } else {
        return false;
    }
    return true;
}

static bool parse_search_flag(Options *options, Args *args, const char *flag) {
    if (strcmp(flag, "--algo") == 0) {
        select_algorithms(options, take_value(args));
    } else if (strcmp(flag, "--bonus") == 0) {
        options->heuristic.bonus = parse_int(flag, take_value(args));
    } else if (strcmp(flag, "--radius") == 0) {
        options->heuristic.radius = parse_int(flag, take_value(args));
    } else {
        return false;
    }
    return true;
}

static bool parse_run_flag(Options *options, Args *args, const char *flag) {
    if (strcmp(flag, "--no-anim") == 0) {
        options->animate = false;
    } else if (strcmp(flag, "--explored") == 0) {
        options->show_explored = true;
    } else if (strcmp(flag, "--repeat") == 0) {
        options->repeat = parse_at_least_one(flag, take_value(args));
    } else if (strcmp(flag, "--bench") == 0) {
        options->bench = true;
    } else if (strcmp(flag, "--bench-seeds") == 0) {
        options->bench_seeds = parse_at_least_one(flag, take_value(args));
    } else if (strcmp(flag, "--bench-sizes") == 0) {
        select_bench_sizes(options, take_value(args));
    } else {
        return false;
    }
    return true;
}

Options cli_parse(int argc, char **argv) {
    Options options = default_options();
    Args args = {argc, argv, 1};

    for (; args.index < argc; args.index++) {
        const char *flag = argv[args.index];
        if (strcmp(flag, "-h") == 0 || strcmp(flag, "--help") == 0) {
            print_usage(stdout);
            exit(EXIT_SUCCESS);
        }
        if (!parse_map_flag(&options, &args, flag) && !parse_search_flag(&options, &args, flag) &&
            !parse_run_flag(&options, &args, flag)) {
            exit_with_error("opção desconhecida '%s'", flag);
        }
    }

    if (options.bench && (options.map_path != NULL || options.from.set || options.to.set)) {
        exit_with_error("%s gera os próprios mapas e não combina com --map, --from nem --to", "--bench");
    }
    if (options.repeat == 0) {
        options.repeat = options.bench ? DEFAULT_BENCH_REPEAT : DEFAULT_REPEAT;
    }
    return options;
}
