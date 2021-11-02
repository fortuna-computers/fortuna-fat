#include "test.h"

#include <string.h>

#include "image.h"
#include "scenario.h"

FDateTime date_time = {};
FFatResult last_result;

#define X_OK(expr) { last_result = (expr); if (last_result != F_OK) return false; }
#define ASSERT(expr) { if (!(expr)) return false; }

bool test_raw_sector(FFat* f)
{
    f->F_RAWSEC = 0xaf;
    
    X_OK(ffat_op(f, F_READ_RAW, date_time))
    ASSERT(f->buffer[0] == 0xaf)
    ASSERT(f->buffer[511] == 0xaf)
    
    memset(f->buffer, 0x33, 512);
    X_OK(ffat_op(f, F_WRITE_RAW, date_time))
    memset(f->buffer, 0, 512);
    
    X_OK(ffat_op(f, F_READ_RAW, date_time))
    ASSERT(f->buffer[0] == 0x33)
    ASSERT(f->buffer[511] == 0x33)
    ASSERT(img_data[0xaf * SECTOR_SZ] == 0x33);
    
    return true;
}

bool test_raw_sector_past_end_of_image(FFat* f)
{
    f->F_RAWSEC = 0x100000;
    FFatResult r = ffat_op(f, F_READ_RAW, date_time);
    return r == F_IO_ERROR;
}

bool test_raw_sector_io_error(FFat* f)
{
    f->F_RAWSEC = 0;
    emulate_io_error = true;
    FFatResult r = ffat_op(f, F_READ_RAW, date_time);
    bool ok = (r == F_IO_ERROR);
    emulate_io_error = false;
    return ok;
}

static const Test test_list_[] = {
        { "Layer 0 sector access", { scenario_raw_sectors, NULL }, test_raw_sector },
        { "Layer 0 sector past end of image", { scenario_raw_sectors, NULL }, test_raw_sector_past_end_of_image },
        { "Layer 0 I/O error", { scenario_raw_sectors, NULL }, test_raw_sector_io_error },
        { NULL, {}, NULL },
};

const Test* test_list = test_list_;