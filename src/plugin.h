/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#ifndef _CD_PLUGIN_H_
#define _CD_PLUGIN_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <regex.h>

typedef unsigned char bool;
typedef enum {
    FALSE = 0,
    TRUE  = 1
} cd_boolean;

typedef void* (*cd_plugin_init)(void);
typedef void* (*cd_plugin_open)(void*, const char*);
typedef int (*cd_plugin_read)(void*, const char**, const char**, struct stat64*);
typedef void (*cd_plugin_close)(void*);
typedef void (*cd_plugin_finish)(void*);

typedef struct __cd_plugin_info cd_plugin_info;
struct __cd_plugin_info {
    const char* name;
    const char* regex;
    bool ignore_errors;
    cd_plugin_init init;
    cd_plugin_open open;
    cd_plugin_read read;
    cd_plugin_close close;
    cd_plugin_finish finish;
    regex_t* __regex;
    cd_plugin_info* next;
};

extern cd_plugin_info* cd_plugins;

void cd_init_plugins();
void cd_free_plugins();

void cd_add_plugin(cd_plugin_info* info);

cd_plugin_info* cd_find_plugin(const char* file);

// Currently they are loaded so...
void cd_plugin_load_archiver();
void cd_plugin_load_external();
void cd_plugin_unload_external();

#endif /* _CD_PLUGIN_H_ */
