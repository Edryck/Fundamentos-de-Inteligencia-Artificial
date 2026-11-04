#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "test_support.h"
#include "../src/report.h"

#define TEST_DIRECTORY "build/test_tmp"
#define MISSING_DIRECTORY "build/test_tmp/missing_results"

static Dataset dataset;
static BlockList list;
static Individual individual;
static RealSchedule real;
static char lines[40][256];

static int line_equals(FILE *file, const char *expected)
{
    char line[256];
    size_t length;

    if (fgets(line, sizeof(line), file) == NULL) {
        return 0;
    }
    length = strlen(line);
    if (length > 0 && line[length - 1] == '\n') {
        line[length - 1] = '\0';
    }
    return strcmp(line, expected) == 0;
}

static int read_all_lines(FILE *file)
{
    int count = 0;

    while (count < 40 && fgets(lines[count], sizeof(lines[count]), file) != NULL) {
        lines[count][strcspn(lines[count], "\n")] = '\0';
        count++;
    }
    return count;
}

static void test_summary(void)
{
    int penalties[4] = {0, 0, 2, 2};
    RunSummary summary;

    summary_compute(&summary, penalties, 4);
    CHECK(summary.runs == 4);
    CHECK(summary.best_penalty == 0);
    CHECK(summary.mean_penalty == 1.0);
    CHECK(summary.standard_deviation == 1.0);
    CHECK(summary.conflict_free_rate == 0.5);
}

static void test_history_file(void)
{
    GeneticResult result;
    FILE *file;

    result.generations = 3;
    result.best_penalty_history = malloc(sizeof(int) * 3);
    if (result.best_penalty_history == NULL) {
        CHECK(0);
        return;
    }
    result.best_penalty_history[0] = 5;
    result.best_penalty_history[1] = 3;
    result.best_penalty_history[2] = 0;
    CHECK(report_write_history(TEST_DIRECTORY "/history.csv", &result) == 0);
    free(result.best_penalty_history);

    file = fopen(TEST_DIRECTORY "/history.csv", "r");
    CHECK(file != NULL);
    if (file != NULL) {
        CHECK(line_equals(file, "generation,best_penalty"));
        CHECK(line_equals(file, "1,5"));
        CHECK(line_equals(file, "2,3"));
        CHECK(line_equals(file, "3,0"));
        CHECK(fgetc(file) == EOF);
        fclose(file);
    }
}

static void test_summary_file(void)
{
    RunSummary summary;
    FILE *file;

    summary.runs = 4;
    summary.best_penalty = 0;
    summary.mean_penalty = 1.0;
    summary.standard_deviation = 1.0;
    summary.conflict_free_rate = 0.5;
    CHECK(report_write_summary(TEST_DIRECTORY "/summary.csv", &summary) == 0);

    file = fopen(TEST_DIRECTORY "/summary.csv", "r");
    CHECK(file != NULL);
    if (file != NULL) {
        CHECK(line_equals(file, "runs,best_penalty,mean_penalty,standard_deviation,conflict_free_rate"));
        CHECK(line_equals(file, "4,0,1.0000,1.0000,0.5000"));
        CHECK(fgetc(file) == EOF);
        fclose(file);
    }
}

static void test_missing_results_directory(void)
{
    RunSummary summary;
    struct stat info;

    summary.runs = 1;
    summary.best_penalty = 0;
    summary.mean_penalty = 0.0;
    summary.standard_deviation = 0.0;
    summary.conflict_free_rate = 1.0;

    (void)rmdir(MISSING_DIRECTORY);
    CHECK(report_ensure_directory(MISSING_DIRECTORY) == 0);
    CHECK(stat(MISSING_DIRECTORY, &info) == 0 && S_ISDIR(info.st_mode));
    CHECK(report_write_summary(MISSING_DIRECTORY "/summary.csv", &summary) == 0);
    (void)remove(MISSING_DIRECTORY "/summary.csv");
    (void)rmdir(MISSING_DIRECTORY);
}

static void test_schedule_print(void)
{
    FILE *file;
    int count;

    memset(&dataset, 0, sizeof(dataset));
    memset(&list, 0, sizeof(list));
    memset(&individual, 0, sizeof(individual));
    dataset.course_count = 1;
    strcpy(dataset.courses[0].code, "AAA1");
    dataset.courses[0].period = 1;
    list.block_count = 1;
    list.blocks[0].course = 0;
    list.blocks[0].length = 2;
    list.blocks[0].professor = 0;
    list.blocks[0].saturday = 0;
    individual.genes[0] = 0;

    file = fopen(TEST_DIRECTORY "/schedule.txt", "w");
    CHECK(file != NULL);
    if (file == NULL) {
        return;
    }
    report_print_schedule(file, &individual, &list, &dataset, 1);
    fclose(file);

    file = fopen(TEST_DIRECTORY "/schedule.txt", "r");
    CHECK(file != NULL);
    if (file == NULL) {
        return;
    }
    count = read_all_lines(file);
    fclose(file);

    CHECK(count >= 5);
    if (count >= 5) {
        CHECK(strncmp(lines[0], "Periodo 1", 9) == 0);
        CHECK(strncmp(lines[2], "M1", 2) == 0);
        CHECK(strncmp(lines[2] + 9, "AAA1", 4) == 0);
        CHECK(strncmp(lines[3], "M2", 2) == 0);
        CHECK(strncmp(lines[3] + 9, "AAA1", 4) == 0);
        CHECK(strncmp(lines[4], "M3", 2) == 0);
        CHECK(strstr(lines[4], "AAA1") == NULL);
    }
    (void)remove(TEST_DIRECTORY "/schedule.txt");
}

static char *read_whole_file(const char *path)
{
    FILE *file;
    char *text;
    long size;

    file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    text = malloc((size_t)size + 1);
    if (text == NULL) {
        fclose(file);
        return NULL;
    }
    text[fread(text, 1, (size_t)size, file)] = '\0';
    fclose(file);
    return text;
}

static int count_occurrences(const char *text, const char *needle)
{
    int count = 0;
    const char *cursor = text;

    while ((cursor = strstr(cursor, needle)) != NULL) {
        count++;
        cursor += strlen(needle);
    }
    return count;
}

static void prepare_html_fixture(void)
{
    memset(&dataset, 0, sizeof(dataset));
    memset(&list, 0, sizeof(list));
    memset(&individual, 0, sizeof(individual));
    dataset.course_count = 2;
    strcpy(dataset.courses[0].code, "AAA1");
    dataset.courses[0].period = 1;
    strcpy(dataset.courses[1].code, "B&B<2>");
    dataset.courses[1].period = 2;
    dataset.professor_count = 2;
    strcpy(dataset.professors[0], "ZELDA");
    strcpy(dataset.professors[1], "X&Y");
    list.block_count = 2;
    list.blocks[0].course = 0;
    list.blocks[0].length = 2;
    list.blocks[0].professor = 0;
    list.blocks[1].course = 1;
    list.blocks[1].length = 1;
    list.blocks[1].professor = 1;
    individual.genes[0] = 0;
    individual.genes[1] = 12;
    individual.penalty = 7;
    memset(&real, 0, sizeof(real));
    real.lesson_count = 4;
    strcpy(real.lessons[0].code, "AAA1");
    real.lessons[0].period = 1;
    strcpy(real.lessons[0].professor, "ZELDA");
    real.lessons[0].slot = 0;
    strcpy(real.lessons[0].room, "E09");
    strcpy(real.lessons[1].code, "OPX1");
    real.lessons[1].period = 1;
    strcpy(real.lessons[1].professor, "LINK");
    real.lessons[1].slot = 0;
    strcpy(real.lessons[1].room, "REMOTA");
    strcpy(real.lessons[2].code, "B&B<2>");
    real.lessons[2].period = 2;
    strcpy(real.lessons[2].professor, "X&Y");
    real.lessons[2].slot = 66 - 1;
    strcpy(real.lessons[2].room, "L<5>");
    strcpy(real.lessons[3].code, "AAA1");
    real.lessons[3].period = 3;
    strcpy(real.lessons[3].professor, "ZELDA");
    real.lessons[3].slot = 6;
    strcpy(real.lessons[3].room, "L5");
    real.name_count = 3;
    strcpy(real.name_codes[0], "AAA1");
    strcpy(real.names[0], "Algoritmos Alpha");
    strcpy(real.name_codes[1], "OPX1");
    strcpy(real.names[1], "Optativa Xis");
    strcpy(real.name_codes[2], "B&B<2>");
    strcpy(real.names[2], "Nome <b> & Co");
}

static void test_html_pages(void)
{
    char *text;

    prepare_html_fixture();
    CHECK(report_write_html(TEST_DIRECTORY "/schedule.html", &individual, &list, &dataset, 8, &real, 99) == 0);
    text = read_whole_file(TEST_DIRECTORY "/schedule.html");
    CHECK(text != NULL);
    if (text != NULL) {
        CHECK(count_occurrences(text, "<section") == 8);
        CHECK(strstr(text, "lang=\"pt-BR\"") != NULL);
        CHECK(strstr(text, "07:30") != NULL);
        CHECK(strstr(text, "17:30") != NULL);
        CHECK(strstr(text, "http://") == NULL);
        CHECK(strstr(text, "https://") == NULL);
        free(text);
    }
}

static void test_html_warning(void)
{
    char *text;

    prepare_html_fixture();
    CHECK(report_write_html(TEST_DIRECTORY "/schedule.html", &individual, &list, &dataset, 8, &real, 99) == 0);
    text = read_whole_file(TEST_DIRECTORY "/schedule.html");
    CHECK(text != NULL);
    if (text != NULL) {
        CHECK(strstr(text, "simula") != NULL);
        CHECK(strstr(text, "2026/2") != NULL);
        CHECK(strstr(text, "Grade simulada a partir dos professores de 2026/2. N\xc3\xa3o \xc3\xa9 a grade oficial.") != NULL);
        CHECK(strstr(text, "Per\xc3\xadodo 1 de 8") != NULL);
        free(text);
    }
}

static void test_html_tables(void)
{
    char *text;

    prepare_html_fixture();
    CHECK(report_write_html(TEST_DIRECTORY "/schedule.html", &individual, &list, &dataset, 8, &real, 99) == 0);
    text = read_whole_file(TEST_DIRECTORY "/schedule.html");
    CHECK(text != NULL);
    if (text != NULL) {
        CHECK(count_occurrences(text, "<table>") == 16);
        CHECK(count_occurrences(text, "Grade real 2026/2") == 8 + 1);
        CHECK(count_occurrences(text, "Grade gerada pelo AG") == 8 + 1);
        CHECK(count_occurrences(text, "Seed 99 \xc2\xb7 Penalidade 7") == 8);
        CHECK(count_occurrences(text, "Intervalo Manh\xc3\xa3 10:00") == 16);
        CHECK(count_occurrences(text, "Intervalo Tarde 15:30") == 16);
        CHECK(count_occurrences(text, "<th>Segunda</th>") == 16);
        CHECK(count_occurrences(text, "<th>S\xc3\xa1" "bado</th>") == 16);
        CHECK(count_occurrences(text, "M1 <span") == 16);
        CHECK(count_occurrences(text, "T6 <span") == 16);
        CHECK(strstr(text, "viewport") != NULL);
        CHECK(strstr(text, "@media (prefers-color-scheme:dark)") != NULL);
        CHECK(strstr(text, "ArrowRight") != NULL);
        CHECK(strstr(text, "<!--") == NULL);
        free(text);
    }
}

static void test_html_real_content(void)
{
    char *text;

    prepare_html_fixture();
    CHECK(report_write_html(TEST_DIRECTORY "/schedule.html", &individual, &list, &dataset, 8, &real, 99) == 0);
    text = read_whole_file(TEST_DIRECTORY "/schedule.html");
    CHECK(text != NULL);
    if (text != NULL) {
        CHECK(strstr(text, "<span class=\"name\">Algoritmos Alpha</span><span class=\"code\">AAA1</span>"
                           "<span class=\"prof\">ZELDA</span><span class=\"room\">E09</span>") != NULL);
        CHECK(strstr(text, "<span class=\"name\">Optativa Xis</span>") != NULL);
        CHECK(strstr(text, "<span class=\"room\">REMOTA</span>") != NULL);
        CHECK(strstr(text, "E09</span></div><div class=\"lesson\"><span class=\"name\">Optativa Xis") != NULL);
        CHECK(strstr(text, "Nome &lt;b&gt; &amp; Co") != NULL);
        CHECK(strstr(text, "L&lt;5&gt;") != NULL);
        CHECK(strstr(text, "Nome <b>") == NULL);
        CHECK(count_occurrences(text, "<span class=\"room\">") == 4);
        CHECK(strstr(text, "O aviso vale para a tabela Grade gerada pelo AG") != NULL);
        free(text);
    }
}

static void test_html_course(void)
{
    char *text;

    prepare_html_fixture();
    CHECK(report_write_html(TEST_DIRECTORY "/schedule.html", &individual, &list, &dataset, 8, &real, 99) == 0);
    text = read_whole_file(TEST_DIRECTORY "/schedule.html");
    CHECK(text != NULL);
    if (text != NULL) {
        CHECK(strstr(text, "AAA1") != NULL);
        CHECK(strstr(text, "ZELDA") != NULL);
        CHECK(strstr(text, "B&amp;B&lt;2&gt;") != NULL);
        CHECK(strstr(text, "X&amp;Y") != NULL);
        CHECK(strstr(text, "B&B<2>") == NULL);
        free(text);
    }
    (void)remove(TEST_DIRECTORY "/schedule.html");
}

void test_report_run(void)
{
    CHECK(report_ensure_directory("build") == 0);
    CHECK(report_ensure_directory(TEST_DIRECTORY) == 0);
    test_summary();
    test_history_file();
    test_summary_file();
    test_missing_results_directory();
    test_schedule_print();
    test_html_pages();
    test_html_warning();
    test_html_tables();
    test_html_real_content();
    test_html_course();
    (void)remove(TEST_DIRECTORY "/history.csv");
    (void)remove(TEST_DIRECTORY "/summary.csv");
}
