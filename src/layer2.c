#include "layer2.h"

#include <string.h>

// region -> Initialization

FFatResult f_init_layer2(FFat* f)
{
    f->F_CD_CLSTR = f->F_ROOT;
    return F_OK;
}

// endregion

// region -> Directory management

typedef enum DirAttr {
    READ_ONLY = 0x1,
    HIDDEN    = 0x2,
    SYSTEM    = 0x4,
    VOLUME_ID = 0x8,
    DIRECTORY = 0x10,
    ARCHIVE   = 0x20,
} DirAttr;

typedef struct DirEntry {
    char      name[11];
    DirAttr   attr : 8;
    uint8_t   nt_res;
    uint8_t   crt_time_tenth;
    uint16_t  crt_time;
    uint16_t  crt_date;
    uint16_t  lst_acc_date;
    uint16_t  fst_clus_hi;
    uint16_t  wrt_time;
    uint16_t  wrt_date;
    uint16_t  fst_clus_lo;
    uint32_t  file_size;
} __attribute__((packed)) DirEntry;

FFatResult adjust_filename(char* filename, bool is_file)
{
    if (filename[0] == '.' || filename[0] == '\0')
        return F_INVALID_FILENAME;

    uint8_t idx = 0;
    while (filename[idx] != '\0') {
        if (idx >= 8)
            return F_INVALID_FILENAME;

        char c = filename[idx];
        if (c < 0x20 || c == 0x22 || (c >= 0x2a && c <= 0x2f) || (c >= 0x3a && c <= 0x3f) || (c >= 0x5b && c <= 0x5d) || c == 0x7c)
            return F_INVALID_FILENAME;

        if (filename[idx] >= 'a' && filename[idx] <= 'z')
            filename[idx] -= ('a' - 'A');

        if (c == '.') {
            int length = strlen(filename);
            if (is_file) {
                filename[9] = length > (idx+1) ? filename[idx+1] : ' ';
                filename[10] = length > (idx+2) ? filename[idx+2] : ' ';
                filename[11] = length > (idx+3) ? filename[idx+3] : ' ';
            } else {
                return F_INVALID_FILENAME;
            }
        }

        ++idx;
    }

    // TODO...

    return F_OK;
}

FFatResult f_mkdir_(FFat* f)
{
    // validate filename
    TRY(adjust_filename((char *) f->buffer), false)

    // check if directory does not already exists

    // allocate area for the new directory

    // create new directory entry in parent

    // create '.' and '..' in new directory

    return F_NOT_IMPLEMENTED;
}

// endregion