#include "test.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "image.h"
#include "scenario.h"
#include "ff/ff.h"

FDateTime date_time = {};
FFatResult last_result;

#define R(expr) { FRESULT r = (expr); if (r != FR_OK) { fprintf(stderr, "FSFAT error: %d\n", r); exit(EXIT_FAILURE); } }
#define X_OK(expr) { last_result = (expr); if (last_result != F_OK) return false; }
#define ASSERT(expr) { if (!(expr)) return false; }

static bool test_raw_sector(FFat* f, Scenario scenario)
{
    (void) scenario;
    
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

static bool test_raw_sector_past_end_of_image(FFat* f, Scenario scenario)
{
    (void) scenario;
    
    f->F_RAWSEC = 0x100000;
    FFatResult r = ffat_op(f, F_READ_RAW, date_time);
    return r == F_IO_ERROR;
}

static bool test_raw_sector_io_error(FFat* f, Scenario scenario)
{
    (void) scenario;
    
    f->F_RAWSEC = 0;
    emulate_io_error = true;
    FFatResult r = ffat_op(f, F_READ_RAW, date_time);
    bool ok = (r == F_IO_ERROR);
    emulate_io_error = false;
    return ok;
}

#if LAYER_IMPLEMENTED >= 1

static bool test_f_init(FFat* f, Scenario scenario)
{
    f->F_PARM = 0;
    X_OK(ffat_op(f, F_INIT, date_time))
    
    if (scenario == scenario_fat32_spc1)
        ASSERT(f->F_SPC == 1)
    else if (scenario == scenario_fat32_spc8)
        ASSERT(f->F_SPC == 8)
    else
        ASSERT(f->F_SPC == 4)
    return true;
}

static bool test_f_boot(FFat* f, Scenario scenario)
{
    X_OK(ffat_op(f, F_BOOT, date_time))
    
    ASSERT(f->buffer[510] == 0x55);
    ASSERT(f->buffer[511] == 0xaa);
    
    return true;
}

static bool test_f_free(FFat* f, Scenario scenario)
{
    DWORD fsfat_free;
    
    X_OK(ffat_op(f, F_FREE, date_time))
    uint32_t free_ = *(uint32_t *) f->buffer;
    
    FATFS* fatfs = calloc(1, sizeof(FATFS));
    R(f_mount(fatfs, "", 0));
    R(f_getfree("", &fsfat_free, &fatfs))
    R(f_mount(NULL, "", 0));
    free(fatfs);
    
    return fsfat_free == free_;
}

#endif  // LAYER_IMPLEMENT >= 1

static const Scenario layer0_scenarios[] = { scenario_raw_sectors, NULL };
#if LAYER_IMPLEMENTED >= 1
static const Scenario layer1_scenarios[] = { scenario_fat32, scenario_fat32_align512, scenario_fat32_spc1, scenario_fat32_spc8, scenario_fat32_2_partitions, scenario_fat16, NULL };
#endif

static const Test test_list_[] = {
        { "Layer 0 sector access", layer0_scenarios, test_raw_sector },
        { "Layer 0 sector past end of image", layer0_scenarios, test_raw_sector_past_end_of_image },
        { "Layer 0 I/O error", layer0_scenarios, test_raw_sector_io_error },
#if LAYER_IMPLEMENTED >= 1
        { "F_INIT", layer1_scenarios, test_f_init },
        { "F_BOOT", layer1_scenarios, test_f_boot },
        { "F_FREE", layer1_scenarios, test_f_free },
#endif
        { NULL, NULL, NULL },
};

const Test* test_list = test_list_;