#ifndef FFAT_H_
#define FFAT_H_

#include <stdbool.h>
#include <stdint.h>

#ifndef LAYER_IMPLEMENTED
#  define LAYER_IMPLEMENTED 0
#endif

typedef enum FFat32Op {
    F_READ_RAW   = 0x0,
    F_WRITE_RAW  = 0x1,
} FFat32Op;

typedef enum FFatResult {
    F_OK              = 0x00,
    F_INVALID_OP      = 0x01,
    F_IO_ERROR        = 0x02,
    F_NOT_IMPLEMENTED = 0xff,
} FFatResult;

typedef struct FFat {
    uint8_t*     buffer;
    union {
        uint64_t F_RAWSEC;
    };
    FFatResult   F_RSLT : 8;
} FFat;

typedef struct FDateTime {} FDateTime;

FFatResult ffat_op(FFat* f, FFat32Op op, FDateTime date_time);

// please implement these three functions
bool     raw_write(uint64_t sector, uint8_t const* buffer);
bool     raw_read(uint64_t sector, uint8_t* buffer);
uint64_t total_sectors();

#endif