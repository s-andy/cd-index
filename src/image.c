/*
 * Copyright (C) 2017 Andriy Lesyuk; All rights reserved.
 */

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "extract.h"
#include "image.h"

// TODO graphicsmagick + libexrator?

int cd_get_thumbnail_size(int* width, int* height) {
    if (*width > *height) {
        if (*width > CD_THUMBNAIL_SIZE) {
            *height = (int)((*height) * CD_THUMBNAIL_SIZE / (*width));
            *width = CD_THUMBNAIL_SIZE;
            return 1;
        }
    } else {
        if (*height > CD_THUMBNAIL_SIZE) {
            *width = (int)((*width) * CD_THUMBNAIL_SIZE / (*height));
            *height = CD_THUMBNAIL_SIZE;
            return 1;
        }
    }
    return 0;
}

int cd_create_data_dir(const char* dir) {
    int error = mkdir(dir, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
    if (error) {
        if (errno == EEXIST) {
            struct stat dstat;
            error = stat(dir, &dstat);
            if (error || !S_ISDIR(dstat.st_mode)) {
                printf("[warning] unable to create data directory %s\n", dir);
                return 1;
            }
        } else {
            printf("[warning] failed to create data directory %s\n", dir);
            return 1;
        }
    }
    return 0;
}

static cd_extractor_info cd_image = {
    "image",
    "\\.(bmp|gif|ico|jpe?g|png|psd|svg|tiff?|xcf)$",
    NULL /* cd_image_init */,
    NULL /* cd_image_getdata */,
    NULL /* cd_image_finish */,
    NULL,
    NULL,
    NULL
};
