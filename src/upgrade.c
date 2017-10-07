/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <regex.h>

#include "data.h"

typedef struct {
    cd_index_mark mark;
    cd_bool bootable;
    char volume_id[32];
    uint32_t size;           // v2: cd_size change to uint64_t
    char publisher[128];
    char preparer[128];
    char generator[128];
    cd_time ctime;
    cd_time mtime;
} packed(cd_iso_header_v1); // was 433

typedef struct  {
    cd_offset id;
    cd_type type;
    char name[CD_NAME_MAX];
    cd_mode mode;
    cd_time mtime;
    cd_uid uid;
    cd_gid gid;
    uint32_t size;          // v2: cd_size change to uint64_t
    cd_offset info;
    cd_offset parent;
    cd_offset child;
    cd_offset next;
} packed(cd_file_entry_v1); // was 286 + 4 of id

cd_byte cd_get_index_version(const char* path) {
    cd_byte ver = 0x00;
    int fd = open(path, O_RDONLY);
    if (fd != -1) {
        cd_index_mark mark;
        if ((read(fd, (void*)&mark, sizeof(cd_index_mark)) == sizeof(cd_index_mark)) &&
            (memcmp(mark.mark, CD_INDEX_MARK, CD_INDEX_MARK_LEN) == 0)) {
            ver = mark.version;
        } else {
            printf("[error] %s is not cd index\n", path);
        }
        close(fd);
    } else {
        printf("[error] could not open %s\n", path);
    }
    return ver;
}

int cd_upgrade_v1_to_v2(const char* v1, const char* v2) {
    int ret = EXIT_SUCCESS;
    int fd1 = open(v1, O_RDONLY);
    if (fd1 != -1) {
        struct stat stat1, stat2;
        fstat(fd1, &stat1);
        int fd2 = open(v2, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        if (fd2 != -1) {
            cd_size datasize = 0;
            cd_iso_header header2;
            cd_file_entry entry2;
            cd_iso_header_v1 header1;
            cd_file_entry_v1 entry1;
            regex_t* iregex = (regex_t*)malloc(sizeof(regex_t));
            regex_t* riregex = (regex_t*)malloc(sizeof(regex_t));
            regex_t* vregex = (regex_t*)malloc(sizeof(regex_t));
            regcomp(iregex, "\\.(bmp|gif|ico|jpe?g|png|psd|svg|tiff?|xcf)$", REG_EXTENDED|REG_ICASE|REG_NOSUB);
            regcomp(riregex, "\\.(nef|crw|cr2)$", REG_EXTENDED|REG_ICASE|REG_NOSUB);
            regcomp(vregex, "\\.(mpe?g|vob|ogg|mov|mp4|mkv|avi|3gp|wmv)$", REG_EXTENDED|REG_ICASE|REG_NOSUB);
            char buf[CD_NAME_MAX+1];
            buf[CD_NAME_MAX] = '\0';
            int invsizes = 0, files = 0, images = 0, rimages = 0, videos = 0;
            ssize_t bytes = read(fd1, (void*)&header1, sizeof(cd_iso_header_v1));
            memcpy(&header2, &header1, (void*)&header1.size - (void*)&header1);
            header2.size = (cd_size)header1.size * 2048;
            memcpy(&header2.publisher, &header1.publisher, sizeof(cd_iso_header_v1) - ((void*)&header1.publisher - (void*)&header1));
            write(fd2, &header2, sizeof(cd_iso_header));
            while ((bytes < stat1.st_size) && ((stat1.st_size - bytes) >= (sizeof(cd_file_entry_v1) - sizeof(cd_offset)))) {
                bytes += read(fd1, (void*)&entry1 + sizeof(cd_offset), sizeof(cd_file_entry_v1) - sizeof(cd_offset));
                memcpy(&entry2, &entry1, (void*)&entry1.size - (void*)&entry1);
                entry2.size = entry1.size;
                memcpy(&entry2.info, &entry1.info, sizeof(cd_file_entry_v1) - ((void*)&entry1.info - (void*)&entry1));
                write(fd2, (void*)&entry2 + sizeof(cd_offset), sizeof(cd_file_entry) - sizeof(cd_offset));
                if (entry1.size == 0xffffffff) {
                    printf("[warning] invalid size of large file %.*s\n", CD_NAME_MAX, entry1.name);
                    invsizes++;
                }
                if (entry2.type == CD_REG) {
                    strncpy(buf, entry2.name, CD_NAME_MAX);
                    if (regexec(iregex, buf, 0, NULL, 0) == 0) images++;
                    if (regexec(riregex, buf, 0, NULL, 0) == 0) rimages++;
                    if (regexec(vregex, buf, 0, NULL, 0) == 0) videos++;
                    datasize += entry2.size;
                    files++;
                }
            }
            fstat(fd2, &stat2);
            regfree(iregex);
            regfree(riregex);
            regfree(vregex);
            free(iregex);
            free(riregex);
            free(vregex);
            close(fd2);
            if (bytes != stat1.st_size) printf("[warning] %ld bytes read and file size is %ld\n", bytes, stat1.st_size);
            printf("[info] file size: %ld => %ld, ISO size: %lu (%u), data size: ~%lu (files: %d)\n", stat1.st_size, stat2.st_size, header2.size, header1.size, datasize, files);
            printf("[info] invalid sizes: %d, images: %d (raw: +%d), videos: %d\n", invsizes, images, rimages, videos);
        } else {
            printf("[error] could not create %s\n", v2);
            ret = EXIT_FAILURE;
        }
        close(fd1);
    } else {
        ret = EXIT_FAILURE;
    }
    return ret;
}

int cd_upgrade(const char* path) {
    int ret = EXIT_SUCCESS;
    cd_byte cdiver = cd_get_index_version(path);
    if (cdiver == 0x00) {
        ret = EXIT_FAILURE;
    } else if (cdiver == 0x01) { // Upgrade to v2
        char* bck = strdup(path);
        bck[strlen(bck)-1] = '~';
        if (rename(path, bck) == 0) {
            ret = cd_upgrade_v1_to_v2(bck, path);
        } else {
            printf("[error] could not backup %s\n", path);
        }
        free(bck);
    } else if (cdiver == CD_INDEX_VERSION) {
        printf("[info] %s is up to date\n", path);
    } else {
        printf("[warning] cdupgrade tool is outdated\n");
    }
    return ret;
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        return cd_upgrade(argv[1]);
    }
    return EXIT_FAILURE;
}
