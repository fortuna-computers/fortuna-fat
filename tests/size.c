#include "../src/ffat.h"

static uint8_t buffer[512] = {0};

bool raw_write(uint64_t sector, uint8_t const* buffer)
{
    return true;
}

bool raw_read(uint64_t sector, uint8_t* buffer)
{
    return true;
}

uint64_t total_sectors()
{
    return 0;
}

int main()
{
    FDateTime date_time = {};
    
    FFat f;
    f.buffer = buffer;
    
    ffat_op(&f, F_READ_RAW, date_time);
    ffat_op(&f, F_WRITE_RAW, date_time);
    ffat_op(&f, F_INIT, date_time);
    ffat_op(&f, F_BOOT, date_time);
    ffat_op(&f, F_FREE, date_time);
    ffat_op(&f, F_FSI_CALC, date_time);
    ffat_op(&f, F_CREATE, date_time);
    ffat_op(&f, F_SEEK_FW, date_time);
    ffat_op(&f, F_SEEK_EOF, date_time);
    ffat_op(&f, F_APPEND, date_time);
    ffat_op(&f, F_TRUNCATE, date_time);
    ffat_op(&f, F_READ, date_time);
    ffat_op(&f, F_WRITE, date_time);
}

