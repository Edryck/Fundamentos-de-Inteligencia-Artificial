#include "metrics.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MICROS_PER_SECOND 1000000.0
#define NANOS_PER_MICRO   1000.0

#define RUN_COLUMN_COUNT 9

static const char *const RUN_HEADERS[RUN_COLUMN_COUNT] = {
    "Algoritmo", "Passos", "Custo", "Expandidos", "Gerados", "Fronteira máx.",
    "$ coletados", "$ vistos", "Tempo (μs)"
};

/* Relógio monotônico: não sofre ajustes de horário do sistema durante a medição. */
double metrics_now_us(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)now.tv_sec * MICROS_PER_SECOND + (double)now.tv_nsec / NANOS_PER_MICRO;
}

/* printf conta bytes, e letras acentuadas ocupam mais de um byte em UTF-8. Para alinhar
   as colunas é preciso contar caracteres, ignorando os bytes de continuação (10xxxxxx). */
static int display_width(const char *text) {
    int width = 0;
    for (const char *c = text; *c != '\0'; c++) {
        if (((unsigned char)*c & 0xC0) != 0x80) {
            width++;
        }
    }
    return width;
}

static void print_cell(const char *text, int width, bool align_left) {
    int padding = width - display_width(text);
    if (align_left) {
        printf("| %s%*s ", text, padding, "");
    } else {
        printf("| %*s%s ", padding, "", text);
    }
}

static void print_line(int column_count, const char *const *texts, const int *widths) {
    for (int column = 0; column < column_count; column++) {
        print_cell(texts[column], widths[column], column == 0);
    }
    printf("|\n");
}

static void print_separator(int column_count, const int *widths) {
    for (int column = 0; column < column_count; column++) {
        putchar('+');
        for (int i = 0; i < widths[column] + 2; i++) {
            putchar('-');
        }
    }
    printf("+\n");
}

void metrics_print_grid(int column_count, const char *const *headers, int row_count, const char *cells) {
    int *widths = malloc((size_t)column_count * sizeof(int));
    const char **line = malloc((size_t)column_count * sizeof(char *));
    if (widths == NULL || line == NULL) {
        fprintf(stderr, "erro: sem memória para imprimir a tabela\n");
        exit(EXIT_FAILURE);
    }

    for (int column = 0; column < column_count; column++) {
        widths[column] = display_width(headers[column]);
        for (int row = 0; row < row_count; row++) {
            int width = display_width(cells + ((size_t)row * column_count + column) * METRICS_CELL_SIZE);
            widths[column] = width > widths[column] ? width : widths[column];
        }
    }

    print_separator(column_count, widths);
    print_line(column_count, headers, widths);
    print_separator(column_count, widths);
    for (int row = 0; row < row_count; row++) {
        for (int column = 0; column < column_count; column++) {
            line[column] = cells + ((size_t)row * column_count + column) * METRICS_CELL_SIZE;
        }
        print_line(column_count, line, widths);
    }
    print_separator(column_count, widths);

    free(line);
    free(widths);
}

static void format_row(const MetricsRow *row, char cells[RUN_COLUMN_COUNT][METRICS_CELL_SIZE]) {
    const SearchResult *result = row->result;
    char steps[METRICS_CELL_SIZE] = "-";
    char cost[METRICS_CELL_SIZE] = "-";
    char collected[METRICS_CELL_SIZE] = "-";

    if (result->found) {
        snprintf(steps, sizeof steps, "%d", result->path_len);
        snprintf(cost, sizeof cost, "%ld", result->cost);
        snprintf(collected, sizeof collected, "%d", result->rewards_collected);
    }

    snprintf(cells[0], METRICS_CELL_SIZE, "%s", row->name);
    snprintf(cells[1], METRICS_CELL_SIZE, "%s", steps);
    snprintf(cells[2], METRICS_CELL_SIZE, "%s", cost);
    snprintf(cells[3], METRICS_CELL_SIZE, "%d", result->expanded);
    snprintf(cells[4], METRICS_CELL_SIZE, "%d", result->generated);
    snprintf(cells[5], METRICS_CELL_SIZE, "%d", result->peak_frontier);
    snprintf(cells[6], METRICS_CELL_SIZE, "%s", collected);
    snprintf(cells[7], METRICS_CELL_SIZE, "%d", result->rewards_seen);
    snprintf(cells[8], METRICS_CELL_SIZE, "%.1f", result->time_us);
}

void metrics_print_table(const MetricsRow *rows, int count) {
    char *cells = malloc((size_t)count * RUN_COLUMN_COUNT * METRICS_CELL_SIZE);
    if (cells == NULL) {
        fprintf(stderr, "erro: sem memória para imprimir a tabela\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < count; i++) {
        format_row(&rows[i], (char(*)[METRICS_CELL_SIZE])(cells + (size_t)i * RUN_COLUMN_COUNT * METRICS_CELL_SIZE));
    }
    metrics_print_grid(RUN_COLUMN_COUNT, RUN_HEADERS, count, cells);
    free(cells);
}
