/*
 * Copyright (C) 2017 Andriy Lesyuk; All rights reserved.
 */

#include <unistd.h>
#include <fcntl.h>
#include <libraw/libraw.h>
#include <wand/MagickWand.h>

#include "extract.h"
#include "image.h"

typedef struct {
    cd_base* base;
    libraw_data_t* rdata;
    MagickWand* wand;
    const char* dir;
    int skip_thumbs;
} cd_rawimage_base;

float cd_rawimage_get_coordinate(float coord[3], char ref) {
    float result = coord[0] + coord[1] / 60 + coord[2] / 3600;
    if ((ref == 'S') || (ref == 'W')) {
        result *= -1;
    }
    return result;
}

int cd_rawimage_thumbnail_init(cd_rawimage_base* rbase) {
    if (!rbase->wand) {
        if (!IsMagickWandInstantiated()) MagickWandGenesis();
        rbase->wand = NewMagickWand();
        wand_count++;
        rbase->skip_thumbs = cd_create_data_dir(rbase->dir);
    }
    return !rbase->skip_thumbs;
}

void* cd_rawimage_init(cd_base* base) {
    cd_rawimage_base* rbase = (cd_rawimage_base*)malloc(sizeof(cd_rawimage_base));
    rbase->base = base;
    rbase->rdata = libraw_init(0);
    rbase->wand = NULL;
    size_t baselen = strlen(base->base_name);
    rbase->dir = (char*)malloc(baselen - 3);
    strncpy((char*)rbase->dir, base->base_name, baselen - 4);
    ((char*)rbase->dir)[baselen-4] = '\0';
    rbase->skip_thumbs = 0;
    return rbase;
}

cd_offset cd_rawimage_getdata(const char* file, cd_file_entry* cdentry, void* udata) {
    cd_rawimage_base* rbase = (cd_rawimage_base*)udata;
    int images_fd = cd_get_image_fd(((cd_rawimage_base*)udata)->base);
    if (images_fd == -1) return 0;
    if (libraw_open_file(rbase->rdata, file) == 0) {
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
            entry.latitude = cd_rawimage_get_coordinate(rbase->rdata->other.parsed_gps.latitude, rbase->rdata->other.parsed_gps.latref);
            entry.longitude = cd_rawimage_get_coordinate(rbase->rdata->other.parsed_gps.longtitude, rbase->rdata->other.parsed_gps.longref);
        }
        off_t offset = lseek(images_fd, 0, SEEK_END);
        write(images_fd, &entry, sizeof(cd_picture_entry));
#ifdef INCLUDE_THUMBNAILS
        if (!rbase->skip_thumbs) {
            /*
             * FIXME: Raw images usually include thumbnails of the needed size, but there is no lib to read them from there.
             *        P.S. libexiv2 can do this, but it's for C++.
             */
            if (libraw_unpack_thumb(rbase->rdata) == 0) {
                libraw_processed_image_t* thumb = libraw_dcraw_make_mem_thumb(rbase->rdata, NULL);
                if (thumb) {
                    if (cd_rawimage_thumbnail_init(rbase)) {
                        MagickReadImageBlob(rbase->wand, thumb->data, thumb->data_size);
                        int twidth = rbase->rdata->thumbnail.twidth;
                        int theight = rbase->rdata->thumbnail.theight;
                        if (cd_get_thumbnail_size(&twidth, &theight)) {
                            MagickResizeImage(rbase->wand, twidth, theight, LanczosFilter, 1);
                        }
                        MagickAutoOrientImage(rbase->wand);
                        MagickStripImage(rbase->wand);
                        MagickSetImageCompressionQuality(rbase->wand, CD_THUMBNAIL_JPEG_QUALITY);
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
#endif /* INCLUDE_THUMBNAILS */
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
        wand_count--;
        if (IsMagickWandInstantiated() && (wand_count <= 0)) MagickWandTerminus();
    }
    free((void*)((cd_rawimage_base*)udata)->dir);
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
