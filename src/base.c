/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cdindex.h"
#include "base.h"

#define CD_BASE_EXT     ".cdi"
#define CD_SLINKS_EXT   ".cdl"

void cd_base_free(cd_base* base) {
    free((void*)base->base_name);
    if (base->slinks_name) free((void*)base->slinks_name);
    if (base->images_name) free((void*)base->images_name);
    free(base);
}

cd_base* cd_base_open(const char* path) {
    cd_base* base = (cd_base*)malloc(sizeof(cd_base));
    const char* name = strrchr(path, '/');
    if (!name) name = path;
    if ((strlen(name) < 4) || (strncmp(&name[strlen(name)-4], CD_BASE_EXT, 4))) {
        base->base_name = (char*)malloc(strlen(path) + 5);
        strcpy((char*)base->base_name, path);
        strcat((char*)base->base_name, CD_BASE_EXT);
    } else {
        base->base_name = strdup(path);
    }
    base->slinks_name = NULL;
    base->images_name = NULL;
    base->base_fd = open(base->base_name, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (base->base_fd != -1) {
        base->slinks_fd = -1;
        base->images_fd = -1;
        // We can rewrite base_name only now - when file is opened
        if (*base->base_name != '/') {
            name = base->base_name;
            base->base_name = realpath(name, NULL);
            free((void*)name);
        }
        base->slinks_name = (char*)malloc(strlen(base->base_name) + 1);
        strncpy((char*)base->slinks_name, base->base_name, strlen(base->base_name) - 4);
        ((char*)base->slinks_name)[strlen(base->base_name)-4] = '\0';
        strcat((char*)base->slinks_name, CD_SLINKS_EXT);
        base->images_name = (char*)malloc(strlen(base->base_name) + 1);
        strncpy((char*)base->images_name, base->base_name, strlen(base->base_name) - 4);
        ((char*)base->images_name)[strlen(base->base_name)-4] = '\0';
        strcat((char*)base->images_name, CD_PICTURE_EXT);
        return base;
    } else {
        cd_base_free(base);
        return NULL;
    }
}

void cd_base_close(cd_base* base) {
    if (base->images_fd != -1) close(base->images_fd);
    if (base->slinks_fd != -1) close(base->slinks_fd);
    close(base->base_fd);
    cd_base_free(base);
}
