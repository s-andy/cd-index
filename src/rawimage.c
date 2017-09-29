/*
 * Copyright (C) 2017 Andriy Lesyuk; All rights reserved.
 */

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libraw/libraw.h>
#include <wand/MagickWand.h>

#include "extract.h"
#include "image.h"

typedef struct {
    libraw_data_t* rdata;
    MagickWand* wand;
    const char* dir;
    const char* path;
    int fd;
    int skip_thumbs;
} cd_rawimage_base;

float cd_get_coordinate(float coord[3], char ref) {
    float result = coord[0] + coord[1] / 60 + coord[2] / 3600;
    if ((ref == 'S') || (ref == 'W')) {
        result *= -1;
    }
    return result;
}

int cd_thumbnail_init(cd_rawimage_base* rbase) {
    if (!rbase->wand) {
        MagickWandGenesis();
        rbase->wand = NewMagickWand();
        int error = mkdir(rbase->dir, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
        if (error) {
            if (errno == EEXIST) {
                struct stat dstat;
                error = stat(rbase->dir, &dstat);
                if (error || !S_ISDIR(dstat.st_mode)) {
                    rbase->skip_thumbs = 1;
                    printf("[warning] unable to create directory %s\n", rbase->dir);
                }
            } else {
                rbase->skip_thumbs = 1;
                printf("[warning] failed to create directory %s\n", rbase->dir);
            }
        }
    }
    return !rbase->skip_thumbs;
}

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

void* cd_rawimage_init(cd_base* base) {
    cd_rawimage_base* rbase = (cd_rawimage_base*)malloc(sizeof(cd_rawimage_base));
    rbase->rdata = libraw_init(0);
    rbase->wand = NULL;
    size_t baselen = strlen(base->base_name);
    rbase->dir = (char*)malloc(baselen - 3);
    strncpy((char*)rbase->dir, base->base_name, baselen - 4);
    ((char*)rbase->dir)[baselen-4] = '\0';
    rbase->path = (char*)malloc(baselen + 1);
    strncpy((char*)rbase->path, base->base_name, baselen - 4);
    ((char*)rbase->path)[baselen-4] = '\0';
    strcat((char*)rbase->path, CD_PICTURE_EXT);
    rbase->fd = -1;
    rbase->skip_thumbs = 0;
    return rbase;
}

cd_offset cd_rawimage_getdata(const char* file, cd_file_entry* cdentry, void* udata) {
    cd_rawimage_base* rbase = (cd_rawimage_base*)udata;
    if (rbase->fd == -1) { // FIXME move this to image.c
        rbase->fd = open(rbase->path, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        if (rbase->fd == -1) return 0;
        cd_picture_mark mark;
        memcpy(&mark.mark, CD_PICTURE_MARK, CD_PICTURE_MARK_LEN);
        mark.version = CD_PICTURE_VERSION;
        write(rbase->fd, &mark, sizeof(cd_picture_mark));
    }
    int error = libraw_open_file(rbase->rdata, file);
    if (error == 0) {
        libraw_adjust_sizes_info_only(rbase->rdata);
        cd_picture_entry entry;
        memset(&entry, 0x00, sizeof(cd_picture_entry));
        entry.offset = cdentry->id;
        if ((rbase->rdata->sizes.flip == 5) || (rbase->rdata->sizes.flip == 6)) {
            entry.width = rbase->rdata->sizes.height;
            entry.height = rbase->rdata->sizes.width;
        } else {
            entry.width = rbase->rdata->sizes.width;
            entry.height = rbase->rdata->sizes.height;
        }
        strncpy(entry.creator, rbase->rdata->idata.model, 64);
        strncpy(entry.author, rbase->rdata->other.artist, 64);
        entry.ctime = rbase->rdata->other.timestamp;
        if (rbase->rdata->other.parsed_gps.gpsparsed) {
            entry.latitude = cd_get_coordinate(rbase->rdata->other.parsed_gps.latitude, rbase->rdata->other.parsed_gps.latref);
            entry.longtitude = cd_get_coordinate(rbase->rdata->other.parsed_gps.longtitude, rbase->rdata->other.parsed_gps.longref);
        }
        off_t offset = lseek(rbase->fd, 0, SEEK_END);
        write(rbase->fd, &entry, sizeof(cd_picture_entry));
#ifdef INCLUDE_THUMBNAILS
        if (!rbase->skip_thumbs) {
            // TODO: Try libextract
            error = libraw_unpack_thumb(rbase->rdata);
            if (error == 0) {
                libraw_processed_image_t* thumb = libraw_dcraw_make_mem_thumb(rbase->rdata, &error);
                if (thumb) {
                    if (cd_thumbnail_init(rbase)) {
                        MagickReadImageBlob(rbase->wand, thumb->data, thumb->data_size);
                        int twidth = rbase->rdata->thumbnail.twidth;
                        int theight = rbase->rdata->thumbnail.theight;
                        if (cd_get_thumbnail_size(&twidth, &theight)) {
                            MagickResizeImage(rbase->wand, twidth, theight, LanczosFilter, 1);
                        }
                        char* tpath = (char*)malloc(strlen(rbase->dir) + 16);
                        sprintf(tpath, "%s/%u.jpg", rbase->dir, cdentry->id);
                        printf("[rawimage] writing thumbnail to %s\n", tpath);
                        MagickWriteImage(rbase->wand, tpath);
                        free(tpath);
                    }
                    libraw_dcraw_clear_mem(thumb);
                } else {
                    printf("[warning] failed to extract thumbnail %s\n", file);
                }
            } else {
                printf("[warning] failed to unpack %s\n", file);
            }
        }
#endif
        return offset;
    } else {
        printf("[warning] failed to open %s\n", file);
    }
    return 0;
}

void cd_rawimage_finish(void* udata) {
    libraw_close(((cd_rawimage_base*)udata)->rdata);
    if (((cd_rawimage_base*)udata)->wand) {
        DestroyMagickWand(((cd_rawimage_base*)udata)->wand);
        MagickWandTerminus();
    }
    if (((cd_rawimage_base*)udata)->fd != -1) close(((cd_rawimage_base*)udata)->fd);
    free((void*)((cd_rawimage_base*)udata)->dir);
    free((void*)((cd_rawimage_base*)udata)->path);
    free(udata);
}

static cd_extractor_info cd_rawimage = {
    "rawimage",
    "\\.(nef|crw|cr2)$",
    cd_rawimage_init,
    cd_rawimage_getdata,
    cd_rawimage_finish,
    NULL,
    NULL,
    NULL
};

void cd_extractor_load_rawimage(cd_base* base) {
    cd_add_extractor(&cd_rawimage, base);
}
