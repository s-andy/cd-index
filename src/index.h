/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#ifndef _CD_INDEX_H_
#define _CD_INDEX_H_

#include "data.h"
#include "base.h"

void cd_index(const char* path, cd_file_entry* parent, cd_offset* offset, cd_base* base);

void cd_header(const char* device, cd_base* base);

void cd_fix_string(char* string, int length);

#endif /* _CD_INDEX_H_ */
