/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grp.h>
#include <time.h>
#include <pwd.h>
#include <errno.h>

#include "search.h"
#include "find.h"

#define CD_BASE_EXT     ".cdi"

#define CD_FMT_BUFSIZE  256

#define OK      1
#define FAIL    0

#define true    1
#define false   0

#define NONE    0
#define SLASH   1
#define FORMAT  2

typedef struct {
    const char* name;
    const char* filename;
} cd_find_file;

typedef struct __cd_find_path cd_find_path;
struct __cd_find_path {
    cd_file_entry* entry;
    cd_find_path* prev;
};

int cd_sort_file(const void* f1, const void* f2) {
    return strcasecmp((*(cd_find_file**)f1)->name, (*(cd_find_file**)f2)->name);
}

cd_offset cd_find_offset(int fd, const char* path) {
    if (path) {
        size_t length;
        const char* slash;
        cd_file_entry entry;
        const char* file = path;
        cd_offset offset = sizeof(cd_iso_header);
        while (file) {
            slash = strchr(file, '/');
            length = (slash) ? slash - file : strlen(file);
            for (;;) {
                lseek(fd, offset, SEEK_SET);
                read(fd, (void*)&entry + sizeof(cd_offset), sizeof(cd_file_entry) - sizeof(cd_offset));
                if (!strncmp(file, entry.name, length) && !entry.name[length]) {
                    if ((entry.type == CD_DIR) && entry.child) {
                        offset = sizeof(cd_iso_header) + (entry.child - 1) * (sizeof(cd_file_entry) - sizeof(cd_offset));
                        break;
                    } else {
                        return 0;
                    }
                } else if (entry.next) {
                    offset = sizeof(cd_iso_header) + (entry.next - 1) * (sizeof(cd_file_entry) - sizeof(cd_offset));
                } else {
                    return 0;
                }
            }
            file = (slash && *(slash+1)) ? slash + 1 : NULL;
        }
        return offset;
    } else {
        return sizeof(cd_iso_header);
    }
}

int cd_find_match(cd_file_entry* entry, cd_find_exp* exps) {
    cd_find_exp* exp;
    int matches = true;
    for (exp = exps; exp; exp = exp->next) {
        if ((exp->flags & FIND_MASK) == FIND_WILDCARD) {
            int flags = FNM_PATHNAME;
            if ((exp->flags & FIND_FLAGS) == FIND_ICASE) flags |= FNM_CASEFOLD;
            if (fnmatch(exp->wildcard, entry->name, flags)) return false;
        } else if ((exp->flags & FIND_MASK) == FIND_REGEXP) {
            if (regexec(exp->regex, entry->name, 0, NULL, 0)) return false;
        } else if ((exp->flags & FIND_MASK) == FIND_TYPE) {
            if (entry->type != exp->type) return false;
        } else if ((exp->flags & FIND_MASK) == FIND_MTIME) {
            time_t mtime = entry->mtime;
            struct tm* tm = localtime(&mtime);
            tm->tm_sec = 0;
            tm->tm_min = 0;
            tm->tm_hour = 0;
            mtime = mktime(tm);
            if ((exp->flags & FIND_FLAGS) == FIND_LESS) {
                if (mtime < exp->time) return false;
            } else if ((exp->flags & FIND_FLAGS) == FIND_GREATER) {
                if (mtime > exp->time) return false;
            } else if (mtime != exp->time) return false;
        } else if ((exp->flags & FIND_MASK) == FIND_SIZE) {
            if ((exp->flags & FIND_FLAGS) == FIND_LESS) {
                if (entry->size > exp->size) return false;
            } else if ((exp->flags & FIND_FLAGS) == FIND_GREATER) {
                if (entry->size < exp->size) return false;
            } else if (entry->size != exp->size) return false;
        }
    }
    return matches;
}

int cd_get_path(char* buf, int buflen, const char* file, cd_find_path* path, const char* parent) {
    int len = 0;
    if (parent) {
        if (buf) strcpy(buf, parent);
        len += strlen(parent);
        if (parent[strlen(parent)-1] != '/') {
            if (buf) strcpy(&buf[len], "/");
            len++;
        }
    }
    int end = buflen;
    if (file) {
        len += strlen(file);
        if (buf) {
            end = buflen - strlen(file);
            strcpy(&buf[end], file);
        }
    }
    int flen;
    cd_find_path* element;
    for (element = path; element; element = element->prev) {
        flen = strlen(element->entry->name);
        if (buf) {
            buf[--end] = '/';
            end -= flen;
            strncpy(&buf[end], element->entry->name, flen);
        }
        len += flen + 1;
    }
    if (!file && buf) buf[buflen-1] = '\0';
    return len;
}

static inline int isoctal(int c) {
    if ((c >= 0x30) && (c <= 0x37)) return c;
    else return 0;
}

void cd_find_output(const char* fmt, cd_file_entry* entry, cd_find_path* path, const char* parent, cd_find_file* file) {
    if (fmt) {
        int i;
        int type = NONE;
        for (i = 0; i < strlen(fmt); i++) {
            if (type == SLASH) {
                if (fmt[i] == 'a') putchar(0x07);
                else if (fmt[i] == 'b') putchar(0x08);
                else if (fmt[i] == 'f') putchar(0x0C);
                else if (fmt[i] == 'n') putchar(0x0A);
                else if (fmt[i] == 'r') putchar(0x0D);
                else if (fmt[i] == 't') putchar(0x09);
                else if (fmt[i] == 'v') putchar(0x0B);
                else if (fmt[i] == '\\') putchar('\\');
                else if (isoctal(fmt[i]) && ((i + 2) < strlen(fmt)) &&
                    isoctal(fmt[i+1]) && isoctal(fmt[i+2])) {
                    putchar((fmt[i+2] - 0x30) | ((fmt[i+1] - 0x30) << 3) | ((fmt[i] - 0x30) << 6));
                    i += 2;
                } else {
                    putchar('\\');
                    putchar(fmt[i]);
                }
                type = NONE;
            } else if (type == FORMAT) {
                if (fmt[i] == '%') putchar('%');
                else if (fmt[i] == 'b') printf("%lu", (entry->size / 512));
                else if (fmt[i] == 'd') {
                    int depth = 1;
                    if (parent) {
                        const char* next;
                        for (next = parent; next;) {
                            depth++;
                            next = strchr(next, '/');
                            if (next) {
                                next++;
                                if (!*next) next = NULL;
                            }
                        }
                    }
                    cd_find_path* element;
                    for (element = path; element; element = element->prev) depth++;
                    printf("%d", depth);
                } else if (fmt[i] == 'f') printf(entry->name);
                else if (fmt[i] == 'g') {
                    struct group* grp = getgrgid(entry->gid);
                    if (grp) printf(grp->gr_name);
                    else printf("%d", entry->gid);
                } else if (fmt[i] == 'G') printf("%d", entry->gid);
                else if (fmt[i] == 'h') {
                    int plen = cd_get_path(NULL, 0, NULL, path, parent);
                    char pbuf[plen+1];
                    cd_get_path(pbuf, plen, NULL, path, parent);
                    printf(pbuf);
                } else if (fmt[i] == 'k') printf("%lu", (entry->size / 1024));
                else if (fmt[i] == 'l') {
                    if (entry->type == CD_LNK) {
                        char* lpath = strdup(file->filename);
                        lpath[strlen(file->filename)-1] = 'l';
                        int fd = open(lpath, O_RDONLY);
                        if (fd != -1) {
                            lseek(fd, entry->info, SEEK_SET);
                            char lbuf[entry->size+1];
                            read(fd, lbuf, entry->size);
                            lbuf[entry->size] = '\0';
                            printf(lbuf);
                            close(fd);
                        }
                        free(lpath);
                    }
                } else if (fmt[i] == 'm') printf("%04o", (entry->mode & 0777));
                else if (fmt[i] == 'M') {
                    printf("%c%c%c%c%c%c%c%c%c%c",
                    (entry->type == CD_DIR) ? 'd' : (entry->type == CD_LNK) ? 'l' : '-',
                    (entry->mode & S_IRUSR) ? 'r' : '-',
                    (entry->mode & S_IWUSR) ? 'w' : '-',
                    (entry->mode & S_IXUSR) ? 'x' : '-',
                    (entry->mode & S_IRGRP) ? 'r' : '-',
                    (entry->mode & S_IWGRP) ? 'w' : '-',
                    (entry->mode & S_IXGRP) ? 'x' : '-',
                    (entry->mode & S_IROTH) ? 'r' : '-',
                    (entry->mode & S_IWOTH) ? 'w' : '-',
                    (entry->mode & S_IXOTH) ? 'x' : '-');
                } else if (fmt[i] == 'p') {
                    int plen = cd_get_path(NULL, 0, entry->name, path, parent);
                    char pbuf[plen+1];
                    cd_get_path(pbuf, plen, entry->name, path, parent);
                    printf(pbuf);
                } else if (fmt[i] == 's') printf("%lu", entry->size);
                else if (fmt[i] == 't') {
                    time_t mtime = entry->mtime;
                    printf(ctime(&mtime));
                } else if (fmt[i] == 'T') {
                    i++;
                    if (fmt[i] == '@') printf("%u", entry->mtime);
                    else {
                        char buf[32];
                        char tfmt[] = "%X";
                        time_t mtime = entry->mtime;
                        tfmt[1] = fmt[i];
                        strftime(buf, 32, tfmt, localtime(&mtime));
                        printf(buf);
                    }
                } else if (fmt[i] == 'u') {
                    struct passwd* pwd = getpwuid(entry->uid);
                    if (pwd) printf(pwd->pw_name);
                    else printf("%d", entry->uid);
                } else if (fmt[i] == 'U') printf("%d", entry->uid);
                else if (fmt[i] == 'y') printf("%c", (entry->type == CD_DIR) ? 'd' : (entry->type == CD_LNK) ? 'l' : '-');
                else if (fmt[i] == 'L') printf(file->name);
                else putchar(fmt[i]);
                type = NONE;
            } else {
                if (fmt[i] == '\\') type = SLASH;
                else if (fmt[i] == '%') type = FORMAT;
                else putchar(fmt[i]);
            }
        }
    } else {
        int plen = cd_get_path(NULL, 0, entry->name, path, parent);
        char pbuf[plen+1];
        cd_get_path(pbuf, plen, entry->name, path, parent);
        printf("%s: %s\n", file->name, pbuf);
    }
}

void cd_find_in(int fd, cd_offset start, cd_find_path* path, cd_find_file* file, cd_find_req* req) {
    cd_file_entry entry;
    cd_offset offset = start;
    for (;;) {
        lseek(fd, offset, SEEK_SET);
        read(fd, (void*)&entry + sizeof(cd_offset), sizeof(cd_file_entry) - sizeof(cd_offset));
        if (cd_find_match(&entry, req->exp)) {
            cd_find_output(req->format, &entry, path, req->path, file);
        }
        if (entry.type == CD_DIR) {
            if (entry.child) {
                cd_find_path element;
                element.entry = &entry;
                element.prev = path;
                cd_find_in(fd, sizeof(cd_iso_header) + (entry.child - 1) * (sizeof(cd_file_entry) - sizeof(cd_offset)),
                    &element, file, req);
            }
        } else if (entry.type == CD_ARC) {
            if (!req->noarc && entry.child) {
                cd_find_path element;
                element.entry = &entry;
                element.prev = path;
                cd_find_in(fd, sizeof(cd_iso_header) + (entry.child - 1) * (sizeof(cd_file_entry) - sizeof(cd_offset)),
                    &element, file, req);
            }
        }
        if (entry.next) {
            offset = sizeof(cd_iso_header) + (entry.next - 1) * (sizeof(cd_file_entry) - sizeof(cd_offset));
        } else break;
    }
}

int cd_search(const char* dir, cd_find_req* req) {
    DIR* d = opendir(dir);
    if (d) {
        int flen = 0;
        struct dirent* f;
        cd_find_file** files = NULL;
        while ((f = readdir(d))) {
            if ((f->d_type == DT_REG) && (!strncasecmp(f->d_name + strlen(f->d_name) - 4, CD_BASE_EXT, 4))) {
                char* name = (char*)malloc(strlen(f->d_name) - 3);
                strncpy(name, f->d_name, strlen(f->d_name) - 4);
                name[strlen(f->d_name)-4] = '\0';
                if (req->cdimask && fnmatch(req->cdimask, name, FNM_PATHNAME)) {
                    free(name);
                    continue;
                } else {
                    cd_find_file* file = (cd_find_file*)malloc(sizeof(cd_find_file));
                    file->name = name;
                    file->filename = strdup(f->d_name);
                    if (files) files = (cd_find_file**)realloc(files, sizeof(cd_find_file*) * (flen + 1));
                    else files = (cd_find_file**)malloc(sizeof(cd_find_file*));
                    files[flen] = file;
                    flen++;
                }
            }
        }
        closedir(d);
        qsort(files, flen, sizeof(cd_find_file*), cd_sort_file);

        int i;
        if (strcmp(dir, "./")) chdir(dir);
        for (i = 0; i < flen; i++) {
            int fd = open(files[i]->filename, O_RDONLY);
            if (fd != -1) {
                cd_offset offset = cd_find_offset(fd, req->path);
                if (offset) {
                    cd_find_in(fd, offset, NULL, files[i], req);
                } // skip silently
                close(fd);
            } else {
                printf("cdfind: warning: could not open cd index `%s'\n", files[i]->filename);
            }
        }
    } else {
        printf("cdfind: %s: %s\n", dir, strerror(errno));
        return FAIL;
    }
    return OK;
}
