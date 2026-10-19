#ifndef METRICS_H
#define METRICS_H

#include "search.h"

#define METRICS_CELL_SIZE 32

typedef struct {
    const char         *name;
    const SearchResult *result;
} MetricsRow;

double metrics_now_us(void);

/* Imprime a tabela comparativa de uma execução, com as colunas alinhadas. */
void metrics_print_table(const MetricsRow *rows, int count);

/* Imprime qualquer tabela de texto. `cells` tem row_count * column_count textos de
   até METRICS_CELL_SIZE bytes, linha por linha. A primeira coluna fica à esquerda. */
void metrics_print_grid(int column_count, const char *const *headers, int row_count, const char *cells);

#endif
