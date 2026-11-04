#include <string.h>
#include "test_support.h"
#include "../src/real_schedule.h"

static RealSchedule real;
static char error[256];

static void test_real_valid(void)
{
    error[0] = '\0';
    CHECK(real_schedule_load(&real, "tests/fixtures/real_valid", error, sizeof(error)) == 0);
    CHECK(real.lesson_count == 4);
    CHECK(strcmp(real.lessons[0].code, "AAA1") == 0);
    CHECK(real.lessons[0].period == 1);
    CHECK(real.lessons[0].slot == 0);
    CHECK(strcmp(real.lessons[0].professor, "ZELDA") == 0);
    CHECK(strcmp(real.lessons[0].room, "E09") == 0);
    CHECK(real.lessons[2].period == 2);
    CHECK(real.lessons[2].slot == 62);
    CHECK(strcmp(real.lessons[2].room, "REMOTA") == 0);
    CHECK(real.lessons[3].slot == 0);
    CHECK(real.name_count == 3);
    CHECK(strcmp(real_schedule_name(&real, "AAA1"), "Algoritmos") == 0);
    CHECK(strcmp(real_schedule_name(&real, "BBB2"), "Modelagem, Parte 2") == 0);
    CHECK(real_schedule_name(&real, "ZZZ9") == NULL);
}

static void test_real_crlf(void)
{
    error[0] = '\0';
    CHECK(real_schedule_load(&real, "tests/fixtures/real_crlf", error, sizeof(error)) == 0);
    CHECK(real.lesson_count == 4);
    CHECK(strcmp(real.lessons[1].room, "E09") == 0);
    CHECK(strcmp(real.lessons[2].room, "REMOTA") == 0);
    CHECK(strcmp(real_schedule_name(&real, "CCC3"), "Redes") == 0);
}

static void test_real_invalid_slot(void)
{
    error[0] = '\0';
    CHECK(real_schedule_load(&real, "tests/fixtures/real_bad_slot", error, sizeof(error)) == -1);
    CHECK(error[0] != '\0');
    CHECK(strstr(error, "SAT-T1") != NULL);
}

static void test_real_missing(void)
{
    error[0] = '\0';
    CHECK(real_schedule_load(&real, "tests/fixtures/real_absent", error, sizeof(error)) == -1);
    CHECK(error[0] != '\0');
}

static void test_real_project_data(void)
{
    error[0] = '\0';
    CHECK(real_schedule_load(&real, "data", error, sizeof(error)) == 0);
    CHECK(real.lesson_count == 219);
    CHECK(real_schedule_name(&real, "CC1AED1") != NULL);
}

void test_real_schedule_run(void)
{
    test_real_valid();
    test_real_crlf();
    test_real_invalid_slot();
    test_real_missing();
    test_real_project_data();
}
