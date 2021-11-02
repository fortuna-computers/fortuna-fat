#include "layer0.h"

FFatResult f_raw_read(FFat* f)
{
    if (f->F_RAWSEC >= total_sectors())
        return F_IO_ERROR;
    return raw_read(f->F_RAWSEC, f->buffer);
}

FFatResult f_raw_write(FFat* f)
{
    if (f->F_RAWSEC >= total_sectors())
        return F_IO_ERROR;
    return raw_write(f->F_RAWSEC, f->buffer);
}
