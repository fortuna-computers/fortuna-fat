#include "test.h"

#include "scenario.h"

bool test_raw_sector(FFat* f)
{
    return true;
}

const Test _test_list[] = {
        { "Raw sector access", { scenario_raw_sectors, NULL }, test_raw_sector },
        { NULL, {}, NULL },
};

const Test* test_list = _test_list;