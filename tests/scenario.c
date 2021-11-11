#include "scenario.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "ff/ff.h"

#define MB (1024 * 1024)

static BYTE work[FF_MAX_SS];

static void R(FRESULT fresult)
{
    if (fresult != FR_OK) {
        fprintf(stderr, "FatFS operation failed.\n");
        exit(EXIT_FAILURE);
    }
}


void (*scenario_list[MAX_SCENARIOS])() = {
        scenario_raw_sectors,
        scenario_fat32,
        scenario_fat32_align512,
        scenario_fat32_spc1,
        scenario_fat32_spc8,
        scenario_fat32_2_partitions,
        scenario_fat16,
        NULL
};

void scenario_raw_sectors()
{
    scenario_name = "Image with raw sectors";
    img_sz = 256;
    for (size_t i = 0; i < 256; ++i)
        memset(&img_data[i * SECTOR_SZ], (uint8_t) i, SECTOR_SZ);
}

void scenario_fat16()
{
    scenario_name = "FAT16 partition";
    
    img_sz = 16 * MB / SECTOR_SZ;
    
    LBA_t lba[] = { 100, 0 };
    R(f_fdisk(0, lba, work));
    
    MKFS_PARM mkfs_parm = { .fmt = FM_FAT, .n_fat = 2, .align = 1 };
    R(f_mkfs("", &mkfs_parm, work, sizeof work));
}

void scenario_fat32()
{
    scenario_name = "FAT32 partition";
    
    img_sz = 256 * MB / SECTOR_SZ;
    
    LBA_t lba[] = { 100, 0 };
    R(f_fdisk(0, lba, work));
    
    MKFS_PARM mkfs_parm = { .fmt = FM_FAT32, .n_fat = 2, .align = 1, .au_size = 4 * 512U };
    R(f_mkfs("", &mkfs_parm, work, sizeof work));
    
    export_image("/tmp/img.img");
}

void scenario_fat32_align512()
{
    scenario_name = "FAT32 partition (512-byte alignment)";
    
    img_sz = 256 * MB / SECTOR_SZ;
    
    LBA_t lba[] = { 100, 0 };
    R(f_fdisk(0, lba, work));
    
    MKFS_PARM mkfs_parm = { .fmt = FM_FAT32, .n_fat = 2, .align = 512, .au_size = 4 * 512U };
    R(f_mkfs("", &mkfs_parm, work, sizeof work));
}

void scenario_fat32_spc8()
{
    scenario_name = "FAT32 partition (8 sec. per cluster)";
    
    img_sz = 512 * MB / SECTOR_SZ;
    
    LBA_t lba[] = { 100, 0 };
    R(f_fdisk(0, lba, work));
    
    MKFS_PARM mkfs_parm = { .fmt = FM_FAT32, .n_fat = 2, .align = 512, .au_size = 8 * 512U };
    R(f_mkfs("", &mkfs_parm, work, sizeof work));
}

void scenario_fat32_spc1()
{
    scenario_name = "FAT32 partition (1 sec. per cluster)";
    
    img_sz = 256 * MB / SECTOR_SZ;
    
    LBA_t lba[] = { 100, 0 };
    R(f_fdisk(0, lba, work));
    
    MKFS_PARM mkfs_parm = { .fmt = FM_FAT32, .n_fat = 2, .align = 512, .au_size = 1 * 512U };
    R(f_mkfs("", &mkfs_parm, work, sizeof work));
}

void scenario_fat32_2_partitions()
{
    scenario_name = "FAT32 (2 partitions)";
    
    img_sz = 512 * MB / SECTOR_SZ;
    
    LBA_t lba[] = { 50, 50, 0 };
    R(f_fdisk(0, lba, work));
    
    MKFS_PARM mkfs_parm = { .fmt = FM_FAT32, .n_fat = 2, .align = 1, .au_size = 4 * 512U };
    R(f_mkfs("0:", &mkfs_parm, work, sizeof work));
    R(f_mkfs("1:", &mkfs_parm, work, sizeof work));
}