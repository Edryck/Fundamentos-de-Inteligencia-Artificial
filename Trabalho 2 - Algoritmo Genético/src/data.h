#ifndef DATA_H
#define DATA_H

#include <stddef.h>

#define MAX_COURSES 128
#define MAX_PROFESSORS 64
#define MAX_COURSE_PROFESSORS 2
#define MAX_PREREQUISITES 128
#define SLOT_COUNT 66

typedef struct {
    char code[16];
    int period;
    int weekly_lessons;
    int professors[MAX_COURSE_PROFESSORS];
    int professor_count;
    int saturday;
} Course;

typedef struct {
    Course courses[MAX_COURSES];
    int course_count;
    char professors[MAX_PROFESSORS][32];
    int professor_count;
    int prerequisites[MAX_PREREQUISITES][2];
    int prerequisite_count;
    unsigned char blocked[MAX_PROFESSORS][SLOT_COUNT];
} Dataset;

int dataset_load(Dataset *dataset, const char *directory, int max_period, char *error, size_t error_size);
int slot_parse(const char *text);
void slot_format(int slot, char *text, size_t text_size);

#endif
