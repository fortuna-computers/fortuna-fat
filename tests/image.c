#include "image.h"

#include <string.h>

uint64_t img_sz = 0;
uint8_t  img_data[256 * 1024 * 1024];
bool     emulate_io_error = false;

bool raw_write(uint64_t sector, uint8_t const* buffer)
{
    if (emulate_io_error)
        return false;
    memcpy(&img_data[sector * SECTOR_SZ], buffer, SECTOR_SZ);
    return true;
}

bool raw_read(uint64_t sector, uint8_t* buffer)
{
    if (emulate_io_error)
        return false;
    memcpy(buffer, &img_data[sector * SECTOR_SZ], SECTOR_SZ);
    return true;
}

uint64_t total_sectors()
{
    return img_sz;
}