#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "../tests/ff/ff.h"
#include "../tests/ff/diskio.h"

#define IMG_SZ  (256 * 1024 * 1024)
#define SECTOR_SZ 512

static BYTE work[FF_MAX_SS];
static uint8_t  img_data[IMG_SZ];

bool raw_write(uint64_t sector, uint8_t const* buffer)
{
    memcpy(&img_data[sector * SECTOR_SZ], buffer, SECTOR_SZ);
    return true;
}

bool raw_read(uint64_t sector, uint8_t* buffer)
{
    memcpy(buffer, &img_data[sector * SECTOR_SZ], SECTOR_SZ);
    return true;
}

static void R(FRESULT fresult)
{
    if (fresult != FR_OK) {
        fprintf(stderr, "FatFS operation failed.\n");
        exit(EXIT_FAILURE);
    }
}

int main()
{
    memset(img_data, 0, IMG_SZ);

    LBA_t lba[] = { 100, 0 };
    R(f_fdisk(0, lba, work));

    MKFS_PARM mkfs_parm = { .fmt = FM_FAT32, .n_fat = 2, .align = 1, .au_size = 4 * 512U };
    R(f_mkfs("", &mkfs_parm, work, sizeof work));

    FATFS* fatfs = calloc(1, sizeof(FATFS));
    R(f_mount(fatfs, "", 0));
    R(f_mkdir("temp"));
    R(f_mount(NULL, "", 0));
    free(fatfs);

    fwrite(img_data, IMG_SZ, 1, stdout);
}

//
// functions below are implemented for FSFAT
//


PARTITION VolToPart[FF_VOLUMES] = {
        {0, 1},
        {0, 2}
};

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
        BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
    (void) pdrv;
    return 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
        BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
    (void) pdrv;
    return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
        BYTE pdrv,		/* Physical drive nmuber to identify the drive */
        BYTE *buff,		/* Data buffer to store read data */
        LBA_t sector,	/* Start sector in LBA */
        UINT count		/* Number of sectors to read */
)
{
    (void) pdrv;
    memcpy(buff, &img_data[sector * 512], count * 512);
    return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
        BYTE pdrv,			/* Physical drive nmuber to identify the drive */
        const BYTE *buff,	/* Data to be written */
        LBA_t sector,		/* Start sector in LBA */
        UINT count			/* Number of sectors to write */
)
{
    (void) pdrv;
    memcpy(&img_data[sector * 512], buff, count * 512);
    return RES_OK;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
        BYTE pdrv,		/* Physical drive nmuber (0..) */
        BYTE cmd,		/* Control code */
        void *buff		/* Buffer to send/receive control data */
)
{
    (void) pdrv;

    switch (cmd) {
        case CTRL_SYNC:
            return RES_OK;
        case GET_SECTOR_COUNT:
            *(LBA_t*) buff = IMG_SZ / SECTOR_SZ;
            return RES_OK;
        default:
            abort();
    }
}

DWORD get_fattime (void)
{
    return 0;
}
