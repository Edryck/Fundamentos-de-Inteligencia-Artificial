#ifndef REAL_SCHEDULE_H
#define REAL_SCHEDULE_H

#include <stddef.h>

#define REAL_MAX_LESSONS 512
#define REAL_MAX_NAMES 128

typedef struct {
    char code[16];
    int period;
    char professor[32];
    int slot;
    char room[16];
} RealLesson;

typedef struct {
    RealLesson lessons[REAL_MAX_LESSONS];
    int lesson_count;
    char name_codes[REAL_MAX_NAMES][16];
    char names[REAL_MAX_NAMES][96];
    int name_count;
} RealSchedule;

int real_schedule_load(RealSchedule *real, const char *directory, char *error, size_t error_size);
const char *real_schedule_name(const RealSchedule *real, const char *code);

#endif
