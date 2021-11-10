#include "layer1.h"

#include "layer0.h"

#define TRY(expr) { FFatResult r = (expr); if (r != F_OK) return r; }

#define BYTES_PER_SECTOR 512

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
    f->F_ABS = frombuf32(f->buffer, PARTITION_ENTRY_1 + (partition_number * 4) + 8);
    if (f->F_ABS == 0)
        return F_NO_PARTITION;
    return F_OK;
}

static FFatResult load_boot_sector(FFat* f)
{
    f->F_RAWSEC = f->F_ABS; TRY(f_raw_read(f))
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
    f->F_FATST = frombuf16(f->buffer, BPB_RESVD_SEC_CNT);
    f->F_NFATS = f->buffer[BPB_NUM_FATS];
    f->F_SPC = f->buffer[BPB_SEC_PER_CLUS];
    
    uint32_t root_dir_sectors = ((root_ent_cnt * 32) + (BYTES_PER_SECTOR - 1)) / BYTES_PER_SECTOR;
    uint32_t tot_sec = tot_sec_16 ? tot_sec_16 : tot_sec_32;
    f->F_FATSZ = fat_sz_16 ? fat_sz_16 : fat_sz_32;
    
    f->F_DATA = tot_sec - (f->F_FATST + (f->F_NFATS * f->F_FATSZ) + root_dir_sectors);
    uint32_t count_of_clusters = f->F_DATA / f->F_SPC;
    
    if (count_of_clusters < 4085)
        return F_UNSUPPORTED_FS;
    else if (count_of_clusters < 65525)
        f->F_TYPE = FAT16;
    else
        f->F_TYPE = FAT32;
    
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

#define FAT16_EOC 0xffff
#define FAT32_EOC 0x0fffffff

static uint8_t fat_byte_sz(FFat* f)
{
    return f->F_TYPE == FAT16 ? 2 : 4;
}

static FFatResult fat_find_entry(FFat* f, uint8_t fat_number, uint32_t cluster_number, uint32_t* sector, uint16_t* entry_ptr)
{
    uint32_t offset = cluster_number * ((f->F_TYPE == FAT16) ? 2 : 4);
    *sector = f->F_FATST + (fat_number * f->F_FATSZ) + (offset / BYTES_PER_SECTOR);
    *entry_ptr = offset % BYTES_PER_SECTOR;
    
    return F_OK;
}

static FFatResult fat_update_cluster(FFat* f, uint32_t cluster_number, uint32_t data)
{
    for (uint8_t fatn = 0; fatn < f->F_NFATS; ++fatn) {
        // find entry_ptr
        uint32_t sector;
        uint16_t entry_ptr;
        TRY(fat_find_entry(f, fatn, cluster_number, &sector, &entry_ptr))
        
        // load and update sector
        f->F_RAWSEC = f->F_ABS + sector;
        TRY(f_raw_read(f))
        tobuf32(f->buffer, entry_ptr, data);
        TRY(f_raw_write(f))
    }
    
    return F_OK;
}

static FFatResult fat_find_next_free_cluster(FFat* f, uint32_t start_at_cluster, uint32_t* cluster_found)
{
    // TODO
    return F_OK;
}

FFatResult fat_count(FFat* f, uint32_t sector_within_fat, uint32_t* free_count, uint32_t* last_free_sector)
{
    f->F_RAWSEC = f->F_ABS + f->F_FATST + sector_within_fat;
    TRY(f_raw_read(f))
    if (f->F_TYPE == FAT16) {
        for (uint32_t entry_nr = 0; entry_nr < BYTES_PER_SECTOR; entry_nr += sizeof(uint16_t)) {
            uint16_t entry = frombuf16(f->buffer, entry_nr);
            if (entry == FAT_CLUSTER_FREE) {
                ++(*free_count);
                if (*last_free_sector == (uint32_t) -1)
                    *last_free_sector = (sector_within_fat * BYTES_PER_SECTOR / sizeof(uint16_t)) + entry_nr;
            }
        }
        
    } else if (f->F_TYPE == FAT32) {
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
    f->F_RAWSEC = f->F_ABS + FSINFO_SECTOR;
    TRY(f_raw_read(f))
    fs_info->next_free = frombuf32(f->buffer, FSI_NEXT_FREE);
    fs_info->free_clusters = frombuf32(f->buffer, FSI_FREE_COUNT);
    return F_OK;
}

static FFatResult write_fsinfo(FFat* f, FFsInfo* fs_info)
{
    tobuf32(f->buffer, FSI_FREE_COUNT, fs_info->free_clusters);
    tobuf32(f->buffer, FSI_NEXT_FREE, fs_info->next_free);
    f->F_RAWSEC = f->F_ABS + FSINFO_SECTOR;
    TRY(f_raw_write(f))
    return F_OK;
}

static FFatResult recalculate_fsinfo(FFat* f, FFsInfo* fs_info)
{
    fs_info->free_clusters = 0;
    fs_info->next_free = (uint32_t) -1;
    for (uint32_t i = 0; i < f->F_FATSZ; ++i) {
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
    if (f->F_TYPE == FAT16)
        TRY(recalculate_fsinfo(f, &fs_info))
    else
        TRY(load_fsinfo(f, &fs_info))
    tobuf32(f->buffer, 0, fs_info.free_clusters);
    return F_OK;
}

FFatResult f_fsi_calc(FFat* f)
{
    FFsInfo fs_info;
    if (f->F_TYPE == FAT32) {
        TRY(recalculate_fsinfo(f, &fs_info))
        TRY(write_fsinfo(f, &fs_info))
    }
    return F_OK;
}

// endregion

// region -> File management

FFatResult f_create(FFat* f)
{
    // find current space used and next free cluster
    FFsInfo fs_info;
    TRY(load_fsinfo(f, &fs_info))
    
    // create cluster in all FATs
    TRY(fat_update_cluster(f, fs_info.next_free / fat_byte_sz(f), f->F_TYPE == FAT16 ? FAT16_EOC : FAT32_EOC))
    
    // update FSINFO
    TRY(fat_find_next_free_cluster(f, fs_info.next_free, &fs_info.next_free))
    --fs_info.free_clusters;
    TRY(write_fsinfo(f, &fs_info))
    
    return F_OK;
}

// endregion