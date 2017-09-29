/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#include <stdlib.h>
#include <archive.h>
#include <archive_entry.h>
#include <string.h>

#include "plugin.h"
#include "cdindex.h"

void* cd_archive_init() {
    struct archive* arc =  archive_read_new();
    archive_read_support_filter_all(arc);
    archive_read_support_format_all(arc);
    return arc;
}

void* cd_archive_open(void* arc, const char* file) {
    if (archive_read_open_filename(arc, file, 2048) == 0) {
        return arc;
    }
    return NULL;
}

int cd_archive_read(void* arc, const char** name, const char** link, struct stat64* stat) {
    struct archive_entry* entry;
    if (archive_read_next_header(arc, &entry) == ARCHIVE_OK) {
        // Skip "." (first record for some archives)
        if (strcmp(archive_entry_pathname(entry), ".") ||
            (archive_read_next_header(arc, &entry) == ARCHIVE_OK)) {
            stat->st_mode = archive_entry_mode(entry);
            stat->st_mtime = archive_entry_mtime(entry);
            stat->st_uid = archive_entry_uid(entry);
            stat->st_gid = archive_entry_gid(entry);
            stat->st_size = archive_entry_size(entry);
            *name = archive_entry_pathname(entry);
            if (!strncmp(*name, "./", 2)) *name += 2;
            if (S_ISLNK(stat->st_mode)) {
                int format = archive_format(arc);
                if ((format & ARCHIVE_FORMAT_BASE_MASK) != ARCHIVE_FORMAT_ISO9660) {
                    *link = archive_entry_symlink(entry);
                } else {
                    *link = NULL;
                }
            }
            archive_read_data_skip(arc);
            return 0;
        }
    }
    return -1;
}

void cd_archive_close(void* arc) {
    archive_read_close(arc);
}

void cd_archive_finish(void* arc) {
    archive_read_free(arc);
}

static cd_plugin_info cd_archive = {
    "archiver",
    "\\.(iso|tar(\\.(gz|bz2|xz)){0,1}|tgz|cpio)$",
    FALSE,
    cd_archive_init,
    cd_archive_open,
    cd_archive_read,
    cd_archive_close,
    cd_archive_finish,
    NULL,
    NULL
};

void cd_plugin_load_archiver() {
    cd_add_plugin(&cd_archive);
}
