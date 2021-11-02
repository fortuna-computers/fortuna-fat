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
    F_CREATE     = 0x13,
    F_SEEK_FW    = 0x14,
    F_SEEK_EOF   = 0x15,
    F_APPEND     = 0x16,
    F_TRUNCATE   = 0x17,
    F_READ       = 0x18,
    F_WRITE      = 0x19,
#endif
} FFat32Op;

typedef enum FFatResult {
    F_OK                    = 0x00,
    F_INVALID_OP            = 0x01,
    F_IO_ERROR              = 0x02,
#if LAYER_IMPLEMENTED >= 1
    F_UNSUPPORTED_FS        = 0x10,
    F_BPS_NOT_512           = 0x11,
    F_DEVICE_FULL           = 0x12,
    F_SEEK_PAST_EOF         = 0x13,
    F_INVALID_FAT_CLUSTER   = 0x14,
#endif
    F_NOT_IMPLEMENTED       = 0xff,
} FFatResult;

typedef struct __attribute__((__packed__)) FFat {
    uint8_t*     buffer;
    union {
        uint64_t F_RAWSEC;
#if LAYER_IMPLEMENTED >= 1
        struct {
            uint32_t F_CLSTR;
            uint16_t F_SCTR;
        };
        uint8_t  F_PARM;
        uint16_t F_ROOT;
        uint16_t F_SPC;
#endif
    };
    FFatResult   F_RSLT : 8;
} FFat;

typedef struct __attribute__((__packed__)) FDateTime {} FDateTime;

FFatResult ffat_op(FFat* f, FFat32Op op, FDateTime date_time);

// please implement these three functions
bool     raw_write(uint64_t sector, uint8_t const* buffer);
bool     raw_read(uint64_t sector, uint8_t* buffer);
uint64_t total_sectors();

#endif