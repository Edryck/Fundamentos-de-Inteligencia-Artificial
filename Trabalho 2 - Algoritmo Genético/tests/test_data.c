#include <string.h>
#include "test_support.h"
#include "../src/data.h"

static Dataset dataset;
static char error[256];

static void test_load_valid(void)
{
    int result = dataset_load(&dataset, "tests/fixtures/valid", 10, error, sizeof(error));
    CHECK(result == 0);
    CHECK(dataset.course_count == 3);
    CHECK(strcmp(dataset.courses[0].code, "AAA1") == 0);
    CHECK(dataset.courses[0].professor_count == 1);
    CHECK(dataset.courses[2].professor_count == 2);
    CHECK(dataset.courses[2].saturday == 1);
    CHECK(dataset.courses[2].weekly_lessons == 6);
    CHECK(dataset.professor_count == 2);
    CHECK(dataset.prerequisite_count == 1);
    CHECK(dataset.prerequisites[0][0] == 1);
    CHECK(dataset.prerequisites[0][1] == 0);
    CHECK(dataset.blocked[dataset.courses[0].professors[0]][20] == 1);
    CHECK(dataset.blocked[dataset.courses[0].professors[0]][21] == 0);
}

static void test_load_crlf(void)
{
    int result = dataset_load(&dataset, "tests/fixtures/crlf", 10, error, sizeof(error));
    CHECK(result == 0);
    CHECK(dataset.course_count == 3);
    CHECK(dataset.courses[2].professor_count == 2);
    CHECK(strcmp(dataset.courses[2].code, "CCC3") == 0);
    CHECK(dataset.prerequisite_count == 1);
    CHECK(dataset.blocked[dataset.courses[0].professors[0]][20] == 1);
}

static void test_period_filter(void)
{
    int result = dataset_load(&dataset, "tests/fixtures/period", 4, error, sizeof(error));
    CHECK(result == 0);
    CHECK(dataset.course_count == 2);
    CHECK(dataset.prerequisite_count == 1);
    CHECK(dataset.blocked[dataset.courses[0].professors[0]][0] == 1);
}

static void test_unknown_prerequisite(void)
{
    int result = dataset_load(&dataset, "tests/fixtures/unknown_prereq", 10, error, sizeof(error));
    CHECK(result == -1);
    CHECK(strstr(error, "ZZZ9") != NULL);
}

static void test_missing_file(void)
{
    int result = dataset_load(&dataset, "tests/fixtures/missing", 10, error, sizeof(error));
    CHECK(result == -1);
    CHECK(error[0] != '\0');
}

static void test_invalid_lessons(void)
{
    int result = dataset_load(&dataset, "tests/fixtures/bad_lessons", 10, error, sizeof(error));
    CHECK(result == -1);
}

static void test_unknown_blocked_professor(void)
{
    int result = dataset_load(&dataset, "tests/fixtures/unknown_blocked", 10, error, sizeof(error));
    CHECK(result == -1);
    CHECK(strstr(error, "GHOST") != NULL);
}

static void test_blocked_professor_filtered_out(void)
{
    int result = dataset_load(&dataset, "tests/fixtures/filtered_blocked", 1, error, sizeof(error));
    CHECK(result == 0);
    CHECK(dataset.course_count == 1);
    CHECK(dataset.professor_count == 1);
}

static void test_empty_courses(void)
{
    int result = dataset_load(&dataset, "tests/fixtures/empty_courses", 10, error, sizeof(error));
    CHECK(result == -1);
    CHECK(error[0] != '\0');
}

static void test_real_data_periods(void)
{
    CHECK(dataset_load(&dataset, "data", 4, error, sizeof(error)) == 0);
    CHECK(dataset_load(&dataset, "data", 8, error, sizeof(error)) == 0);
}

static void test_slot_roundtrip(void)
{
    char text[16];
    CHECK(slot_parse("TUE-T3") == 20);
    CHECK(slot_parse("SAT-M1") == 60);
    CHECK(slot_parse("SAT-T1") == -1);
    CHECK(slot_parse("MON-M1") == 0);
    CHECK(slot_parse("FRI-T6") == 59);
    CHECK(slot_parse("SAT-M6") == 65);
    CHECK(slot_parse("XXX-M1") == -1);
    CHECK(slot_parse("MON-M7") == -1);
    CHECK(slot_parse("MON") == -1);
    CHECK(slot_parse("") == -1);
    slot_format(20, text, sizeof(text));
    CHECK(strcmp(text, "TUE-T3") == 0);
    slot_format(60, text, sizeof(text));
    CHECK(strcmp(text, "SAT-M1") == 0);
    slot_format(65, text, sizeof(text));
    CHECK(strcmp(text, "SAT-M6") == 0);
}

void test_data_run(void)
{
    test_load_valid();
    test_load_crlf();
    test_period_filter();
    test_unknown_prerequisite();
    test_missing_file();
    test_invalid_lessons();
    test_unknown_blocked_professor();
    test_blocked_professor_filtered_out();
    test_empty_courses();
    test_real_data_periods();
    test_slot_roundtrip();
}
