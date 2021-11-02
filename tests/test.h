#ifndef FORTUNA_FAT_TEST_H
#define FORTUNA_FAT_TEST_H

#include <stddef.h>
#include "../src/ffat.h"

#define MAX_SCENARIOS 32

typedef struct Test {
    const char* name;
    void (*scenarios[MAX_SCENARIOS])(void);
    bool (*run_test)(FFat*);
} Test;

const Test test_list[];

#endif //FORTUNA_FAT_TEST_H
