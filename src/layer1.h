#ifndef FORTUNA_FAT_LAYER1_H
#define FORTUNA_FAT_LAYER1_H

#include "ffat.h"

FFatResult f_init(FFat* f);
FFatResult f_boot(FFat* f);

FFatResult f_free(FFat* f);
FFatResult f_fsi_calc(FFat* f);

FFatResult f_create(FFat* f);

#endif
