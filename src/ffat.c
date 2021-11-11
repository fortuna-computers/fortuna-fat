#include "ffat.h"

#include "layer0.h"
#if LAYER_IMPLEMENTED >= 1
#  include "layer1.h"
#endif

FFatResult ffat_op(FFat* f, FFat32Op op)
{
    switch (op) {
        case F_READ_RAW:   return f->F_RSLT = f_raw_read(f);
        case F_WRITE_RAW:  return f->F_RSLT = f_raw_write(f);
#if LAYER_IMPLEMENTED >= 1
        case F_INIT:       return f->F_RSLT = f_init(f);
        case F_BOOT:       return f->F_RSLT = f_boot(f);
        case F_FREE:       return f->F_RSLT = f_free(f);
        case F_FSI_CALC:   return f->F_RSLT = f_fsi_calc(f);
        case F_SEEK:       return f->F_RSLT = f_seek(f);
        case F_APPEND:     return f->F_RSLT = f_append(f);
        case F_TRUNCATE:   return f->F_RSLT = f_truncate_(f);
        case F_REMOVE:     return f->F_RSLT = f_remove(f);
        case F_READ_DATA:  return f->F_RSLT = f_read_data(f);
        case F_WRITE_DATA: return f->F_RSLT = f_write_data(f);
#endif
        default:           return f->F_RSLT = F_NOT_IMPLEMENTED;
    }
}