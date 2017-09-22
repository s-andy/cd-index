/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#ifndef _CD_DATA_H_
#define _CD_DATA_H_

#include <stdint.h>

#define CD_NAME_MAX         255

#define packed(NAME)        __attribute__((__packed__)) NAME

#define ISO_VD_BOOT         0

#define CD_INDEX_MARK_LEN   3
#define CD_INDEX_MARK       "CDI"
#define CD_LINKS_MARK       "CDL"
#define CD_INDEX_VERSION    0x01
#define CD_LINKS_VERSION    0x01

typedef enum {
    CD_DIR = 0,     // directory
    CD_ARC = 1,     // archive
    CD_REG = 2,     // regular file
    CD_LNK = 3      // symlink
} cd_file_type;

typedef uint8_t  cd_bool;
typedef uint8_t  cd_byte;
typedef uint8_t  cd_type;
typedef uint16_t cd_mode;
typedef uint16_t cd_uid;
typedef uint16_t cd_gid;
typedef uint16_t cd_word;
typedef uint32_t cd_size;
typedef uint32_t cd_offset;
typedef uint32_t cd_time;

typedef struct {
    char mark[3];           // "CDI"
    cd_byte version;        // 0x01
} packed(cd_index_mark);

typedef struct {
    cd_index_mark mark;
    cd_bool bootable;       // Is bootable?
    char volume_id[32];     // Volume ID
    cd_size size;           // The size of the media
    char publisher[128];    // Name of publisher
    char preparer[128];     // Name of preparer
    char generator[128];    // Name of ISO generator
    cd_time ctime;          // Created
    cd_time mtime;          // Modified
} packed(cd_iso_header);    // 433

typedef struct  {
    cd_offset id;           // ID of the record
    cd_type type;           // Type (dir, symlink or file)
    char name[CD_NAME_MAX]; // File name
    cd_mode mode;           // Access permissions
    cd_time mtime;          // Created time
    cd_uid uid;             // UID
    cd_gid gid;             // GID
    cd_size size;           // Size of the file
    cd_offset info;         // Offset in info database
    cd_offset parent;       // Parent directory
    cd_offset child;        // First file in the directory (for dirs)
    cd_offset next;         // Next file in the directory
} packed(cd_file_entry);    // 290

#endif /* _CD_DATA_H_ */
