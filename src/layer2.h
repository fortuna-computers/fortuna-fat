#ifndef FORTUNA_FAT_LAYER2_H
#define FORTUNA_FAT_LAYER2_H

#include "ffat.h"

FFatResult f_init_layer2(FFat* f);

FFatResult f_adjust_filename(FFat* f);
FFatResult f_mkdir_(FFat* f);

#endif
