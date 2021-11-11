#ifndef FFAT_H_
#define FFAT_H_

#include <stdbool.h>
#include <stdint.h>

#ifndef LAYER_IMPLEMENTED
#  define LAYER_IMPLEMENTED 1
#endif

typedef enum FFat32Op {
    F_READ_RAW   = 0x00,
    F_WRITE_RAW  = 0x01,
#if LAYER_IMPLEMENTED >= 1
    F_INIT       = 0x10,
    F_BOOT       = 0x11,
    F_FREE       = 0x12,
    F_FSI_CALC   = 0x13,
    F_SEEK       = 0x14,
    F_APPEND     = 0x15,
    F_TRUNCATE   = 0x16,
    F_READ       = 0x17,
    F_WRITE      = 0x18,
#endif
} FFat32Op;

typedef enum FFatResult {
    F_OK                    = 0x00,
    F_IO_ERROR              = 0x02,
#if LAYER_IMPLEMENTED >= 1
    F_UNSUPPORTED_FS        = 0x10,
    F_BPS_NOT_512           = 0x11,
    F_DEVICE_FULL           = 0x12,
    F_SEEK_PAST_EOF         = 0x13,
    F_INVALID_FAT_CLUSTER   = 0x14,
    F_NO_PARTITION          = 0x15,
#endif
    F_NOT_IMPLEMENTED       = 0xff,
} FFatResult;

typedef enum FFatType { FAT16 = 0, FAT32 = 1 } FFatType;

typedef struct __attribute__((__packed__)) FFat {
    uint8_t* buffer;          // buffer to exchange data between computer and device
    uint64_t F_RAWSEC;        // sector parameter (when using layer 0)
#if LAYER_IMPLEMENTED >= 1
    uint32_t F_CLSTR;         // cluster parameter
    uint16_t F_SCTR;          // sector parameter (sector count starting on cluster)
    uint32_t F_PARM;          // additional parameter
    uint16_t F_ROOT;          // root directory sector
    uint8_t  F_SPC;           // sectors per cluster
    uint32_t F_ABS;           // partition sector start (from beginning of disk)
    uint16_t F_FATST;         // FAT starting sector
    uint32_t F_FATSZ;         // FAT size
    uint32_t F_DATA;          // Data starting sector
    uint8_t  F_NFATS : 3;     // Number of fats
    FFatType F_TYPE : 2;      // Filesystem type (FAT16/32)
#endif
    FFatResult F_RSLT : 8;   // result of the last operation
} FFat;

FFatResult ffat_op(FFat* f, FFat32Op op);

// please implement these three functions
bool     raw_write(uint64_t sector, uint8_t const* buffer);
bool     raw_read(uint64_t sector, uint8_t* buffer);
uint64_t total_sectors();

#endif