#include "test.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "image.h"
#include "scenario.h"
#include "ff/ff.h"

static FFatResult last_result;

// region -> Helper functions

#define UNUSED __attribute__((unused))
#define R(expr) { FRESULT r = (expr); if (r != FR_OK) { fprintf(stderr, "FSFAT error: %d\n", r); exit(EXIT_FAILURE); } }
#define X_OK(expr) { last_result = (expr); if (last_result != F_OK) return false; }
#define ASSERT(expr) { if (!(expr)) return false; }

static UNUSED void print_fat(FFat* f)
{
    const uint64_t sector = f->F_ABS + f->F_FATST;
    for (size_t i = 0; i < f->F_FATSZ; ++i) {
        f->F_RAWSEC = sector + i;
        ffat_op(f, F_READ_RAW);
        
        if (f->F_TYPE == FAT16) {
            for (size_t j = 0; j < 512; j += 2) {
                printf("%c%02X%02X", j % 32 == 0 ? '\n' : ' ', f->buffer[j+1], f->buffer[j]);
                if (j > 8 && memcmp(&f->buffer[j-8], "\0\0\0\0\0\0\0\0", 8) == 0)
                    goto done;
            }
        } else {
            for (size_t j = 0; j < 512; j += 4) {
                printf("%c%02X%02X%02X%02X", j % 32 == 0 ? '\n' : ' ', f->buffer[j+3], f->buffer[j+2], f->buffer[j+1], f->buffer[j+0]);
                if (j > 8 && memcmp(&f->buffer[j-8], "\0\0\0\0\0\0\0\0", 8) == 0)
                    goto done;
            }
        }
    }
done:
    printf("\n");
}

static uint32_t add_tags_txt(uint32_t* last_cluster)
{
    extern uint8_t _binary_tests_TAGS_TXT_start[];
    extern uint8_t _binary_tests_TAGS_TXT_end[];
    
    FIL fp;
    UINT bw;
    FATFS* fatfs = calloc(1, sizeof(FATFS));
    
    R(f_mount(fatfs, "", 0));
    R(f_open(&fp, "tags.txt", FA_CREATE_NEW | FA_WRITE))
    R(f_write(&fp, _binary_tests_TAGS_TXT_start, (_binary_tests_TAGS_TXT_end - _binary_tests_TAGS_TXT_start), &bw))
    uint32_t file_cluster = fp.obj.sclust;
    if (last_cluster)
        *last_cluster = fp.clust;
    R(f_close(&fp))
    R(f_mount(NULL, "", 0));
    free(fatfs);
    
    return file_cluster;
}

static FFatResult check_fsinfo(FFat* f, uint32_t* last_used, uint32_t* free_clusters)
{
    if (f->F_TYPE == FAT16) {
        if (last_used)
            *last_used = 0;
        if (free_clusters) {
            X_OK(ffat_op(f, F_FREE))
            *free_clusters = *(uint32_t *) f->buffer;
        }
    } else {
        f->F_RAWSEC = f->F_ABS + 1;
        X_OK(ffat_op(f, F_READ_RAW))
        if (last_used)
            *last_used = *(uint32_t *) &f->buffer[492];
        if (free_clusters)
            *free_clusters = *(uint32_t *) &f->buffer[488];
    }
    return F_OK;
}

// endregion

// region -> Layer 0

static bool test_raw_sector(FFat* f, Scenario scenario)
{
    (void) scenario;
    
    f->F_RAWSEC = 0xaf;
    
    X_OK(ffat_op(f, F_READ_RAW))
    ASSERT(f->buffer[0] == 0xaf)
    ASSERT(f->buffer[511] == 0xaf)
    
    memset(f->buffer, 0x33, 512);
    X_OK(ffat_op(f, F_WRITE_RAW))
    memset(f->buffer, 0, 512);
    
    X_OK(ffat_op(f, F_READ_RAW))
    ASSERT(f->buffer[0] == 0x33)
    ASSERT(f->buffer[511] == 0x33)
    ASSERT(img_data[0xaf * SECTOR_SZ] == 0x33);
    
    return true;
}

static bool test_raw_sector_past_end_of_image(FFat* f, Scenario scenario)
{
    (void) scenario;
    
    f->F_RAWSEC = 0x100000;
    FFatResult r = ffat_op(f, F_READ_RAW);
    return r == F_IO_ERROR;
}

static bool test_raw_sector_io_error(FFat* f, Scenario scenario)
{
    (void) scenario;
    
    f->F_RAWSEC = 0;
    emulate_io_error = true;
    FFatResult r = ffat_op(f, F_READ_RAW);
    bool ok = (r == F_IO_ERROR);
    emulate_io_error = false;
    return ok;
}

// endregion

// region -> Layer 1

#if LAYER_IMPLEMENTED >= 1

static bool test_f_init(FFat* f, Scenario scenario)
{
    f->F_PARM = 0;
    X_OK(ffat_op(f, F_INIT))
    
    if (scenario == scenario_fat32_spc1)
        ASSERT(f->F_SPC == 1)
    else if (scenario == scenario_fat32_spc8)
        ASSERT(f->F_SPC == 8)
    else
        ASSERT(f->F_SPC == 4)
    return true;
}

static bool test_f_boot(FFat* f, UNUSED Scenario scenario)
{
    X_OK(ffat_op(f, F_BOOT))
    
    ASSERT(f->buffer[510] == 0x55);
    ASSERT(f->buffer[511] == 0xaa);
    
    return true;
}

static bool test_f_free(FFat* f, Scenario scenario)
{
    X_OK(ffat_op(f, F_FREE))
    uint32_t free1 = *(uint32_t *) f->buffer;
    
    DWORD free2;
    FATFS* fatfs = calloc(1, sizeof(FATFS));
    R(f_mount(fatfs, "", 0));
    R(f_getfree("", &free2, &fatfs))
    R(f_mount(NULL, "", 0));
    free(fatfs);
    
    if (scenario == scenario_fat16)
        ASSERT(free1 > (free2 * 0.8) && free1 < (free1 * 1.2))
    else
        ASSERT(free1 == free2)
        
    return true;
}

static bool test_f_fsi_calc(FFat* f, UNUSED Scenario scenario)
{
    X_OK(ffat_op(f, F_FREE))
    uint32_t free1 = *(uint32_t *) f->buffer;
    
    X_OK(ffat_op(f, F_FSI_CALC))
    X_OK(ffat_op(f, F_FREE))
    uint32_t free2 = *(uint32_t *) f->buffer;
    
    ASSERT(free1 > (free2 * 0.8) && free1 < (free1 * 1.2))
    
    return true;
}

static bool test_f_fsi_calc_nxt_free(FFat* f, UNUSED Scenario scenario)
{
    // check next free cluster before recalculation
    f->F_RAWSEC = f->F_ABS + 1;  // FSInfo sector
    X_OK(ffat_op(f, F_READ_RAW))
    uint32_t nxt_free_1 = *(uint32_t *) &f->buffer[492];
    
    // recalculate
    X_OK(ffat_op(f, F_FSI_CALC))
    
    // check next free cluster after recalculation
    f->F_RAWSEC = f->F_ABS + 1;  // FSInfo sector
    X_OK(ffat_op(f, F_READ_RAW))
    uint32_t nxt_free_2 = *(uint32_t *) &f->buffer[492];
    
    ASSERT(nxt_free_1 == nxt_free_2)
    
    return true;
}

static bool test_f_seek_one(FFat* f, UNUSED Scenario scenario)
{
    uint32_t file_cluster = add_tags_txt(NULL);
    
    f->F_CLSTR = file_cluster;
    f->F_PARM = 1;
    X_OK(ffat_op(f, F_SEEK))
    
    ASSERT(f->F_CLSTR == file_cluster + 1);
    
    return true;
}

static bool test_f_seek_end(FFat* f, UNUSED Scenario scenario)
{
    uint32_t last_cluster;
    uint32_t file_cluster = add_tags_txt(&last_cluster);
    
    f->F_CLSTR = file_cluster;
    f->F_PARM = (uint32_t) -1;
    FFatResult r = ffat_op(f, F_SEEK);
    
    ASSERT(r == F_SEEK_PAST_EOF)
    ASSERT(f->F_CLSTR == last_cluster)
    
    return true;
}

static bool test_f_append_create_file(FFat* f, Scenario scenario)
{
    // get FSInfo before creation
    uint32_t last_used_before, free_before;
    X_OK(check_fsinfo(f, &last_used_before, &free_before))
    
    // create new FAT entry
    f->F_CLSTR = 0;
    X_OK(ffat_op(f, F_APPEND))
    
    ASSERT(f->F_CLSTR > 1 && f->F_CLSTR < 100)
    
    // check if FAT entry was created with EOF
    X_OK(ffat_op(f, F_READ_RAW))
    if (scenario == scenario_fat16)
        ASSERT(*(uint16_t *) &f->buffer[f->F_CLSTR * 2] >= 0xfff8)
    else
        ASSERT((*(uint32_t *) &f->buffer[f->F_CLSTR * 4] & 0x0fffffff) >= 0x0ffffff8)
        
    // get FSInfo after creation
    uint32_t last_used_after, free_after;
    X_OK(check_fsinfo(f, &last_used_after, &free_after))
    
    ASSERT(free_after == free_before - 1)
    if (scenario != scenario_fat16)
        ASSERT(last_used_after > last_used_before)
    
    return true;
}

static bool test_f_append_one_to_new_file(FFat* f, Scenario UNUSED scenario)
{
    // get FSInfo before creation
    uint32_t free_before;
    X_OK(check_fsinfo(f, NULL, &free_before))
    
    // create a new file
    f->F_CLSTR = 0;
    X_OK(ffat_op(f, F_APPEND))
    uint32_t file_cluster = f->F_CLSTR;
    
    ASSERT(file_cluster > 1 && file_cluster < 100)
    
    // append clusters
    X_OK(ffat_op(f, F_APPEND))
    ASSERT(f->F_CLSTR > file_cluster)
    
    // check FAT
    X_OK(ffat_op(f, F_READ_RAW))
    if (scenario == scenario_fat16) {
        ASSERT(*(uint16_t *) &f->buffer[file_cluster * 2] == f->F_CLSTR)
        ASSERT(*(uint16_t *) &f->buffer[f->F_CLSTR * 2] >= 0xfff8)
    } else {
        ASSERT((*(uint32_t *) &f->buffer[file_cluster * 4] & 0x0fffffff) == f->F_CLSTR)
        ASSERT((*(uint32_t *) &f->buffer[f->F_CLSTR * 4] & 0x0fffffff) >= 0x0ffffff8)
    }
    
    // get FSInfo after creation & append
    uint32_t free_after;
    X_OK(check_fsinfo(f, NULL, &free_after))
    
    ASSERT(free_after == free_before - 2)
    
    return true;
}

static bool test_f_append_to_existing_file(FFat* f, UNUSED Scenario scenario)
{
    uint32_t last_cluster;
    uint32_t file_cluster = add_tags_txt(&last_cluster);
    
    // append one cluster
    f->F_CLSTR = file_cluster;
    X_OK(ffat_op(f, F_APPEND))
    ASSERT(f->F_CLSTR > last_cluster)
    
    // check FAT
    X_OK(ffat_op(f, F_READ_RAW))
    if (scenario == scenario_fat16) {
        ASSERT(*(uint16_t *) &f->buffer[last_cluster * 2] == f->F_CLSTR)
        ASSERT(*(uint16_t *) &f->buffer[f->F_CLSTR * 2] >= 0xfff8)
    } else {
        ASSERT((*(uint32_t *) &f->buffer[last_cluster * 4] & 0x0fffffff) == f->F_CLSTR)
        ASSERT((*(uint32_t *) &f->buffer[f->F_CLSTR * 4] & 0x0fffffff) >= 0x0ffffff8)
    }
    
    return true;
}

static bool test_f_truncate_first_sector(FFat* f, UNUSED Scenario scenario)
{
    uint32_t last_cluster;
    uint32_t file_cluster = add_tags_txt(&last_cluster);
    
    // truncate file in first cluster
    f->F_CLSTR = file_cluster;
    X_OK(ffat_op(f, F_TRUNCATE))
    
    // check FAT
    X_OK(ffat_op(f, F_READ_RAW))
    if (scenario == scenario_fat16) {
        ASSERT(*(uint16_t *) &f->buffer[file_cluster * 2] >= 0xfff8)
        ASSERT(*(uint16_t *) &f->buffer[last_cluster * 2] == 0x0)
    } else {
        ASSERT((*(uint32_t *) &f->buffer[file_cluster * 4] & 0x0fffffff) >= 0x0ffffff8)
        ASSERT((*(uint32_t *) &f->buffer[last_cluster * 4] & 0x0fffffff) == 0x0)
    }
    
    return true;
}

static bool test_f_truncate_3rd_sector(FFat* f, UNUSED Scenario scenario)
{
    uint32_t last_cluster;
    uint32_t file_cluster = add_tags_txt(&last_cluster);
    
    // truncate file in first cluster
    f->F_CLSTR = file_cluster + 2;  // we assume the file was created in order
    X_OK(ffat_op(f, F_TRUNCATE))
    
    // check FAT
    X_OK(ffat_op(f, F_READ_RAW))
    if (scenario == scenario_fat16) {
        ASSERT(*(uint16_t *) &f->buffer[file_cluster * 2] < 0xfff8)
        ASSERT(*(uint16_t *) &f->buffer[(file_cluster + 2) * 2] >= 0xfff8)
        ASSERT(*(uint16_t *) &f->buffer[last_cluster * 2] == 0x0)
    } else {
        ASSERT((*(uint32_t *) &f->buffer[file_cluster * 4] & 0x0fffffff) < 0x0ffffff8)
        ASSERT((*(uint32_t *) &f->buffer[(file_cluster + 2) * 4] & 0x0fffffff) >= 0x0ffffff8)
        ASSERT((*(uint32_t *) &f->buffer[last_cluster * 4] & 0x0fffffff) == 0x0)
    }
    
    return true;
}

static bool test_f_remove(FFat* f, UNUSED Scenario scenario)
{
    uint32_t last_cluster;
    uint32_t file_cluster = add_tags_txt(&last_cluster);
 
    // remove file
    f->F_CLSTR = file_cluster;
    X_OK(ffat_op(f, F_REMOVE))
    
    // check FAT
    X_OK(ffat_op(f, F_READ_RAW))
    if (scenario == scenario_fat16) {
        ASSERT(*(uint16_t *) &f->buffer[file_cluster * 2] == 0)
        ASSERT(*(uint16_t *) &f->buffer[(file_cluster + 2) * 2] == 0)
        ASSERT(*(uint16_t *) &f->buffer[last_cluster * 2] == 0x0)
    } else {
        ASSERT((*(uint32_t *) &f->buffer[file_cluster * 4] & 0x0fffffff) == 0)
        ASSERT((*(uint32_t *) &f->buffer[(file_cluster + 2) * 4] & 0x0fffffff) == 0)
        ASSERT((*(uint32_t *) &f->buffer[last_cluster * 4] & 0x0fffffff) == 0x0)
    }
    
    return true;
}

static bool test_f_read_write(FFat* f, UNUSED Scenario scenario)
{
    extern uint8_t _binary_tests_TAGS_TXT_start[];
    
    uint32_t file_cluster = add_tags_txt(NULL);
    
    f->F_CLSTR = file_cluster;
    f->F_SCTR = 1;
    X_OK(ffat_op(f, F_READ_DATA))
    ASSERT(memcmp(f->buffer, &_binary_tests_TAGS_TXT_start[512], 512) == 0)
    
    f->buffer[50] = 'X';
    X_OK(ffat_op(f, F_WRITE_DATA))
    
    memset(f->buffer, 0, 512);
    
    X_OK(ffat_op(f, F_READ_DATA))
    ASSERT(f->buffer[50] == 'X')
    
    return true;
}

#endif  // LAYER_IMPLEMENT >= 1

// endregion

// region -> Scenarios and tests

static const Scenario layer0_scenarios[] = { scenario_raw_sectors, NULL };
#if LAYER_IMPLEMENTED >= 1
static const Scenario layer1_scenarios[] = {
        scenario_fat32,
        scenario_fat32_align512,
        scenario_fat32_spc1,
        scenario_fat32_spc8,
        scenario_fat32_2_partitions,
        scenario_fat16,
        NULL
};
#endif

static const Test test_list_[] = {
        { "Layer 0 sector access", layer0_scenarios, test_raw_sector },
        { "Layer 0 sector past end of image", layer0_scenarios, test_raw_sector_past_end_of_image },
        { "Layer 0 I/O error", layer0_scenarios, test_raw_sector_io_error },
#if LAYER_IMPLEMENTED >= 1
        { "Layer 1 F_INIT", layer1_scenarios, test_f_init },
        { "Layer 1 F_BOOT", layer1_scenarios, test_f_boot },
        { "Layer 1 F_FREE", layer1_scenarios, test_f_free },
        { "Layer 1 F_FSI_CALC (free space)", layer1_scenarios, test_f_fsi_calc },
        { "Layer 1 F_FSI_CALC (next free last_cluster)", layer1_scenarios, test_f_fsi_calc_nxt_free },
        { "Layer 1 F_SEEK (one sector)", layer1_scenarios, test_f_seek_one },
        { "Layer 1 F_SEEK (last sector)", layer1_scenarios, test_f_seek_end },
        { "Layer 1 F_APPEND (create file)", layer1_scenarios, test_f_append_create_file },
        { "Layer 1 F_APPEND (append one to new file)", layer1_scenarios, test_f_append_one_to_new_file },
        { "Layer 1 F_APPEND (append to existing file)", layer1_scenarios, test_f_append_to_existing_file },
        { "Layer 1 F_TRUNCATE (first sector)", layer1_scenarios, test_f_truncate_first_sector },
        { "Layer 1 F_TRUNCATE (3rd sector)", layer1_scenarios, test_f_truncate_3rd_sector },
        { "Layer 1 F_REMOVE", layer1_scenarios, test_f_remove },
        { "Layer 1 F_READ_DATA / F_WRITE_DATA", layer1_scenarios, test_f_read_write },
#endif
        { NULL, NULL, NULL },
};

const Test* test_list = test_list_;

// endregion