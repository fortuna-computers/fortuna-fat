#include "ffat.h"

#include "layer0.h"

FFatResult ffat_op(FFat* f, FFat32Op op, FDateTime date_time)
{
    (void) date_time;
    
    switch (op) {
        case F_READ_RAW:  return f->F_RSLT = f_raw_read(f); break;
        case F_WRITE_RAW: return f->F_RSLT = f_raw_write(f); break;
        default:          return f->F_RSLT = F_INVALID_OP;
    }
}