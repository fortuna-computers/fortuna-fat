#include "test.h"

#include "scenario.h"

bool test_raw_sector(FFat* f)
{
    return true;
}

const Test test_list[] = {
        { "Raw sector access", { scenario_raw_sectors, NULL }, test_raw_sector },
        { NULL, {}, NULL },
};