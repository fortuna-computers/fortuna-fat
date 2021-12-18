#include "layer2.h"

#include <string.h>

#include "layer1.h"
#include "layer0.h"

#define TRY(expr) { FFatResult r = (expr); if (r != F_OK) return r; }

#define DIR_ENTRY_SZ 32

// region -> Initialization

FFatResult f_init_layer2(FFat* f)
{
    if (f->F_TYPE == FAT16)
        f->F_CD_CLSTR = 0;  // special case where 0 = root dir
    else
        f->F_CD_CLSTR = f->F_ROOT;
    return F_OK;
}

// endregion

// region -> Data loading

static FFatResult f_read_sector(FFat* f, uint32_t cluster, uint16_t sector)
{
    if (cluster == 0 && f->F_TYPE == FAT16) {
        f->F_RAWSEC = f->F_ABS + f->F_ROOT;
        TRY(f_raw_read(f))
    } else {
        f->F_CLSTR = cluster;
        f->F_SCTR = sector;
        TRY(f_read_data(f))
    }
    return F_OK;
}

static FFatResult f_write_sector(FFat* f, uint32_t cluster, uint16_t sector)
{
    if (cluster == 0 && f->F_TYPE == FAT16) {
        f->F_RAWSEC = f->F_ABS + f->F_ROOT;
        TRY(f_raw_write(f))
    } else {
        f->F_CLSTR = cluster;
        f->F_SCTR = sector;
        TRY(f_write_data(f))
    }
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

static FFatResult f_foreach_dir_entry(FFat* f, uint32_t cluster, uint16_t sector, FFatResult (*func)(FFat*, DirEntry*, uint32_t, uint16_t, uint16_t, void*), void* data)
{
    TRY(f_read_sector(f, cluster, sector))
    for (uint16_t i = 0; i < 512; i += 32) {
        FFatResult r = func(f, (DirEntry *) &f->buffer[i], cluster, sector, i, data);
        if (r == F_DONE)
            return F_OK;
        else if (r != F_OK)
            return r;
    }

    // TODO - advance to next sector/cluster

    return F_OK;
}

// endregion

// region -> Directory management

typedef struct DirEntryPtr {
    uint32_t cluster;
    uint16_t sector;
    uint16_t index;
} DirEntryPtr;

typedef struct TFindNextDir {
    DirEntryPtr dir_ptr;
    bool        exists;
    bool        empty_entry_found;
} TFindNextDir;

static FFatResult f_find_next_dir(FFat* f, DirEntry* dir_entry, uint32_t cluster, uint16_t sector, uint16_t index, void* data)
{
    (void) f;

    TFindNextDir* find_next_dir = data;
    if (dir_entry->name[0] == 0x0 && !find_next_dir->exists) {
        find_next_dir->dir_ptr = (DirEntryPtr) { cluster, sector, index };
        find_next_dir->empty_entry_found = true;
        return F_DONE;
    } else {
        // TODO - check if the directory has the same name
    }
    return F_OK;
}

static FFatResult f_create_dir_entry(FFat* f, const char* filename, uint32_t cluster, DirEntryPtr const* dir_ptr)
{
    uint32_t dt = current_datetime();

    TRY(f_read_sector(f, dir_ptr->cluster, dir_ptr->sector))

    DirEntry dir_entry = {0};
    memcpy(dir_entry.name, filename, 11);
    dir_entry.attr = DIRECTORY;
    dir_entry.fst_clus_hi = (cluster >> 16);
    dir_entry.fst_clus_lo = (cluster & 0xffff);
    dir_entry.crt_date = dir_entry.wrt_date = (dt >> 16);
    dir_entry.crt_time = dir_entry.wrt_time = (dt & 0xffff);

    memcpy(&f->buffer[dir_ptr->index], &dir_entry, sizeof(DirEntry));
    TRY(f_write_sector(f, dir_ptr->cluster, dir_ptr->sector))

    return F_OK;
}

static FFatResult f_create_empty_dir(FFat* f, uint32_t cluster, uint32_t parent_cluster)
{
    DirEntryPtr dir_ptr = { cluster, 0, 0 };
    TRY(f_create_dir_entry(f, ".", cluster, &dir_ptr))
    dir_ptr.index += DIR_ENTRY_SZ;
    TRY(f_create_dir_entry(f, "..", parent_cluster, &dir_ptr))
    return F_OK;
}

FFatResult f_mkdir_(FFat* f)
{
    // validate filename
    char tmp_filename[12];
    TRY(f_adjust_filename(f))
    memcpy(tmp_filename, f->buffer, 12);

    // check if directory does not already exists, and find where to create the dir entry
    TFindNextDir find_next_dir = { { 0, 0, 0 }, false, false };
    TRY(f_foreach_dir_entry(f, f->F_CD_CLSTR, 0, f_find_next_dir, &find_next_dir))
    if (find_next_dir.exists)
        return F_FILE_ALREADY_EXISTS;
    if (!find_next_dir.empty_entry_found)
        return F_NO_SPACE_LEFT;

    // allocate area for the new directory
    f->F_CLSTR = 0;
    f_append(f);
    uint32_t new_dir_cluster = f->F_CLSTR;

    // create new directory entry in parent
    TRY(f_create_dir_entry(f, tmp_filename, new_dir_cluster, &find_next_dir.dir_ptr))

    // create '.' and '..' in new directory
    TRY(f_create_empty_dir(f, new_dir_cluster, find_next_dir.dir_ptr.cluster))

    return F_OK;
}

// endregion
