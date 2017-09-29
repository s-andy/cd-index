/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#ifndef _CD_EXTRACT_H_
#define _CD_EXTRACT_H_

#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

#include "base.h"
#include "data.h"

typedef void* (*cd_extractor_init)(cd_base*);
typedef cd_offset (*cd_extractor_getdata)(const char*, cd_file_entry*, void*);
typedef void (*cd_extractor_finish)(void*);

typedef struct __cd_extractor_info cd_extractor_info;
struct __cd_extractor_info {
    const char* name;
    const char* regex;
    cd_extractor_init init;
    cd_extractor_getdata getdata;
    cd_extractor_finish finish;
    void* __udata;
    regex_t* __regex;
    cd_extractor_info* next;
};

extern cd_extractor_info* cd_extractors;

void cd_init_extractors(cd_base* base);
void cd_free_extractors();

void cd_add_extractor(cd_extractor_info* info, cd_base* base);

cd_extractor_info* cd_find_extractor(const char* file);

void cd_extractor_load_audio(cd_base* base);
void cd_extractor_load_rawimage(cd_base* base);

#endif /* _CD_EXTRACT_H_ */
