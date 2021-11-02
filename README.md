# fortuna-fat32

[![Automated tests](https://github.com/fortuna-computers/fortuna-fat/actions/workflows/automated-tests.yml/badge.svg?branch=master)](https://github.com/fortuna-computers/fortuna-fat32/actions/workflows/automated-tests.yml)
[![Code size](https://github.com/fortuna-computers/fortuna-fat/actions/workflows/code-size.yml/badge.svg?branch=master)](https://github.com/fortuna-computers/fortuna-fat32/actions/workflows/code-size.yml)

This is a very small very small (&lt; 8 kB) and memory conscious (&lt; 40 bytes + a shared 512 byte buffer) C11 code for accessing FAT images.
Compilable to both AVR and x64, for use in Fortuna computers and emulator.

To use in your project, just add the files in the `src/` directory to the project source.

It is implemented in a series of layers:

* **Layer 0**: raw access to 512-bytes sectors in the device;
* **Layer 1**: FAT aware access to sectors. Permit creating/navigating/resizing files. Does not understand the concept of directory listings, treats everything as a file. Keeps FSINFO updated.
* **Layer 2**: Understand directory listings. Can update file date and size. Can create/delete and list directories. (NOT IMPLEMENTED YET)

Each layer builds on top of the previous one.

Layers can be added or removed from the implementation by setting the define `LAYER_IMPLEMENTED`. For example, to enable
only layers 0 and 1, use:

```c
#define LAYER_IMPLEMENTED 1
```

---
## Layer 0 (raw access)

### Registers

| Name | Description | Size | RW |
|------|-------------|------|----|
| `F_RSLT` | Result of the last operation (see below). |  8-bit | Read-only |
| `F_RAWSEC` | Number of the 512-byte sector in the device. | 64-bit | Read/write |

### Supported operations

| Operation | Description |
|-----------|-------------|
| `F_READ_RAW` | Reads a 512-byte sector from the device to the buffer. |
| `F_WRITE_RAW` | Writes the buffer into a 512-byte sector in the device. |

## Possible responses (`F_RSLT`)

| Code | Response |
|------|----------|
| `F_OK` | Operation successful. |
| `F_IO_ERROR` | I/O error in the device. |
| `F_INVALID_OP` | A non-existing operation was requested. |

---
## Layer 1 (FAT, FSINFO and data clusters)

All requests from layer 0 are also available in layer 1.

Layer 1 mostly allows navigation and modification of the FAT. Creating a new file will update the FAT and FSINFO, but will
not create a new file in the directory. This needs to be done in a superior layer.

### Registers

| Name | Description | Size | RW |
|------|-------------|------|----|
| `F_CLSTR` | Number of the data cluster to which the operation refers to. | 32-bit | Read/write |
| `F_SCTR`  | Number of the sector inside the cluster. | 16-bit | Read/write |
| `F_PARM`  | Additional parameter used in some operations. | 8-bit | Read/write |
| `F_ROOT`  | Number of the root directory sector (relative to start of data sectors). | 16-bit | Read-only
| `F_SPC`   | Number of sectors per cluster | 16-bit | Read-only

### Supported operations

| Operation    | Description | Additional information |
|--------------|-------------|-------|
| `F_INIT`     | Reads a partition boot sector and initialize the partition. When changing partitions, new `F_INIT` must be issued. | `F_PARM`: partition number |
| `F_BOOT`     | Load boot sector into buffer. |
| `F_FREE`     | Returns number of free clusters into first 4 bytes of buffer. |
| `F_CREATE`   | Create a new file in FAT. |
| `F_SEEK_FW`  | Move* forward a number of sectors | `F_PARM`: number of sectors to move forward. |
| `F_SEEK_EOF` | Move* until the end of file. |
| `F_APPEND`   | Move* until the end of file and append a new sector. |
| `F_TRUNCATE` | Limit the file size to the one specified in `F_CLSTR` and `F_SCTR`, clearing all following FAT entries. |
| `F_READ`     | Read a sector in a data cluster. |
| `F_WRITE`    | Write a sector in a data cluster. |

* Move operations will change `F_CLSTR` and `F_SCTR`, always following the linked list in FAT until the specified parameter.

## Possible responses (`F_RSLT`)

| Code | Response |
|------|----------|
| `F_UNSUPPORTED_FS`  | The file system is not FAT16 or FAT32. |
| `F_BPS_NOT_512`     | Bytes per sector is not 512. |
| `F_DEVICE_FULL`     | There's no space left in the device. |
| `F_SEEK_PAST_EOF`   | A seek command moved the pointer past EOF. |
| `F_INVALID_FAT_CLUSTER` | A command was issued on a invalid FAT cluster. |

---

## Layer 2 (files and directories)

Layer 2 is not yet implemented, but will have the following functionalities:

* Navigate between directories
* Create/delete files
* Read/write files
* Create/delete directories
* Stat files and directories
* Move and rename files and directories
