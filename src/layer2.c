#include "layer2.h"

FFatResult f_init_layer2(FFat* f)
{
    f->F_CD_CLSTR = 0;
    f->F_CD_SCTR = 0;
    return F_OK;
}

FFatResult f_mkdir_(FFat* f)
{
    return F_NOT_IMPLEMENTED;
}
