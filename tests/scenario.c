#include "scenario.h"

#include <string.h>

#include "image.h"

void (*scenario_list[MAX_SCENARIOS])() = {
        scenario_raw_sectors,
        NULL
};

void scenario_raw_sectors()
{
    scenario_name = "Image with raw sectors";
    img_sz = 256 * SECTOR_SZ;
    for (size_t i = 0; i < 256; ++i)
        memset(&img_data[i * SECTOR_SZ], (uint8_t) i, SECTOR_SZ);
}