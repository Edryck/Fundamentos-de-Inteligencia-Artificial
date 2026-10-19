#include "map.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COST_SOLID 1
#define COST_SAND  4
#define COST_ROCK  10
#define COST_SWAMP 20
#define COST_WALL  (-1)

#define MAP_SYMBOLS "#SARP@*$."
#define CELL_ALIAS_SOLID '.'

void map_set_error(char *error, size_t error_size, const char *format, ...) {
    if (error == NULL || error_size == 0) {
        return;
    }
    va_list args;
    va_start(args, format);
    vsnprintf(error, error_size, format, args);
    va_end(args);
}

/* ---------- células ---------- */

bool cell_is_walkable(char cell) {
    return cell != CELL_WALL;
}

int cell_entry_cost(char cell) {
    switch (cell) {
    case CELL_SAND:
        return COST_SAND;
    case CELL_ROCK:
        return COST_ROCK;
    case CELL_SWAMP:
        return COST_SWAMP;
    case CELL_WALL:
        return COST_WALL;
    default:
        return COST_SOLID;
    }
}

/* ---------- alocação ---------- */

void map_free(Map *map) {
    if (map->cells != NULL) {
        for (int row = 0; row < map->rows; row++) {
            free(map->cells[row]);
        }
        free(map->cells);
    }
    map->cells = NULL;
    map->rows = 0;
    map->cols = 0;
}

/* Todas as linhas ficam terminadas em '\0' para poderem ser impressas e salvas direto. */
bool map_allocate(Map *map, int rows, int cols) {
    map->cells = calloc((size_t)rows, sizeof(char *));
    map->rows = rows;
    map->cols = cols;
    if (map->cells == NULL) {
        map->rows = 0;
        return false;
    }
    for (int row = 0; row < rows; row++) {
        map->cells[row] = malloc((size_t)cols + 1);
        if (map->cells[row] == NULL) {
            map_free(map);
            return false;
        }
        memset(map->cells[row], CELL_WALL, (size_t)cols);
        map->cells[row][cols] = '\0';
    }
    return true;
}

int map_count_walkable(const Map *map) {
    int total = 0;
    for (int row = 0; row < map->rows; row++) {
        for (int col = 0; col < map->cols; col++) {
            if (cell_is_walkable(map->cells[row][col])) {
                total++;
            }
        }
    }
    return total;
}

void map_find_symbol(const Map *map, char symbol, int *row_out, int *col_out) {
    for (int row = 0; row < map->rows; row++) {
        for (int col = 0; col < map->cols; col++) {
            if (map->cells[row][col] == symbol) {
                *row_out = row;
                *col_out = col;
                return;
            }
        }
    }
}

/* ---------- validação ---------- */

bool map_validate(const Map *map, char *error, size_t error_size) {
    int starts = 0;
    int goals = 0;
    int rewards = 0;

    for (int row = 0; row < map->rows; row++) {
        for (int col = 0; col < map->cols; col++) {
            char cell = map->cells[row][col];
            starts += (cell == CELL_START);
            goals += (cell == CELL_GOAL);
            rewards += (cell == CELL_REWARD);
        }
    }

    if (starts != 1) {
        map_set_error(error, error_size, "o mapa precisa ter exatamente um início '%c' (encontrados: %d)",
                  CELL_START, starts);
        return false;
    }
    if (goals != 1) {
        map_set_error(error, error_size, "o mapa precisa ter exatamente um prêmio '%c' (encontrados: %d)",
                  CELL_GOAL, goals);
        return false;
    }
    if (rewards < MAP_MIN_REWARDS) {
        map_set_error(error, error_size, "o mapa tem %d recompensas '%c', o mínimo é %d",
                  rewards, CELL_REWARD, MAP_MIN_REWARDS);
        return false;
    }
    int vertices = map_count_walkable(map);
    if (vertices < MAP_MIN_VERTICES) {
        map_set_error(error, error_size, "o mapa tem %d células caminháveis, o mínimo é %d",
                  vertices, MAP_MIN_VERTICES);
        return false;
    }
    return true;
}

/* ---------- reposicionamento de início e prêmio ---------- */

bool map_move_symbol(Map *map, char symbol, int row, int col, char *error, size_t error_size) {
    char other = (symbol == CELL_START) ? CELL_GOAL : CELL_START;

    if (row < 0 || row >= map->rows || col < 0 || col >= map->cols) {
        map_set_error(error, error_size, "posição %d,%d fora do mapa (%d linhas, %d colunas)", row, col,
                  map->rows, map->cols);
        return false;
    }
    if (!cell_is_walkable(map->cells[row][col])) {
        map_set_error(error, error_size, "a posição %d,%d é uma parede", row, col);
        return false;
    }
    if (map->cells[row][col] == other) {
        map_set_error(error, error_size, "a posição %d,%d já é ocupada por '%c'", row, col, other);
        return false;
    }

    int old_row = 0;
    int old_col = 0;
    map_find_symbol(map, symbol, &old_row, &old_col);
    map->cells[old_row][old_col] = CELL_SOLID;
    map->cells[row][col] = symbol;
    return map_validate(map, error, error_size);
}

/* ---------- carga e gravação em arquivo ---------- */

/* Converte '.' em solo e rejeita qualquer símbolo fora da tabela do mapa. */
static bool normalize_line(char *line, size_t length, int line_number, char *error, size_t error_size) {
    for (size_t col = 0; col < length; col++) {
        if (line[col] == CELL_ALIAS_SOLID) {
            line[col] = CELL_SOLID;
        } else if (strchr(MAP_SYMBOLS, line[col]) == NULL) {
            map_set_error(error, error_size, "caractere inválido '%c' na linha %d, coluna %zu",
                      line[col], line_number, col + 1);
            return false;
        }
    }
    return true;
}

static void free_lines(char **lines, int count) {
    for (int i = 0; i < count; i++) {
        free(lines[i]);
    }
    free(lines);
}

static bool append_line(char **lines, int *count, const char *line, size_t length) {
    lines[*count] = malloc(length + 1);
    if (lines[*count] == NULL) {
        return false;
    }
    memcpy(lines[*count], line, length);
    lines[*count][length] = '\0';
    (*count)++;
    return true;
}

static bool read_lines(FILE *file, char **lines, int *row_count, int *col_count, char *error, size_t error_size) {
    char buffer[MAP_MAX_COLS + 3];
    int line_number = 0;

    while (fgets(buffer, sizeof buffer, file) != NULL) {
        line_number++;
        size_t length = strcspn(buffer, "\r\n");
        if (buffer[length] == '\0' && !feof(file)) {
            map_set_error(error, error_size, "linha %d maior que o limite de %d colunas", line_number, MAP_MAX_COLS);
            return false;
        }
        if (length == 0) {
            continue;
        }
        if (*row_count == MAP_MAX_ROWS) {
            map_set_error(error, error_size, "o mapa passa do limite de %d linhas", MAP_MAX_ROWS);
            return false;
        }
        if (*row_count == 0) {
            *col_count = (int)length;
        } else if ((int)length != *col_count) {
            map_set_error(error, error_size, "a linha %d tem %zu colunas, o esperado era %d",
                      line_number, length, *col_count);
            return false;
        }
        buffer[length] = '\0';
        if (!normalize_line(buffer, length, line_number, error, error_size)) {
            return false;
        }
        if (!append_line(lines, row_count, buffer, length)) {
            map_set_error(error, error_size, "sem memória para ler o mapa");
            return false;
        }
    }
    return true;
}

bool map_load(const char *path, Map *map, char *error, size_t error_size) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        map_set_error(error, error_size, "não foi possível abrir '%s'", path);
        return false;
    }

    char **lines = calloc(MAP_MAX_ROWS, sizeof(char *));
    int row_count = 0;
    int col_count = 0;
    if (lines == NULL) {
        fclose(file);
        map_set_error(error, error_size, "sem memória para ler o mapa");
        return false;
    }

    bool read_ok = read_lines(file, lines, &row_count, &col_count, error, error_size);
    fclose(file);

    if (read_ok && row_count == 0) {
        map_set_error(error, error_size, "o arquivo '%s' está vazio", path);
        read_ok = false;
    }
    if (!read_ok) {
        free_lines(lines, row_count);
        return false;
    }

    map->cells = lines;
    map->rows = row_count;
    map->cols = col_count;
    if (!map_validate(map, error, error_size)) {
        map_free(map);
        return false;
    }
    return true;
}

bool map_save(const Map *map, const char *path, char *error, size_t error_size) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        map_set_error(error, error_size, "não foi possível criar '%s'", path);
        return false;
    }
    for (int row = 0; row < map->rows; row++) {
        fprintf(file, "%s\n", map->cells[row]);
    }
    if (fclose(file) != 0) {
        map_set_error(error, error_size, "falha ao gravar '%s'", path);
        return false;
    }
    return true;
}
