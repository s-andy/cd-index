/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#ifndef _CD_IMAGE_H_
#define _CD_IMAGE_H_

#include "data.h"

#define CD_THUMBNAIL_SIZE           160
#define CD_THUMBNAIL_JPEG_QUALITY   76

#define CD_PICTURE_MARK     "CDP"
#define CD_PICTURE_MARK_LEN 3
#define CD_PICTURE_VERSION  0x01

extern int wand_count;

typedef struct {
    char mark[3];           // "CDP"
    cd_byte version;        // 0x01
} packed(cd_picture_mark);

typedef struct {
    cd_offset offset;       // ID of entry
    cd_word width;          // Width
    cd_word height;         // Height
    char creator[64];       // Software/camera
    char author[64];        // Author
    cd_time ctime;          // Created
    float latitude;         // GPS latitude
    float longtitude;       // GPS longitude
} packed(cd_picture_entry);

int cd_get_thumbnail_size(int* width, int* height);

int cd_create_data_dir(const char* dir);

int cd_get_image_fd(cd_base* base);

#endif /* _CD_IMAGE_H_ */
