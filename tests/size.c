#include "../src/ffat.h"

static uint8_t buffer[512] = {0};
static FFat f = {};

bool raw_write(uint64_t sector, uint8_t const* buffer)
{
    (void) sector;
    (void) buffer;
    return true;
}

bool raw_read(uint64_t sector, uint8_t* buffer)
{
    (void) sector;
    (void) buffer;
    return true;
}

uint64_t total_sectors()
{
    return 0;
}

int main()
{
    f.buffer = buffer;
    
    for (uint16_t i = 0; i < 256; ++i)
        ffat_op(&f, i);
}

