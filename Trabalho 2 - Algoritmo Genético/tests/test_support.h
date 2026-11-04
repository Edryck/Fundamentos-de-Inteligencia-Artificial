#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include <stdio.h>

extern int test_checks_run;
extern int test_checks_failed;

#define CHECK(condition) \
    do { \
        test_checks_run++; \
        if (!(condition)) { \
            test_checks_failed++; \
            printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #condition); \
        } \
    } while (0)

void test_data_run(void);
void test_chromosome_run(void);
void test_fitness_run(void);
void test_genetic_run(void);
void test_cli_run(void);
void test_report_run(void);

#include "../src/data.h"
#include "../src/chromosome.h"
int verify_penalty(const Individual *individual, const BlockList *list, const Dataset *dataset);

#endif
