#include "ffat.h"

#include "layer0.h"
#if LAYER_IMPLEMENTED >= 1
#  include "layer1.h"
#endif

FFatResult ffat_op(FFat* f, FFat32Op op, FDateTime date_time)
{
    (void) date_time;
    
    switch (op) {
        case F_READ_RAW:  return f->F_RSLT = f_raw_read(f);
        case F_WRITE_RAW: return f->F_RSLT = f_raw_write(f);
#if LAYER_IMPLEMENTED >= 1
        case F_INIT:      return f->F_RSLT = f_init(f);
        case F_BOOT:      return f->F_RSLT = f_boot(f);
        case F_FREE:      return f->F_RSLT = f_free(f);
        case F_FSI_CALC:  return f->F_RSLT = f_fsi_calc(f);
        case F_CREATE:    return f->F_RSLT = f_create(f);
#endif
        default:          return f->F_RSLT = F_NOT_IMPLEMENTED;
    }
}