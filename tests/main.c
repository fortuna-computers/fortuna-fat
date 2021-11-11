#include "scenario.h"
#include "test.h"

#include <stdio.h>

#define GRN "\e[0;32m"
#define RED "\e[0;31m"
#define RST "\e[0m"

static bool has_scenario(Test const* test, void (* scenario)())
{
    size_t i = 0;
    do {
        if (scenario == test->scenarios[i])
            return true;
        ++i;
    } while (test->scenarios[i]);
    return false;
}

int main()
{
    uint8_t buffer[512];
    FFat f = {0};
    f.buffer = buffer;
    
    size_t i = 0, j = 0;
    do {
        printf("[%c] %s\n", (char) i + 'A', test_list[i].name);
        ++i;
    } while (test_list[i].name);
    
    printf("%40c", ' ');
    i = 0; do { printf("%c", (char) i + 'A'); ++i; } while (test_list[i].name);
    printf("\n");
    for (size_t k = 0; k < 40 + i; ++k) putchar('=');
    putchar('\n');
    
    i = 0;
    do {
        j = 0;
        do {
            scenario_list[i]();  // prepare scenario
            if (j == 0)
                printf("%-40s", scenario_name);
            if (has_scenario(&test_list[j], scenario_list[i])) {
                if (test_list[j].run_test(&f, scenario_list[i]))
                    printf(GRN "\u2713" RST);
                else
                    printf(RED "X" RST);
            } else {
                putchar(' ');
            }
            ++j;
        } while (test_list[j].name);
        ++i;
        putchar('\n');
    } while (scenario_list[i]);
    
    return 0;
}