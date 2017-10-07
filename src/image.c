/*
 * Copyright (C) 2017 Andriy Lesyuk; All rights reserved.
 */

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <wand/MagickWand.h>

#include "extract.h"
#include "image.h"

int wand_count = 0;

typedef struct {
    cd_base* base;
    MagickWand* wand;
    const char* dir;
    int skip_thumbs;
    int dir_created;
} cd_image_base;

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

int cd_get_image_fd(cd_base* base) {
    if (base->images_fd == -1) {
        base->images_fd = open(base->images_name, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        if (base->images_fd == -1) return 0;
        cd_picture_mark mark;
        memcpy(&mark.mark, CD_PICTURE_MARK, CD_PICTURE_MARK_LEN);
        mark.version = CD_PICTURE_VERSION;
        write(base->images_fd, &mark, sizeof(cd_picture_mark));
    }
    return base->images_fd;
}

MagickWand* cd_image_get_magick_wand(cd_image_base* ibase) {
    if (!ibase->wand) {
        if (!IsMagickWandInstantiated()) MagickWandGenesis();
        ibase->wand = NewMagickWand();
        wand_count++;
    }
    return ibase->wand;
}

int cd_image_thumbnail_init(cd_image_base* ibase) {
    if (!ibase->dir_created) {
        ibase->skip_thumbs = cd_create_data_dir(ibase->dir);
        ibase->dir_created = 1;
    }
    return !ibase->skip_thumbs;
}

void cd_image_dump_properties(MagickWand* wand) {
    size_t pcount;
    char** properties = MagickGetImageProperties(wand, "*", &pcount);
    for (int i = 0; i < pcount; i++) {
        char* value = MagickGetImageProperty(wand, properties[i]);
        printf(" *> %s: %s\n", properties[i], value);
        MagickRelinquishMemory(value);
    }
    printf("%lu properties\n", pcount);
}

cd_time cd_image_get_ctime(MagickWand* wand) {
    cd_time ctime = 0;
    char* date = MagickGetImageProperty(wand, "exif:DateTimeOriginal");
    if (!date) date = MagickGetImageProperty(wand, "exif:DateTime");
    if (date) {
        struct tm tm;
        memset(&tm, 0x00, sizeof(struct tm));
        if (strptime(date, "%Y:%m:%d %H:%M:%S", &tm)) {
            time_t result = mktime(&tm);
            if (result != (time_t)-1) {
               ctime = result; // Result not very accurate due to timezones
            }
        }
        MagickRelinquishMemory(date);
    }
    return ctime;
}

float cd_image_get_coordinate(const char* str, char ref) {
    int dnum, dden, mnum, mden, snum, sden;
    if (sscanf(str, "%d/%d, %d/%d, %d/%d", &dnum, &dden, &mnum, &mden, &snum, &sden) == 6) {
        float coord = dnum / (float)dden + mnum / (float)mden / 60 + snum / (float)sden / 3600;
        if ((ref == 'S') || (ref == 'W')) {
            coord *= -1;
        }
        return coord;
    }
    return 0;
}

int cd_image_get_coordinates(MagickWand* wand, float* lat, float* lon) {
    int ret = 0;
    char* latitude = MagickGetImageProperty(wand, "exif:GPSLatitude");
    char* latref = MagickGetImageProperty(wand, "exif:GPSLatitudeRef");
    char* longitude = MagickGetImageProperty(wand, "exif:GPSLongitude");
    char* longref = MagickGetImageProperty(wand, "exif:GPSLongitudeRef");
    if (latitude && latref && longitude && longref) {
        *lat = cd_image_get_coordinate(latitude, *latref);
        *lon = cd_image_get_coordinate(longitude, *longref);
        if (*lat && *lon) ret = 1;
    }
    if (latitude) MagickRelinquishMemory(latitude);
    if (latref) MagickRelinquishMemory(latref);
    if (longitude) MagickRelinquishMemory(longitude);
    if (longref) MagickRelinquishMemory(longref);
    return ret;
}

void* cd_image_init(cd_base* base) {
    cd_image_base* ibase = (cd_image_base*)malloc(sizeof(cd_image_base));
    ibase->base = base;
    ibase->wand = NULL;
    size_t baselen = strlen(base->base_name);
    ibase->dir = (char*)malloc(baselen - 3);
    strncpy((char*)ibase->dir, base->base_name, baselen - 4);
    ((char*)ibase->dir)[baselen-4] = '\0';
    ibase->skip_thumbs = 0;
    ibase->dir_created = 0;
    return ibase;
}

cd_offset cd_image_getdata(const char* file, cd_file_entry* cdentry, void* udata) {
    int images_fd = cd_get_image_fd(((cd_image_base*)udata)->base);
    if (images_fd == -1) return 0;
    MagickWand* wand = cd_image_get_magick_wand((cd_image_base*)udata);
    if (wand && (MagickReadImage(wand, file) != MagickFalse)) {
        cd_picture_entry entry;
        memset(&entry, 0x00, sizeof(cd_picture_entry));
        entry.offset = cdentry->id;
        int width = MagickGetImageWidth(wand);
	int height = MagickGetImageHeight(wand);
        switch (MagickGetImageOrientation(wand)) {
            case LeftTopOrientation:
            case RightTopOrientation:
            case RightBottomOrientation:
            case LeftBottomOrientation:
                entry.height = width;
                entry.width = height;
                break;
            default:
                entry.width = width;
                entry.height = height;
                break;
        }
        // cd_image_dump_properties(wand);
        char* property = MagickGetImageProperty(wand, "exif:Model");
        if (property) {
            strncpy(entry.creator, property, 64);
            MagickRelinquishMemory(property);
        }
        property = MagickGetImageProperty(wand, "exif:Artist");
        if (property) {
            strncpy(entry.author, property, 64);
            MagickRelinquishMemory(property);
        }
        entry.ctime = cd_image_get_ctime(wand);
        cd_image_get_coordinates(wand, &entry.latitude, &entry.longitude);
        off_t offset = lseek(images_fd, 0, SEEK_END);
        write(images_fd, &entry, sizeof(cd_picture_entry));
#ifdef INCLUDE_THUMBNAILS
        if (cd_image_thumbnail_init((cd_image_base*)udata)) {
            if (cd_get_thumbnail_size(&width, &height)) {
                MagickResizeImage(wand, width, height, LanczosFilter, 1);
            }
            MagickAutoOrientImage(wand);
            MagickStripImage(wand);
            MagickSetImageCompressionQuality(wand, CD_THUMBNAIL_JPEG_QUALITY);
            char* tpath = (char*)malloc(strlen(((cd_image_base*)udata)->dir) + 16);
            sprintf(tpath, "%s/%u.jpg", ((cd_image_base*)udata)->dir, cdentry->id);
            printf("[image] writing thumbnail to %s\n", tpath);
            MagickWriteImage(wand, tpath);
            free(tpath);
        }
#endif /* INCLUDE_THUMBNAILS */
        return offset;
    } else {
        printf("[warning] failed to open image %s\n", file);
    }
    return 0;
}

void cd_image_finish(void* udata) {
    if (((cd_image_base*)udata)->wand) {
        DestroyMagickWand(((cd_image_base*)udata)->wand);
        wand_count--;
        if (IsMagickWandInstantiated() && (wand_count <= 0)) MagickWandTerminus();
    }
    free((void*)((cd_image_base*)udata)->dir);
    free(udata);
}

static cd_extractor_info cd_image = {
    "image",
    "\\.(bmp|gif|ico|jpe?g|png|psd|svg|tiff?|xcf)$",
    cd_image_init,
    cd_image_getdata,
    cd_image_finish,
    NULL,
    NULL,
    NULL
};

void cd_extractor_load_image(cd_base* base) {
    cd_add_extractor(&cd_image, base);
}
