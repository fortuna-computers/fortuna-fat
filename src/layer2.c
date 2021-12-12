#include "layer2.h"

#include <string.h>

#include "layer1.h"

#define TRY(expr) { FFatResult r = (expr); if (r != F_OK) return r; }

// region -> Initialization

FFatResult f_init_layer2(FFat* f)
{
    f->F_CD_CLSTR = f->F_ROOT;
    return F_OK;
}

// endregion

// region -> Utils

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

FFatResult f_adjust_filename(FFat* f)
{
    if (f->buffer[0] == '.' || f->buffer[0] == '\0')
        return F_INVALID_FILENAME;

    // copy current string to position 12 - string starting in 0 will be the converted string
    const char* origin = (const char *) &f->buffer[13];
    char* dest = (char *) f->buffer;
    memcpy((char *) &f->buffer[13], dest, 13);
    memset(dest, ' ', 11);
    dest[11] = '\0';

    char c;
    uint8_t orig_i = 0, dest_i = 0;

    bool extension = false;
    while ((c = origin[orig_i]) != '\0') {

        if ((dest_i > 8 && !extension) || (dest_i > 11 && extension)) {
            return F_INVALID_FILENAME;
        } else if (c == '.' && !extension) {
            dest_i = 8;
            extension = true;
        } else if (c < 0x20 || c == 0x22 || (c >= 0x2a && c <= 0x2f) || (c >= 0x3a && c <= 0x3f) || (c >= 0x5b && c <= 0x5d) || c == 0x7c) {
            return F_INVALID_FILENAME;
        } else if (origin[orig_i] >= 'a' && origin[orig_i] <= 'z') {
            dest[dest_i++] = c - ('a' - 'A');
        } else {
            dest[dest_i++] = c;
        }

        ++orig_i;
    }

    return F_OK;
}

static FFatResult f_foreach_dir_entry(FFat* f, uint32_t cluster, uint16_t sector, FFatResult (*func)(FFat*, DirEntry*, void*), void* data)
{
    f->F_CLSTR = cluster;
    f->F_SCTR = sector;
    TRY(f_read_data(f))
    for (uint16_t i = 0; i < 512; i += 32)
        func(f, (DirEntry *) &f->buffer[i], data);
    // TODO - next sector/cluster
    return F_OK;
}

// endregion

// region -> Directory management

typedef struct TFindNextDir {
    uint32_t create_in_cluster;
    uint16_t create_in_sector;
    uint16_t create_in_index;
    bool     exists;
} TFindNextDir;

static FFatResult f_find_next_dir(FFat* f, DirEntry* dir_entry, void* data)
{
    return F_OK;
}

FFatResult f_mkdir_(FFat* f)
{
    // validate filename
    TRY(f_adjust_filename(f))

    // check if directory does not already exists
    TFindNextDir find_next_dir = { 0, 0, 0, false };
    TRY(f_foreach_dir_entry(f, f->F_CD_CLSTR, 0, f_find_next_dir, &find_next_dir))

    // allocate area for the new directory

    // create new directory entry in parent

    // create '.' and '..' in new directory

    return F_NOT_IMPLEMENTED;
}

// endregion