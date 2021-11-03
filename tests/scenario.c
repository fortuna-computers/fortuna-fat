#include "scenario.h"

#include <string.h>

#include "image.h"

void (*scenario_list[MAX_SCENARIOS])() = {
        scenario_raw_sectors,
        scenario_fat16,
        scenario_fat32,
        scenario_fat32_align512,
        scenario_fat32_spc128,
        scenario_fat32_2_partitions,
        NULL
};

void scenario_raw_sectors()
{
    scenario_name = "Image with raw sectors";
    img_sz = 256 * SECTOR_SZ;
    for (size_t i = 0; i < 256; ++i)
        memset(&img_data[i * SECTOR_SZ], (uint8_t) i, SECTOR_SZ);
}

void scenario_fat16()
{
    scenario_name = "FAT16 partition";
}

void scenario_fat32()
{
    scenario_name = "FAT32 partition";
}

void scenario_fat32_align512()
{
    scenario_name = "FAT32 partition (512-byte alignment)";
}

void scenario_fat32_spc128()
{
    scenario_name = "FAT32 partition (128 sec. per cluster)";
}

void scenario_fat32_2_partitions()
{
    scenario_name = "FAT32 (2 partitions)";
}