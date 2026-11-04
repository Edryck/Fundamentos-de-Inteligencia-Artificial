#include "real_schedule.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"

#define LINE_SIZE 512

static void set_error(char *error, size_t error_size, const char *format, ...)
{
    va_list args;

    if (error != NULL && error_size > 0) {
        va_start(args, format);
        vsnprintf(error, error_size, format, args);
        va_end(args);
    }
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

static int split(char *line, char **fields, int max_fields)
{
    int count = 0;
    char *cursor = line;

    fields[count++] = cursor;
    while (*cursor != '\0') {
        if (*cursor == ',' && count < max_fields) {
            *cursor = '\0';
            fields[count++] = cursor + 1;
        }
        cursor++;
    }
    for (int index = 0; index < count; index++) {
        trim(fields[index]);
    }
    return count;
}

static FILE *open_file(const char *directory, const char *name, char *error, size_t error_size)
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

static int copy_field(char *target, size_t size, const char *source)
{
    if (strlen(source) >= size) {
        return -1;
    }
    strcpy(target, source);
    return 0;
}

static int load_lessons(RealSchedule *real, const char *directory, char *error, size_t error_size)
{
    char line[LINE_SIZE];
    char *fields[8];
    FILE *file;
    int line_number = 0;
    int count;
    RealLesson *lesson;

    file = open_file(directory, "real_schedule.csv", error, error_size);
    if (file == NULL) {
        return -1;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        line_number++;
        trim(line);
        if (line[0] == '\0') {
            continue;
        }
        count = split(line, fields, 8);
        if (line_number == 1) {
            if (count != 5 || strcmp(fields[0], "code") != 0) {
                set_error(error, error_size, "real_schedule.csv line %d: invalid header", line_number);
                fclose(file);
                return -1;
            }
            continue;
        }
        if (count != 5 || fields[0][0] == '\0' || fields[2][0] == '\0') {
            set_error(error, error_size, "real_schedule.csv line %d: invalid record", line_number);
            fclose(file);
            return -1;
        }
        if (real->lesson_count >= REAL_MAX_LESSONS) {
            set_error(error, error_size, "real_schedule.csv line %d: too many lessons", line_number);
            fclose(file);
            return -1;
        }
        lesson = &real->lessons[real->lesson_count];
        if (strlen(fields[1]) != 3 || fields[1][0] != 'C' || fields[1][1] != 'C'
            || fields[1][2] < '1' || fields[1][2] > '8') {
            set_error(error, error_size, "real_schedule.csv line %d: invalid class %s", line_number, fields[1]);
            fclose(file);
            return -1;
        }
        lesson->period = fields[1][2] - '0';
        lesson->slot = slot_parse(fields[3]);
        if (lesson->slot < 0) {
            set_error(error, error_size, "real_schedule.csv line %d: invalid slot %s", line_number, fields[3]);
            fclose(file);
            return -1;
        }
        if (copy_field(lesson->code, sizeof(lesson->code), fields[0]) != 0
            || copy_field(lesson->professor, sizeof(lesson->professor), fields[2]) != 0
            || copy_field(lesson->room, sizeof(lesson->room), fields[4]) != 0) {
            set_error(error, error_size, "real_schedule.csv line %d: field too long", line_number);
            fclose(file);
            return -1;
        }
        real->lesson_count++;
    }
    fclose(file);
    return 0;
}

static int load_names(RealSchedule *real, const char *directory, char *error, size_t error_size)
{
    char line[LINE_SIZE];
    char *fields[4];
    FILE *file;
    int line_number = 0;
    int count;

    file = open_file(directory, "course_names.csv", error, error_size);
    if (file == NULL) {
        return -1;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        line_number++;
        trim(line);
        if (line[0] == '\0') {
            continue;
        }
        count = split(line, fields, 2);
        if (line_number == 1) {
            if (count != 2 || strcmp(fields[0], "code") != 0) {
                set_error(error, error_size, "course_names.csv line %d: invalid header", line_number);
                fclose(file);
                return -1;
            }
            continue;
        }
        if (count != 2 || fields[0][0] == '\0' || fields[1][0] == '\0'
            || real->name_count >= REAL_MAX_NAMES
            || copy_field(real->name_codes[real->name_count], sizeof(real->name_codes[0]), fields[0]) != 0
            || copy_field(real->names[real->name_count], sizeof(real->names[0]), fields[1]) != 0) {
            set_error(error, error_size, "course_names.csv line %d: invalid record", line_number);
            fclose(file);
            return -1;
        }
        real->name_count++;
    }
    fclose(file);
    return 0;
}

int real_schedule_load(RealSchedule *real, const char *directory, char *error, size_t error_size)
{
    real->lesson_count = 0;
    real->name_count = 0;
    if (load_lessons(real, directory, error, error_size) != 0) {
        return -1;
    }
    return load_names(real, directory, error, error_size);
}

const char *real_schedule_name(const RealSchedule *real, const char *code)
{
    int index;

    for (index = 0; index < real->name_count; index++) {
        if (strcmp(real->name_codes[index], code) == 0) {
            return real->names[index];
        }
    }
    return NULL;
}
