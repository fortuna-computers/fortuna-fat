#include "layer1.h"

#include "layer0.h"

#define TRY(expr) { FFatResult r = (expr); if (r != F_OK) return r; }

#define PARTITION_ENTRY_1  0x1be

typedef enum FFatType { FAT16, FAT32, FS_OTHER } FFatType;

static uint64_t partition_start_sector;
static uint8_t  fat_type;

// region -> helper functions

static uint32_t frombuf32(uint8_t const* buffer, uint16_t pos) { return *(uint32_t *) &buffer[pos]; }

// endregion

// region -> Initialization

static FFatResult find_partition_start(FFat* f, uint8_t partition, uint64_t* start)
{
    f->F_RAWSEC = 0; TRY(f_raw_read(f))
    *start = frombuf32(f->buffer, PARTITION_ENTRY_1 + (partition * 4));
    if (*start == 0)
        return F_NO_PARTITION;
    return F_OK;
}

static FFatResult load_boot_sector(FFat* f)
{
    f->F_RAWSEC = partition_start_sector; TRY(f_raw_read(f))
    return F_OK;
}

static FFatType find_fat_type(uint8_t const* buffer)
{
    return FS_OTHER;
}

FFatResult f_init(FFat* f)
{
    uint8_t partition = f->F_PARM;
    TRY(find_partition_start(f, partition, &partition_start_sector))
    
    TRY(load_boot_sector(f))
    if (f->buffer[510] != 0x55 || f->buffer[511] != 0xaa)
        return F_UNSUPPORTED_FS;
    
    fat_type = find_fat_type(f->buffer);
    if (fat_type == FS_OTHER)
        return F_UNSUPPORTED_FS;
    
    return F_OK;
}

// endregion