#include "layer1.h"

#include "layer0.h"

#define TRY(expr) { FFatResult r = (expr); if (r != F_OK) return r; }

#define PARTITION_ENTRY_1  0x1be

#define BPB_BYTES_PER_SEC  11
#define BPB_ROOT_ENT_COUNT 17
#define BPB_FAT_SZ_16      22
#define BPB_FAT_SZ_32      36
#define BPB_TOT_SEC_16     19
#define BPB_TOT_SEC_32     32
#define BPB_RESVD_SEC_CNT  14
#define BPB_NUM_FATS       16
#define BPB_SEC_PER_CLUS   13


typedef enum FFatType { FAT16, FAT32, FS_OTHER } FFatType;

typedef struct __attribute__((__packed__)) FPartition {
    uint32_t abs_start_sector;
    uint32_t fat_size;
    uint32_t data_sector;
    uint8_t  num_fats : 3;
    FFatType fat_type : 2;
} FPartition;

static FPartition partition;

// region -> helper functions

static inline uint32_t frombuf16(uint8_t const* buffer, uint16_t pos) { return *(uint16_t *) &buffer[pos]; }
static inline uint32_t frombuf32(uint8_t const* buffer, uint16_t pos) { return *(uint32_t *) &buffer[pos]; }

// endregion

// region -> Initialization

static FFatResult set_partition_start(FFat* f, uint8_t partition_number)
{
    f->F_RAWSEC = 0; TRY(f_raw_read(f))
    partition.abs_start_sector = frombuf32(f->buffer, PARTITION_ENTRY_1 + (partition_number * 4) + 8);
    if (partition.abs_start_sector == 0)
        return F_NO_PARTITION;
    return F_OK;
}

static FFatResult load_boot_sector(FFat* f)
{
    f->F_RAWSEC = partition.abs_start_sector; TRY(f_raw_read(f))
    return F_OK;
}

static FFatResult check_bootsector_valid(FFat* f)
{
    if (f->buffer[0] != 0xeb && f->buffer[0] != 0xe9)
        return F_UNSUPPORTED_FS;
    if (f->buffer[510] != 0x55 || f->buffer[511] != 0xaa)
        return F_UNSUPPORTED_FS;
    if (frombuf16(f->buffer, BPB_BYTES_PER_SEC) != 512)
        return F_BPS_NOT_512;
    return F_OK;
}

static FFatResult parse_bpb_and_set_fat_type(FFat* f)
{
    uint16_t root_ent_cnt = frombuf32(f->buffer, BPB_ROOT_ENT_COUNT);
    uint16_t fat_sz_16 = frombuf16(f->buffer, BPB_FAT_SZ_16);
    uint32_t fat_sz_32 = frombuf32(f->buffer, BPB_FAT_SZ_32);
    uint16_t tot_sec_16 = frombuf16(f->buffer, BPB_TOT_SEC_16);
    uint32_t tot_sec_32 = frombuf32(f->buffer, BPB_TOT_SEC_32);
    uint16_t resvd_sec_cnt = frombuf16(f->buffer, BPB_RESVD_SEC_CNT);
    partition.num_fats = f->buffer[BPB_NUM_FATS];
    f->F_SPC = f->buffer[BPB_SEC_PER_CLUS];
    
    uint32_t root_dir_sectors = ((root_ent_cnt * 32) + 511) / 512;
    uint32_t tot_sec = tot_sec_16 ? tot_sec_16 : tot_sec_32;
    partition.fat_size = fat_sz_16 ? fat_sz_16 : fat_sz_32;
    
    partition.data_sector = tot_sec - (resvd_sec_cnt + (partition.num_fats * partition.fat_size) + root_dir_sectors);
    uint32_t count_of_clusters = partition.data_sector / f->F_SPC;
    
    if (count_of_clusters < 4085)
        return F_UNSUPPORTED_FS;
    else if (count_of_clusters < 65525)
        partition.fat_type = FAT16;
    else
        partition.fat_type = FAT32;
    
    return F_OK;
}

FFatResult f_init(FFat* f)
{
    uint8_t partition_number = f->F_PARM;
    TRY(set_partition_start(f, partition_number))
    
    TRY(load_boot_sector(f))
    TRY(check_bootsector_valid(f))
    
    TRY(parse_bpb_and_set_fat_type(f))
    
    return F_OK;
}

// endregion