/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#ifndef _CD_FIND_H_
#define _CD_FIND_H_

#include <regex.h>

#include "data.h"

#define transparent     __attribute__((__transparent_union__))

typedef enum {
    FIND_WILDCARD = 0x0001,
    FIND_REGEXP   = 0x0002,
    FIND_TYPE     = 0x0003,
    FIND_MTIME    = 0x0004,
    FIND_SIZE     = 0x0005,
    FIND_MASK     = 0x00FF,
    FIND_ICASE    = 0x0100, // for wildcard and regexp
    FIND_EQUAL    = 0x0000, // for time and size
    FIND_LESS     = 0x0100, // for time and size
    FIND_GREATER  = 0x0200, // for time and size
    FIND_FLAGS    = 0xFF00
} cd_find_flags;

typedef struct _cd_find_exp_ cd_find_exp;
struct _cd_find_exp_ {
    cd_find_exp* next;
    int flags;
    union {
        const char* wildcard;
        regex_t* regex;
        cd_file_type type;
        time_t time;
        cd_size size;
    } transparent;
};

typedef struct {
    const char* cdimask;
    const char* path;
    cd_bool nodefdir;
    cd_bool noarc;
    cd_find_exp* exp;
    const char* format;
} cd_find_req;

#endif /* _CD_FIND_H_ */
