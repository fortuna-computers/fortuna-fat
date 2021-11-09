#include "layer1.h"

#include "layer0.h"

#define TRY(expr) { FFatResult r = (expr); if (r != F_OK) return r; }

#define BYTES_PER_SECTOR 512

typedef enum FFatType { FAT16, FAT32 } FFatType;

typedef struct __attribute__((__packed__)) FPartition {
    uint32_t abs_start_sector;
    uint16_t fat_start_sector;
    uint32_t fat_sz_sectors;
    uint32_t data_sector;
    uint8_t  num_fats : 3;
    FFatType fat_type : 2;
} FPartition;

static FPartition partition;

// region -> Helper functions

static inline uint32_t frombuf16(uint8_t const* buffer, uint16_t pos) { return *(uint16_t *) &buffer[pos]; }
static inline uint32_t frombuf32(uint8_t const* buffer, uint16_t pos) { return *(uint32_t *) &buffer[pos]; }
static inline void tobuf32(uint8_t* buffer, uint16_t pos, uint32_t value) { *(uint32_t *) &buffer[pos] = value; }

// endregion

// region -> Initialization & boot sector

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
    if (frombuf16(f->buffer, BPB_BYTES_PER_SEC) != BYTES_PER_SECTOR)
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
    partition.fat_start_sector = frombuf16(f->buffer, BPB_RESVD_SEC_CNT);
    partition.num_fats = f->buffer[BPB_NUM_FATS];
    f->F_SPC = f->buffer[BPB_SEC_PER_CLUS];
    
    uint32_t root_dir_sectors = ((root_ent_cnt * 32) + (BYTES_PER_SECTOR - 1)) / BYTES_PER_SECTOR;
    uint32_t tot_sec = tot_sec_16 ? tot_sec_16 : tot_sec_32;
    partition.fat_sz_sectors = fat_sz_16 ? fat_sz_16 : fat_sz_32;
    
    partition.data_sector = tot_sec - (partition.fat_start_sector + (partition.num_fats * partition.fat_sz_sectors) + root_dir_sectors);
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

FFatResult f_boot(FFat* f)
{
    TRY(load_boot_sector(f))
    return F_OK;
}

// endregion

// region -> FAT management

#define FAT32_MASK       0x0fffffff
#define FAT_CLUSTER_FREE 0x0

FFatResult fat_count(FFat* f, uint32_t sector_within_fat, uint32_t* free_count, uint32_t* last_free_sector)
{
    f->F_RAWSEC = partition.abs_start_sector + partition.fat_start_sector + sector_within_fat;
    TRY(f_raw_read(f))
    if (partition.fat_type == FAT16) {
        for (uint32_t entry_nr = 0; entry_nr < BYTES_PER_SECTOR; entry_nr += sizeof(uint16_t)) {
            uint16_t entry = frombuf16(f->buffer, entry_nr);
            if (entry == FAT_CLUSTER_FREE) {
                ++(*free_count);
                if (*last_free_sector == (uint32_t) -1)
                    *last_free_sector = (sector_within_fat * BYTES_PER_SECTOR / sizeof(uint16_t)) + entry_nr;
            }
        }
        
    } else if (partition.fat_type == FAT32) {
        for (uint32_t entry_nr = 0; entry_nr < BYTES_PER_SECTOR; entry_nr += sizeof(uint32_t)) {
            uint32_t entry = frombuf32(f->buffer, entry_nr) & FAT32_MASK;
            if (entry == FAT_CLUSTER_FREE) {
                ++(*free_count);
                if (*last_free_sector == (uint32_t) -1)
                    *last_free_sector = (sector_within_fat * BYTES_PER_SECTOR / sizeof(uint32_t)) + entry_nr;
            }
        }
    }
    return F_OK;
}

// endregion

// region -> FSINFO read/update

#define FSINFO_SECTOR       1
#define FSI_FREE_COUNT  0x1e8
#define FSI_NEXT_FREE   0x1ec

typedef struct FFsInfo {
    uint32_t next_free;
    uint32_t free_clusters;
} FFsInfo;

static FFatResult load_fsinfo(FFat* f, FFsInfo* fs_info)
{
    f->F_RAWSEC = partition.abs_start_sector + FSINFO_SECTOR;
    TRY(f_raw_read(f))
    fs_info->next_free = frombuf32(f->buffer, FSI_NEXT_FREE);
    fs_info->free_clusters = frombuf32(f->buffer, FSI_FREE_COUNT);
    return F_OK;
}

static FFatResult write_fsinfo(FFat* f, FFsInfo* fs_info)
{
    tobuf32(f->buffer, FSI_FREE_COUNT, fs_info->free_clusters);
    tobuf32(f->buffer, FSI_NEXT_FREE, fs_info->next_free);
    f->F_RAWSEC = partition.abs_start_sector + FSINFO_SECTOR;
    TRY(f_raw_write(f))
    return F_OK;
}

static FFatResult recalculate_fsinfo(FFat* f, FFsInfo* fs_info)
{
    fs_info->free_clusters = 0;
    fs_info->next_free = (uint32_t) -1;
    for (uint32_t i = 0; i < partition.fat_sz_sectors; ++i) {
        uint32_t last = fs_info->next_free;
        TRY(fat_count(f, i, &fs_info->free_clusters, &last))
        if (fs_info->next_free == (uint32_t) -1)
            fs_info->next_free = last;
    }
    return F_OK;
}

FFatResult f_free(FFat* f)
{
    FFsInfo fs_info;
    if (partition.fat_type == FAT16)
        TRY(recalculate_fsinfo(f, &fs_info))
    else
        TRY(load_fsinfo(f, &fs_info))
    tobuf32(f->buffer, 0, fs_info.free_clusters);
    return F_OK;
}

FFatResult f_fsi_calc(FFat* f)
{
    FFsInfo fs_info;
    if (partition.fat_type == FAT32) {
        TRY(recalculate_fsinfo(f, &fs_info))
        TRY(write_fsinfo(f, &fs_info))
    }
    return F_OK;
}

// endregion