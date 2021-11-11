#ifndef FORTUNA_FAT_IMAGE_H
#define FORTUNA_FAT_IMAGE_H

#include <stdbool.h>
#include <stdint.h>

#define SECTOR_SZ 512

extern uint64_t img_sz;
extern uint8_t  img_data[];
extern bool     emulate_io_error;

void export_image(const char* filename);

#endif //FORTUNA_FAT_IMAGE_H
