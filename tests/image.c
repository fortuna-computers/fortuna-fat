#include "image.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ff/ff.h"
#include "ff/diskio.h"
#include "test.h"

#define MAX_IMG_SZ  (512 * 1024 * 1024)

uint64_t img_sector_count = 0;
uint8_t  img_data[MAX_IMG_SZ];
bool     emulate_io_error = false;

bool raw_write(uint64_t sector, uint8_t const* buffer)
{
    if (emulate_io_error)
        return false;
    memcpy(&img_data[sector * SECTOR_SZ], buffer, SECTOR_SZ);
    return true;
}

bool raw_read(uint64_t sector, uint8_t* buffer)
{
    if (emulate_io_error)
        return false;
    memcpy(buffer, &img_data[sector * SECTOR_SZ], SECTOR_SZ);
    return true;
}

uint64_t total_sectors()
{
    return img_sector_count;
}

uint32_t current_datetime()
{
    return (((uint32_t) EXPECTED_DATE) << 16) | EXPECTED_TIME;
}

void export_image(const char* filename)
{
    FILE* f = fopen(filename, "w");
    size_t r = fwrite(img_data, img_sector_count * 512, 1, f);
    if (r <= 0)
        abort();
    fclose(f);
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
            *(LBA_t*) buff = img_sector_count;
            return RES_OK;
        default:
            abort();
    }
}

DWORD get_fattime (void)
{
    return 0;
}
