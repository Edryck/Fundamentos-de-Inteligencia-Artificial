#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include <stddef.h>

#define CELL_WALL    '#'
#define CELL_SOLID   'S'
#define CELL_SAND    'A'
#define CELL_ROCK    'R'
#define CELL_SWAMP   'P'
#define CELL_START   '@'
#define CELL_GOAL    '*'
#define CELL_REWARD  '$'

#define MAP_MIN_VERTICES 30
#define MAP_MIN_REWARDS  5
#define MAP_MAX_ROWS     100
#define MAP_MAX_COLS     200
#define MAP_ERROR_SIZE   160

typedef struct {
    char **cells;
    int    rows;
    int    cols;
} Map;

/* Aloca um mapa só de paredes, com cada linha terminada em '\0'. */
bool map_allocate(Map *map, int rows, int cols);
void map_set_error(char *error, size_t error_size, const char *format, ...);
int  map_count_walkable(const Map *map);
void map_find_symbol(const Map *map, char symbol, int *row_out, int *col_out);

bool map_load(const char *path, Map *map, char *error, size_t error_size);
bool map_save(const Map *map, const char *path, char *error, size_t error_size);
/* Move o início ('@') ou o prêmio ('*') para outra célula caminhável. A célula antiga volta a ser
   terreno sólido e uma recompensa que estivesse no destino é substituída. */
bool map_move_symbol(Map *map, char symbol, int row, int col, char *error, size_t error_size);
bool map_validate(const Map *map, char *error, size_t error_size);
void map_free(Map *map);

bool cell_is_walkable(char cell);
int  cell_entry_cost(char cell);

#endif
