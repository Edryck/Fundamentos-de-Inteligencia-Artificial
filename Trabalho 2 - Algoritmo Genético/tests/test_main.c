#include <stdio.h>
#include "test_support.h"

int test_checks_run = 0;
int test_checks_failed = 0;

int main(void)
{
    test_data_run();
    test_chromosome_run();
    test_fitness_run();
    test_genetic_run();
    test_cli_run();
    test_real_schedule_run();
    test_report_run();
    test_live_run();

    printf("%d checks, %d failed\n", test_checks_run, test_checks_failed);
    if (test_checks_failed == 0) {
        printf("PASS\n");
        return 0;
    }
    printf("FAIL\n");
    return 1;
}
