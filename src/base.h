/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#ifndef _CD_BASE_H_
#define _CD_BASE_H_

typedef struct {
    const char* base_name;
    const char* slinks_name;
    int base_fd;
    int slinks_fd;
} cd_base;

cd_base* cd_base_open(const char* path);

void cd_base_close(cd_base* base);

#endif /* _CD_BASE_H_ */
