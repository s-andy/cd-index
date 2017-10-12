/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/iso_fs.h>
#include <time.h>

#include "cdindex.h"
#include "index.h"
#include "plugin.h"
#include "extract.h"

#define false   0
#define true    1

void cd_fix_prev(cd_file_entry* parent, cd_file_entry* next, cd_base* base) {
    if (parent->child != next->id) {
        off_t offset;
        cd_offset index;
        cd_file_entry entry;
        for (index = parent->child; index;) {
            offset = sizeof(cd_iso_header) + (index - 1) * (sizeof(cd_file_entry) - sizeof(cd_offset));
            if (lseek(base->base_fd, offset, SEEK_SET) == offset) {
                read(base->base_fd, (void*)&entry + sizeof(cd_offset), sizeof(cd_file_entry) - sizeof(cd_offset));
                index = entry.next;
            } else return;
        }
        entry.next = next->id;
        lseek(base->base_fd, offset, SEEK_SET);
        write(base->base_fd, (void*)&entry + sizeof(cd_offset), sizeof(cd_file_entry) - sizeof(cd_offset));
    }
}

const char* cd_copy_filename(const char* path, char* name) {
    int from, to;
    char* end = strrchr(path, '/');
    if (end) {
        if (*(end+1)) {
            from = end - path + 1;
            to = strlen(path);
        } else {
            to = strlen(path) - 1;
            for (from = to - 1; from >= 0; from--) {
                if (path[from] == '/') {
                    from++;
                    break;
                }
            }
            if (from < 0) from = 0;
        }
    } else {
        from = 0;
        to = strlen(path);
    }
    memcpy(name, &path[from], to - from);
    name[to-from] = '\0';
    return name;
}

cd_file_entry* cd_create_entry(const char* name, struct stat64* stat, cd_file_entry* entry, cd_file_entry* parent, cd_offset* offset) {
    if (!entry) entry = (cd_file_entry*)malloc(sizeof(cd_file_entry));

    entry->id = (*offset)++;

    if (S_ISREG(stat->st_mode)) entry->type = CD_REG;
    else if (S_ISLNK(stat->st_mode)) entry->type = CD_LNK;
    else entry->type = CD_DIR;

    memset(entry->name, '\0', CD_NAME_MAX);
    strncpy(entry->name, name, CD_NAME_MAX);

    entry->mode  = (unsigned short)stat->st_mode;
    entry->mtime = stat->st_mtime;
    entry->uid   = stat->st_uid;
    entry->gid   = stat->st_gid;
    entry->size  = stat->st_size;

    entry->info = 0;

    entry->parent = (parent) ? parent->id : 0;
    if (parent && (parent->child == 0)) parent->child = entry->id;
    entry->child = 0;
    entry->next = 0;

    return entry;
}

void cd_save_entry(cd_file_entry* entry, cd_base* base) {
    off_t offset = sizeof(cd_iso_header) + (entry->id - 1) * (sizeof(cd_file_entry) - sizeof(cd_offset));
    if (lseek(base->base_fd, offset, SEEK_SET) == offset) {
        DEBUG_OUTPUT(DEBUG_BASEIO, "saving record #%u\n", entry->id);
        write(base->base_fd, (void*)entry + sizeof(cd_offset), sizeof(cd_file_entry) - sizeof(cd_offset));
    } else {
        printf("[error] seek failed: \"%s\" (%u)\n", entry->name, entry->id);
    }
}

void cd_update_entry(struct stat64* stat, cd_file_entry* entry, cd_base* base) {
    entry->mode  = (unsigned short)stat->st_mode;
    entry->mtime = stat->st_mtime;
    entry->uid   = stat->st_uid;
    entry->gid   = stat->st_gid;
    entry->size  = stat->st_size;

    cd_save_entry(entry, base);
}

cd_file_entry* cd_autocreate_path(const char* path, cd_file_entry* entry, cd_file_entry* parent, cd_offset* offset) {
    char* end = strchr(path, '/');
    if (end && *(end+1)) {
        if (end == path) {
            printf("[fixme] path starts with '/': %s\n", path);
            return NULL;
        }
        char name[CD_NAME_MAX];
        memcpy(name, path, end - path);
        name[end-path] = '\0';
        end = strchr(end + 1, '/');
        if (!end || !*(end+1)) {
            entry->id = (*offset)++;
            entry->type = CD_DIR;
            memset(entry->name, '\0', CD_NAME_MAX);
            strncpy(entry->name, name, CD_NAME_MAX);
            entry->mode  = (unsigned short)S_IFDIR|S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
            entry->mtime = 0;
            entry->uid   = 0;
            entry->gid   = 0;
            entry->size  = 0;
            entry->info  = 0;
            entry->parent = parent->id;
            if (parent->child == 0) parent->child = entry->id;
            entry->child = 0;
            entry->next  = 0;
            printf("[warning] automatically created directory %s\n", name);
            return entry;
        } else {
            printf("[fixme] autocreate of multiple dirs is not supported\n");
        }
    }
    return NULL;
}

cd_file_entry* cd_find_parent(const char* path, cd_file_entry* parent, cd_offset* eoff, cd_base* base) {
    char* next = strchr(path, '/');
    if (!next || !*(next+1)) return parent;
    off_t offset;
    const char* dir;
    cd_offset index = parent->id + 1;
    cd_file_entry* entry = (cd_file_entry*)malloc(sizeof(cd_file_entry));
    for (dir = path; next && *(next+1);) {
        if (index) {
            for (;;) {
                offset = sizeof(cd_iso_header) + (index - 1) * (sizeof(cd_file_entry) - sizeof(cd_offset));
                if (lseek(base->base_fd, offset, SEEK_SET) == offset) {
                    if (read(base->base_fd, (void*)entry + sizeof(cd_offset), sizeof(cd_file_entry) - sizeof(cd_offset)) > 0) {
                        entry->id = index;
                        if (!strncmp(entry->name, dir, next - dir) && !entry->name[next-dir]) {
                            index = entry->child;
                            break;
                        } else if (entry->next) {
                            index = entry->next;
                            continue;
                        }
                    }
                    if (cd_autocreate_path(dir, entry, parent, eoff)) {
                        cd_fix_prev(parent, entry, base);
                        return entry;
                    } else {
                        free(entry);
                        return NULL;
                    }
                } else {
                    free(entry);
                    return NULL;
                }
            }
        } else {
            free(entry);
            return NULL;
        }
        dir = next + 1;
        next = strchr(dir, '/');
    }
    return entry;
}

cd_offset cd_find_entry(cd_file_entry* parent, const char* name, cd_file_entry* entry, cd_base* base) {
    off_t offset;
    cd_offset index;
    for (index = parent->child; index;) {
        offset = sizeof(cd_iso_header) + (index - 1) * (sizeof(cd_file_entry) - sizeof(cd_offset));
        if ((lseek(base->base_fd, offset, SEEK_SET) == offset) &&
            (read(base->base_fd, (void*)entry + sizeof(cd_offset), sizeof(cd_file_entry) - sizeof(cd_offset)) > 0)) {
            if (!strncmp(entry->name, name, CD_NAME_MAX)) return entry->id = index;
            else index = entry->next;
        } else break;
    }
    return 0;
}

off_t cd_add_symlink(const char* path, unsigned long size, cd_base* base) {
    if (base->slinks_fd == -1) {
        base->slinks_fd = open(base->slinks_name, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        if (base->slinks_fd == -1) return 0;
        cd_index_mark mark;
        memcpy(&mark.mark, CD_LINKS_MARK, CD_INDEX_MARK_LEN);
        mark.version = CD_LINKS_VERSION;
        write(base->slinks_fd, &mark, sizeof(cd_index_mark));
    }
    off_t offset = lseek(base->slinks_fd, 0, SEEK_END);
    write(base->slinks_fd, path, size);
    return offset;
}

void cd_index(const char* path, cd_file_entry* parent, cd_offset* offset, cd_base* base) {
    DIR* dir = opendir(path);
    if (dir) {
        chdir(path);
        struct stat64 stat;
        struct dirent* file;
        cd_file_entry* prev = NULL;
        while ((file = readdir(dir))) {
            if (!strcmp(file->d_name, ".")) continue;
            if (!strcmp(file->d_name, "..")) continue;
            if (lstat64(file->d_name, &stat) != -1) {
                if (!S_ISDIR(stat.st_mode) &&
                    !S_ISREG(stat.st_mode) &&
                    !S_ISLNK(stat.st_mode)) {
                    DEBUG_OUTPUT(DEBUG_INDEX, "skipping \"%s\" (type:%03d)\n", file->d_name, file->d_type);
                    continue;
                }
                cd_file_entry* entry = cd_create_entry(file->d_name, &stat, NULL, parent, offset);
                if (entry->type == CD_DIR) {
                    printf("[dir] indexing \"%s\"...\n", file->d_name);
                    cd_index(file->d_name, entry, offset, base);
                } else if (entry->type == CD_LNK) {
                    char* linkpath = (char*)malloc(entry->size);
                    if (readlink(file->d_name, linkpath, entry->size) != -1) {
                        entry->info = cd_add_symlink(linkpath, entry->size, base);
                        free(linkpath);
                    }
                } else if (entry->type == CD_REG) {

                    // Check for extractors
                    cd_extractor_info* extractor = cd_find_extractor(file->d_name);
                    if (extractor) {
                        printf("[extractor] extracting \"%s\" using \"%s\"...\n", file->d_name, extractor->name);
                        entry->info = extractor->getdata(file->d_name, entry, extractor->__udata);
                    }

                    // Check for plugins
                    cd_plugin_info* plugin = cd_find_plugin(file->d_name);
                    if (plugin) {
                        void* arc = (plugin->init) ? plugin->init() : NULL;
                        if (!plugin->init || arc) {
                            void* handle;
                            if ((handle = plugin->open(arc, file->d_name))) {
                                int psave;
                                const char* path;
                                int errors = false;
                                const char* symlink;
                                cd_file_entry* upper;
                                cd_file_entry archive;
                                char name[CD_NAME_MAX];
                                printf("[plugin] indexing \"%s\" using %s...\n", file->d_name, plugin->name);
                                while (plugin->read(handle, &path, &symlink, &stat) != -1) {
                                    psave = false;
                                    upper = cd_find_parent(path, entry, offset, base);
                                    if (upper) {
                                        if ((upper != entry) && (upper->child == 0)) psave = true;
                                        cd_copy_filename(path, name);
                                        if (S_ISDIR(stat.st_mode) && cd_find_entry(upper, name, &archive, base)) {
                                            // We created this dir automatically before, now update it
                                            cd_update_entry(&stat, &archive, base);
                                        } else {
                                            cd_create_entry(name, &stat, &archive, upper, offset);
                                            if ((archive.type == CD_LNK) && symlink) {
                                                archive.size = strlen(symlink);
                                                if (archive.size) archive.info = cd_add_symlink(symlink, archive.size, base);
                                            }
                                            cd_save_entry(&archive, base);
                                            if (psave) cd_save_entry(upper, base);
                                            cd_fix_prev(upper, &archive, base);
                                        }
                                        if (upper != entry) free(upper);
                                    } else {
                                        errors = true;
                                        printf("[error] parent not found: \"%s\"\n", path);
                                    }
                                }
                                if (errors) printf("[warning] indexed with errors: \"%s\"\n", file->d_name);
                                plugin->close(handle);
                                entry->type = CD_ARC;
                            } else {
                                if (!plugin->ignore_errors) printf("[error] could not open archive: \"%s\"\n", file->d_name);
                            }
                            if (plugin->finish) plugin->finish(arc);
                        } else {
                            printf("[error] init failed: %s\n", plugin->name);
                        }
                    }

                }
                if (prev) {
                    prev->next = entry->id;
                    cd_save_entry(prev, base);
                    free(prev);
                }
                prev = entry;
            } else {
                printf("[error] stat failed: \"%s\"\n", file->d_name);
            }
        }
        if (prev) {
            cd_save_entry(prev, base);
            free(prev);
        }
        chdir("..");
        closedir(dir);
    } else {
        printf("[error] opendir failed: \"%s\"\n", path);
    }
}

void cd_fix_string(char* string, int length) {
    register int i;
    for (i = length - 1; i >= 0; i--) {
        if (string[i] == 0x20) string[i] = 0x00;
        else if (string[i]) break;
    }
}

void cd_header(const char* device, cd_base* base) {
    int fd = open(device, O_RDONLY|O_LARGEFILE);
    if (fd != -1) {
        struct tm tm;
        cd_iso_header header;
        lseek(fd, 0x8000, SEEK_SET);
        memset(&tm, '\0', sizeof(struct tm));
        memset(&header, '\0', sizeof(cd_iso_header));
        memcpy(&header.mark.mark, CD_INDEX_MARK, CD_INDEX_MARK_LEN);
        header.mark.version = CD_INDEX_VERSION;
        char buf[sizeof(struct iso_volume_descriptor)];
        struct iso_volume_descriptor* vd = (struct iso_volume_descriptor*)buf;
        struct iso_supplementary_descriptor* pd = (struct iso_supplementary_descriptor*)buf;
        for (;;) {
            if (read(fd, &buf, sizeof(struct iso_volume_descriptor)) > 0) {
                if ((unsigned char)vd->type[0] == ISO_VD_END) break;
                else if ((unsigned char)vd->type[0] == ISO_VD_BOOT) {
                    if (pd->flags[0] == 'E') header.bootable = true;
                } else if ((unsigned char)vd->type[0] == ISO_VD_PRIMARY) {
                    strncpy(header.volume_id, pd->volume_id, 32);
                    cd_fix_string(header.volume_id, 32);
                    header.size = (cd_size)(*(unsigned*)pd->volume_space_size) * 2048;
                    strncpy(header.publisher, pd->publisher_id, 128);
                    cd_fix_string(header.publisher, 128);
                    strncpy(header.preparer, pd->preparer_id, 128);
                    cd_fix_string(header.preparer, 128);
                    strncpy(header.generator, pd->application_id, 128);
                    cd_fix_string(header.generator, 128);
                    if (sscanf(pd->creation_date, "%04d%02d%02d%02d%02d%02d",
                        &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                        &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6) {
                        tm.tm_year -= 1900;
                        tm.tm_mon--;
                        header.ctime = mktime(&tm);
                    }
                    if (sscanf(pd->modification_date, "%04d%02d%02d%02d%02d%02d",
                        &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                        &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6) {
                        tm.tm_year -= 1900;
                        tm.tm_mon--;
                        header.mtime = mktime(&tm);
                    }
                }
            } else break;
        }
        write(base->base_fd, &header, sizeof(cd_iso_header));
        close(fd);
    } else {
        printf("[error] failed to open: \"%s\"\n", device);
    }
}
