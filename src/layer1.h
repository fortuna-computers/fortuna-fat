#ifndef FORTUNA_FAT_LAYER1_H
#define FORTUNA_FAT_LAYER1_H

#include "ffat.h"

FFatResult f_init(FFat* f);
FFatResult f_boot(FFat* f);

FFatResult f_free(FFat* f);
FFatResult f_fsi_calc(FFat* f);

FFatResult f_seek(FFat* f);
FFatResult f_append(FFat* f);
FFatResult f_truncate_(FFat* f);
FFatResult f_remove(FFat* f);

FFatResult f_read_data(FFat* f);
FFatResult f_write_data(FFat* f);

#endif
