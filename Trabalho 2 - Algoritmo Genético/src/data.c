#include "data.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 512
#define MAX_FIELDS 8

static const char *DAY_NAMES[6] = {"MON", "TUE", "WED", "THU", "FRI", "SAT"};

static void set_error(char *error, size_t error_size, const char *format, ...)
{
    va_list args;

    if (error != NULL && error_size > 0) {
        va_start(args, format);
        vsnprintf(error, error_size, format, args);
        va_end(args);
    }
}

int slot_parse(const char *text)
{
    int day = -1;
    int i;
    int index;

    if (text == NULL || strlen(text) != 6 || text[3] != '-') {
        return -1;
    }
    for (i = 0; i < 6; i++) {
        if (strncmp(text, DAY_NAMES[i], 3) == 0) {
            day = i;
        }
    }
    if (day < 0) {
        return -1;
    }
    if (text[5] < '1' || text[5] > '6') {
        return -1;
    }
    index = text[5] - '1';
    if (text[4] == 'M') {
        return day * 12 + index;
    }
    if (text[4] == 'T' && day < 5) {
        return day * 12 + 6 + index;
    }
    return -1;
}

void slot_format(int slot, char *text, size_t text_size)
{
    int day;
    int offset;

    if (text == NULL || text_size == 0) {
        return;
    }
    if (slot < 0 || slot >= SLOT_COUNT) {
        snprintf(text, text_size, "INVALID");
        return;
    }
    day = slot / 12;
    offset = slot % 12;
    snprintf(text, text_size, "%s-%c%d", DAY_NAMES[day], offset < 6 ? 'M' : 'T', offset % 6 + 1);
}

static int split(char *line, char delimiter, char **fields, int max_fields)
{
    int count = 0;
    char *cursor = line;

    fields[count++] = cursor;
    while (*cursor != '\0') {
        if (*cursor == delimiter && count < max_fields) {
            *cursor = '\0';
            fields[count++] = cursor + 1;
        }
        cursor++;
    }
    return count;
}

static void trim(char *text)
{
    size_t length = strlen(text);
    size_t start = 0;

    while (length > 0 && (text[length - 1] == ' ' || text[length - 1] == '\t' ||
                          text[length - 1] == '\r' || text[length - 1] == '\n')) {
        text[--length] = '\0';
    }
    while (text[start] == ' ' || text[start] == '\t') {
        start++;
    }
    if (start > 0) {
        memmove(text, text + start, strlen(text + start) + 1);
    }
}

static int parse_int(const char *text, int *value)
{
    char *end;
    long parsed;

    if (text[0] == '\0') {
        return -1;
    }
    parsed = strtol(text, &end, 10);
    if (*end != '\0') {
        return -1;
    }
    *value = (int)parsed;
    return 0;
}

static FILE *open_csv(const char *directory, const char *name, char *error, size_t error_size)
{
    char path[1024];
    FILE *file;

    snprintf(path, sizeof(path), "%s/%s", directory, name);
    file = fopen(path, "r");
    if (file == NULL) {
        set_error(error, error_size, "cannot open file %s", path);
    }
    return file;
}

static int read_record(FILE *file, char *line, size_t size, char **fields, int max_fields, int *line_number)
{
    while (fgets(line, (int)size, file) != NULL) {
        (*line_number)++;
        trim(line);
        if (line[0] != '\0') {
            return split(line, ',', fields, max_fields);
        }
    }
    return 0;
}

static int find_professor(const Dataset *dataset, const char *name)
{
    int i;

    for (i = 0; i < dataset->professor_count; i++) {
        if (strcmp(dataset->professors[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

static int add_professor(Dataset *dataset, const char *name)
{
    int index = find_professor(dataset, name);

    if (index >= 0) {
        return index;
    }
    if (dataset->professor_count >= MAX_PROFESSORS || strlen(name) >= sizeof(dataset->professors[0])) {
        return -1;
    }
    strcpy(dataset->professors[dataset->professor_count], name);
    return dataset->professor_count++;
}

typedef struct {
    char names[MAX_PROFESSORS * 4][32];
    int count;
} NamePool;

static int pool_add(NamePool *pool, const char *name)
{
    int i;

    for (i = 0; i < pool->count; i++) {
        if (strcmp(pool->names[i], name) == 0) {
            return 0;
        }
    }
    if (pool->count >= MAX_PROFESSORS * 4 || strlen(name) >= sizeof(pool->names[0])) {
        return -1;
    }
    strcpy(pool->names[pool->count++], name);
    return 0;
}

static int pool_has(const NamePool *pool, const char *name)
{
    int i;

    for (i = 0; i < pool->count; i++) {
        if (strcmp(pool->names[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

static int load_courses(Dataset *dataset, const char *directory, int max_period,
                        char all_codes[][16], int *all_count, int *kept_index,
                        NamePool *pool, char *error, size_t error_size)
{
    FILE *file = open_csv(directory, "courses.csv", error, error_size);
    char line[LINE_SIZE];
    char *fields[MAX_FIELDS];
    char *names[MAX_COURSE_PROFESSORS + 1];
    int line_number = 0;
    int count;

    if (file == NULL) {
        return -1;
    }
    read_record(file, line, sizeof(line), fields, MAX_FIELDS, &line_number);
    while ((count = read_record(file, line, sizeof(line), fields, MAX_FIELDS, &line_number)) > 0) {
        Course course;
        int lessons;
        int period;
        int saturday;
        int name_count;
        int i;

        memset(&course, 0, sizeof(course));
        if (count != 5) {
            set_error(error, error_size, "courses.csv line %d: expected 5 fields", line_number);
            fclose(file);
            return -1;
        }
        for (i = 0; i < count; i++) {
            trim(fields[i]);
        }
        if (fields[0][0] == '\0' || strlen(fields[0]) >= sizeof(course.code) ||
            parse_int(fields[1], &period) != 0 || period < 1 ||
            parse_int(fields[2], &lessons) != 0 ||
            (lessons != 2 && lessons != 3 && lessons != 4 && lessons != 6 && lessons != 12) ||
            parse_int(fields[4], &saturday) != 0 || (saturday != 0 && saturday != 1)) {
            set_error(error, error_size, "courses.csv line %d: invalid value", line_number);
            fclose(file);
            return -1;
        }
        if (*all_count >= MAX_COURSES) {
            set_error(error, error_size, "courses.csv: too many courses");
            fclose(file);
            return -1;
        }
        strcpy(all_codes[*all_count], fields[0]);
        kept_index[*all_count] = -1;
        strcpy(course.code, fields[0]);
        course.period = period;
        course.weekly_lessons = lessons;
        course.saturday = saturday;
        name_count = split(fields[3], ';', names, MAX_COURSE_PROFESSORS + 1);
        if (name_count > MAX_COURSE_PROFESSORS) {
            set_error(error, error_size, "courses.csv line %d: too many professors", line_number);
            fclose(file);
            return -1;
        }
        for (i = 0; i < name_count; i++) {
            trim(names[i]);
            if (names[i][0] != '\0' && pool_add(pool, names[i]) != 0) {
                set_error(error, error_size, "courses.csv line %d: invalid professor", line_number);
                fclose(file);
                return -1;
            }
        }
        if (max_period > 0 && period > max_period) {
            (*all_count)++;
            continue;
        }
        for (i = 0; i < name_count; i++) {
            int professor;
            professor = names[i][0] == '\0' ? -1 : add_professor(dataset, names[i]);
            if (professor < 0) {
                set_error(error, error_size, "courses.csv line %d: invalid professor", line_number);
                fclose(file);
                return -1;
            }
            course.professors[i] = professor;
        }
        course.professor_count = name_count;
        kept_index[*all_count] = dataset->course_count;
        dataset->courses[dataset->course_count++] = course;
        (*all_count)++;
    }
    fclose(file);
    if (line_number == 0 || *all_count == 0) {
        set_error(error, error_size, "courses.csv has no courses");
        return -1;
    }
    return 0;
}

static int lookup_code(char all_codes[][16], int all_count, const char *code)
{
    int i;

    for (i = 0; i < all_count; i++) {
        if (strcmp(all_codes[i], code) == 0) {
            return i;
        }
    }
    return -1;
}

static int load_prerequisites(Dataset *dataset, const char *directory,
                              char all_codes[][16], int all_count, const int *kept_index,
                              char *error, size_t error_size)
{
    FILE *file = open_csv(directory, "prerequisites.csv", error, error_size);
    char line[LINE_SIZE];
    char *fields[MAX_FIELDS];
    int line_number = 0;
    int count;

    if (file == NULL) {
        return -1;
    }
    read_record(file, line, sizeof(line), fields, MAX_FIELDS, &line_number);
    while ((count = read_record(file, line, sizeof(line), fields, MAX_FIELDS, &line_number)) > 0) {
        int course;
        int prerequisite;

        if (count != 2) {
            set_error(error, error_size, "prerequisites.csv line %d: expected 2 fields", line_number);
            fclose(file);
            return -1;
        }
        trim(fields[0]);
        trim(fields[1]);
        course = lookup_code(all_codes, all_count, fields[0]);
        if (course < 0) {
            set_error(error, error_size, "unknown course code %s in prerequisites.csv line %d", fields[0], line_number);
            fclose(file);
            return -1;
        }
        prerequisite = lookup_code(all_codes, all_count, fields[1]);
        if (prerequisite < 0) {
            set_error(error, error_size, "unknown course code %s in prerequisites.csv line %d", fields[1], line_number);
            fclose(file);
            return -1;
        }
        if (kept_index[course] < 0 || kept_index[prerequisite] < 0) {
            continue;
        }
        if (dataset->prerequisite_count >= MAX_PREREQUISITES) {
            set_error(error, error_size, "prerequisites.csv: too many pairs");
            fclose(file);
            return -1;
        }
        dataset->prerequisites[dataset->prerequisite_count][0] = kept_index[course];
        dataset->prerequisites[dataset->prerequisite_count][1] = kept_index[prerequisite];
        dataset->prerequisite_count++;
    }
    fclose(file);
    return 0;
}

static int load_blocked(Dataset *dataset, const char *directory, const NamePool *pool,
                        char *error, size_t error_size)
{
    FILE *file = open_csv(directory, "blocked.csv", error, error_size);
    char line[LINE_SIZE];
    char *fields[MAX_FIELDS];
    int line_number = 0;
    int count;

    if (file == NULL) {
        return -1;
    }
    read_record(file, line, sizeof(line), fields, MAX_FIELDS, &line_number);
    while ((count = read_record(file, line, sizeof(line), fields, MAX_FIELDS, &line_number)) > 0) {
        int slot;
        int professor;

        if (count != 2) {
            set_error(error, error_size, "blocked.csv line %d: expected 2 fields", line_number);
            fclose(file);
            return -1;
        }
        trim(fields[0]);
        trim(fields[1]);
        slot = slot_parse(fields[1]);
        if (slot < 0) {
            set_error(error, error_size, "invalid slot %s in blocked.csv line %d", fields[1], line_number);
            fclose(file);
            return -1;
        }
        if (!pool_has(pool, fields[0])) {
            set_error(error, error_size, "unknown professor %s in blocked.csv line %d", fields[0], line_number);
            fclose(file);
            return -1;
        }
        professor = find_professor(dataset, fields[0]);
        if (professor >= 0) {
            dataset->blocked[professor][slot] = 1;
        }
    }
    fclose(file);
    return 0;
}

int dataset_load(Dataset *dataset, const char *directory, int max_period, char *error, size_t error_size)
{
    char all_codes[MAX_COURSES][16];
    int kept_index[MAX_COURSES];
    int all_count = 0;
    static NamePool pool;

    pool.count = 0;
    memset(dataset, 0, sizeof(*dataset));
    if (error != NULL && error_size > 0) {
        error[0] = '\0';
    }
    if (load_courses(dataset, directory, max_period, all_codes, &all_count, kept_index, &pool, error, error_size) != 0) {
        return -1;
    }
    if (load_prerequisites(dataset, directory, all_codes, all_count, kept_index, error, error_size) != 0) {
        return -1;
    }
    if (load_blocked(dataset, directory, &pool, error, error_size) != 0) {
        return -1;
    }
    return 0;
}
