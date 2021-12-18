#include "layer1.h"

#include <stddef.h>

#include "layer0.h"

#define TRY(expr) { FFatResult r = (expr); if (r != F_OK) return r; }

#define BYTES_PER_SECTOR 512

// region -> Helper functions

static inline uint32_t frombuf16(uint8_t const* buffer, uint16_t pos) { return *(uint16_t *) &buffer[pos]; }
static inline uint32_t frombuf32(uint8_t const* buffer, uint16_t pos) { return *(uint32_t *) &buffer[pos]; }
static inline void tobuf16(uint8_t* buffer, uint16_t pos, uint16_t value) { *(uint16_t *) &buffer[pos] = value; }
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
#define BPB_ROOTCLUS       44

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
    f->F_ROOT_DIRS = root_ent_cnt;

    uint32_t root_dir_sectors = ((root_ent_cnt * 32) + (BYTES_PER_SECTOR - 1)) / BYTES_PER_SECTOR;
    uint32_t tot_sec = tot_sec_16 ? tot_sec_16 : tot_sec_32;
    f->F_FATSZ = fat_sz_16 ? fat_sz_16 : fat_sz_32;
    
    f->F_DATA = (f->F_FATST + (f->F_NFATS * f->F_FATSZ) + root_dir_sectors);
    uint32_t count_of_clusters = (tot_sec - f->F_DATA) / f->F_SPC;
    
    if (count_of_clusters < 4085) {
        return F_UNSUPPORTED_FS;
    } else if (count_of_clusters < 65525) {
        f->F_TYPE = FAT16;
        f->F_ROOT = f->F_FATST + (f->F_NFATS * f->F_FATSZ);
    } else {
        f->F_TYPE = FAT32;
        f->F_ROOT = f->buffer[BPB_ROOTCLUS];
    }

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

#define FAT16_EOF 0xfff8
#define FAT32_EOF 0x0ffffff8

static __attribute__((unused)) uint8_t fat_byte_sz(FFat* f)
{
    return f->F_TYPE == FAT16 ? 2 : 4;
}

static FFatResult fat_find_entry(FFat* f, uint8_t fat_number, uint32_t cluster_number, uint32_t* sector, uint16_t* entry_ptr)
{
    uint32_t offset = cluster_number * ((f->F_TYPE == FAT16) ? 2 : 4);
    if (sector)
        *sector = (fat_number * f->F_FATSZ) + (offset / BYTES_PER_SECTOR);
    if (entry_ptr)
        *entry_ptr = offset % BYTES_PER_SECTOR;
    
    return F_OK;
}

static FFatResult fat_update_cluster(FFat* f, uint32_t cluster_number, uint32_t data)
{
    if (cluster_number > f->F_FATSZ)
        return F_INVALID_FAT_CLUSTER;
    
    for (uint8_t fatn = 0; fatn < f->F_NFATS; ++fatn) {
        // find entry_ptr
        uint32_t sector;
        uint16_t entry_ptr;
        TRY(fat_find_entry(f, fatn, cluster_number, &sector, &entry_ptr))
        
        // load and update sector
        f->F_RAWSEC = f->F_ABS + f->F_FATST + sector;
        TRY(f_raw_read(f))
        if (f->F_TYPE == FAT16)
            tobuf16(f->buffer, entry_ptr, data);
        else
            tobuf32(f->buffer, entry_ptr, data);
        TRY(f_raw_write(f))
    }
    
    return F_OK;
}

static FFatResult fat_get_cluster(FFat* f, uint32_t cluster_number, uint32_t* data)
{
    if (cluster_number > f->F_FATSZ)
        return F_INVALID_FAT_CLUSTER;
    
    // find entry
    uint32_t sector;
    uint16_t entry_ptr;
    TRY(fat_find_entry(f, 0, cluster_number, &sector, &entry_ptr))
    
    // load sector
    f->F_RAWSEC = f->F_ABS + f->F_FATST + sector;
    TRY(f_raw_read(f))
    *data = (f->F_TYPE == FAT16) ? frombuf16(f->buffer, entry_ptr) : frombuf32(f->buffer, entry_ptr);
    
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
                if (free_count)
                    ++(*free_count);
            } else if (last_free_sector) {
                *last_free_sector = (sector_within_fat * BYTES_PER_SECTOR / sizeof(uint16_t)) + (entry_nr / 2);
            }
        }
        
    } else if (f->F_TYPE == FAT32) {
        for (uint32_t entry_nr = 0; entry_nr < BYTES_PER_SECTOR; entry_nr += sizeof(uint32_t)) {
            uint32_t entry = frombuf32(f->buffer, entry_nr) & FAT32_MASK;
            if (entry == FAT_CLUSTER_FREE) {
                if (free_count)
                    ++(*free_count);
            } else if (last_free_sector) {
                *last_free_sector = (sector_within_fat * BYTES_PER_SECTOR / sizeof(uint32_t)) + (entry_nr / 4);
            }
        }
    }
    return F_OK;
}

static FFatResult fat_find_next_free_cluster(FFat* f, uint32_t start_at_cluster, uint32_t* cluster_found)
{
    uint32_t starting_sector;
    TRY(fat_find_entry(f, 0, start_at_cluster, &starting_sector, NULL))
    
    for (uint32_t sector = starting_sector; sector < f->F_FATSZ; ++sector) {
        f->F_RAWSEC = f->F_ABS + f->F_FATST;
        TRY(f_raw_read(f))
        for (uint32_t j = 0; j < 512; j += fat_byte_sz(f)) {
            uint32_t value = (f->F_TYPE == FAT16) ? frombuf16(f->buffer, j) : frombuf32(f->buffer, j);
            if (value == FAT_CLUSTER_FREE) {
                *cluster_found = (sector / f->F_SPC) + (j / fat_byte_sz(f));
                return F_OK;
            }
        }
    }
    return F_DEVICE_FULL;
}

// endregion

// region -> FSINFO read/update

#define FSINFO_SECTOR       1
#define FSI_FREE_COUNT  0x1e8
#define FSI_NEXT_FREE   0x1ec

typedef struct FFsInfo {
    uint32_t last_cluster_allocated;
    uint32_t free_clusters;
} FFsInfo;

static FFatResult load_fsinfo(FFat* f, FFsInfo* fs_info)
{
    f->F_RAWSEC = f->F_ABS + FSINFO_SECTOR;
    TRY(f_raw_read(f))
    fs_info->last_cluster_allocated = frombuf32(f->buffer, FSI_NEXT_FREE);
    fs_info->free_clusters = frombuf32(f->buffer, FSI_FREE_COUNT);
    return F_OK;
}

static FFatResult write_fsinfo(FFat* f, FFsInfo* fs_info)
{
    tobuf32(f->buffer, FSI_FREE_COUNT, fs_info->free_clusters);
    tobuf32(f->buffer, FSI_NEXT_FREE, fs_info->last_cluster_allocated);
    f->F_RAWSEC = f->F_ABS + FSINFO_SECTOR;
    TRY(f_raw_write(f))
    return F_OK;
}

static FFatResult recalculate_fsinfo(FFat* f, FFsInfo* fs_info)
{
    fs_info->free_clusters = 0;
    fs_info->last_cluster_allocated = (uint32_t) -1;
    for (uint32_t i = 0; i < f->F_FATSZ; ++i) {
        uint32_t last = fs_info->last_cluster_allocated;
        TRY(fat_count(f, i, &fs_info->free_clusters, &last))
        if (fs_info->last_cluster_allocated == (uint32_t) -1)
            fs_info->last_cluster_allocated = last;
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

FFatResult f_seek(FFat* f)
{
    if (f->F_PARM == 0)
        return F_OK;
    
    uint32_t next_cluster = f->F_CLSTR;
    
    for (uint32_t i = 0; i < f->F_PARM; ++i) {
        TRY(fat_get_cluster(f, f->F_CLSTR, &next_cluster))   // TODO - don't load FAT from disk every time
        if (next_cluster >= ((f->F_TYPE == FAT16) ? FAT16_EOF : FAT32_EOF)) {
            return F_SEEK_PAST_EOF;
        } else {
            f->F_CLSTR = next_cluster;
        }
    }
    
    return F_OK;
}

FFatResult f_append(FFat* f)
{
    // goto to last cluster
    if (f->F_CLSTR != 0) {
        f->F_PARM = (uint32_t) -1;
        f_seek(f);
    }
    
    uint32_t previous_cluster = f->F_CLSTR;
    
    // find current space used and next free cluster
    if (f->F_TYPE == FAT32) {
        FFsInfo fs_info;
        TRY(load_fsinfo(f, &fs_info))
        
        // create next_free_cluster in all FATs
        uint32_t next_free_cluster;
        TRY(fat_find_next_free_cluster(f, fs_info.last_cluster_allocated, &next_free_cluster))
        TRY(fat_update_cluster(f, next_free_cluster, FAT32_EOC))
    
        // redirect previous cluster to new cluster
        if (previous_cluster != 0)
            TRY(fat_update_cluster(f, previous_cluster, next_free_cluster))
        
        // update FSINFO
        fs_info.last_cluster_allocated = next_free_cluster;
        --fs_info.free_clusters;
        TRY(write_fsinfo(f, &fs_info))
    
        f->F_CLSTR = next_free_cluster;
        
    } else {
        uint32_t next_free_cluster;
        TRY(fat_find_next_free_cluster(f, 0, &next_free_cluster))
        TRY(fat_update_cluster(f, next_free_cluster, FAT16_EOC))
        if (previous_cluster != 0)
            TRY(fat_update_cluster(f, previous_cluster, next_free_cluster))
        f->F_CLSTR = next_free_cluster;
    }
    
    return F_OK;
}

FFatResult f_truncate_(FFat* f)
{
    uint32_t current_cluster = f->F_CLSTR;
    uint32_t next_cluster;
    
    bool first = true;
    do {
        TRY(fat_get_cluster(f, current_cluster, &next_cluster))
        TRY(fat_update_cluster(f, current_cluster, first ? (f->F_TYPE == FAT16 ? FAT16_EOF : FAT32_EOF) : FAT_CLUSTER_FREE))
        current_cluster = next_cluster;
        first = false;
    } while (current_cluster < (f->F_TYPE == FAT16 ? FAT16_EOF : FAT32_EOF));
    
    return F_OK;
}

FFatResult f_remove(FFat* f)
{
    TRY(f_truncate_(f))
    TRY(fat_update_cluster(f, f->F_CLSTR, FAT_CLUSTER_FREE))
    return F_OK;
}

FFatResult f_read_data(FFat* f)
{
    f->F_RAWSEC = f->F_ABS + f->F_DATA + ((f->F_CLSTR - 2) * f->F_SPC) + f->F_SCTR;
    TRY(f_raw_read(f))
    return F_OK;
}

FFatResult f_write_data(FFat* f)
{
    f->F_RAWSEC = f->F_ABS + f->F_DATA + ((f->F_CLSTR - 2) * f->F_SPC) + f->F_SCTR;
    TRY(f_raw_write(f))
    return F_OK;
}


// endregion
