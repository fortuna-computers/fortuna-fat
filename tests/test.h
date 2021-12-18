#ifndef FORTUNA_FAT_TEST_H
#define FORTUNA_FAT_TEST_H

#include <stddef.h>
#include "../src/ffat.h"
#include "scenario.h"

#define EXPECTED_DATE 0x02e9  // 9th Jul 1981
#define EXPECTED_TIME 0xabc0  // 9:30 pm

typedef struct Test {
    const char*     name;
    Scenario const* scenarios;
    bool            (*run_test)(FFat*, Scenario scenario);
} Test;

extern const Test* test_list;

#endif //FORTUNA_FAT_TEST_H
