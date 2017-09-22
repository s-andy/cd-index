/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "plugin.h"
#include "cdindex.h"

#define CD_LINE_BUFFER      2048

#define CD_EXEC         01
#define CD_WRITE        02
#define CD_READ         04

/* Something like manual:
 * TRWXRWXRWX UID GID SIZE YYYY-MM-DD HH:MM PATH -> PATH
 */

typedef struct {
    const char* regex;
    const char* command;
    regex_t* __regex;
} cd_external_cmd;

static inline int isunsafe(int c) {
    if ((c == 0x20) ||  // SPACE
        (c == 0x21) ||  // !
        (c == 0x26) ||  // &
        (c == 0x27) ||  // '
        (c == 0x28) ||  // (
        (c == 0x29)) {  // )
        return c;
    }
    return 0;
}

size_t strsafelen(const char* s) {
    int i;
    int length = 0;
    for (i = 0; s[i]; i++) {
        if (isunsafe(s[i])) length += 2;
        else if (s[i] == 0x22) length += 4; // Double quotes
        else length++;
    }
    return length;
}

char* strsafecat(char* dst, const char* src) {
    int i, j;
    for (i = 0; dst[i]; i++);
    for (j = 0; src[j]; j++) {
        if (isunsafe(src[j])) {
            dst[i+j] = '\\';
            i++;
        } else if (src[j] == 0x22) {
            int k;
            for (k = 0; k < 3; k++) dst[i+j+k] = '\\';
            i += 3;
        }
        dst[i+j] = src[j];
    }
    dst[i+j] = '\0';
    return dst;
}

static cd_external_cmd cd_external_cmds[] = {
    { "\\.deb$", "/usr/share/cdindex/tools/deb", NULL },
    { "\\.rpm$", "/usr/share/cdindex/tools/rpm", NULL },
    { "\\.rar$", "/usr/share/cdindex/tools/rar", NULL },
    { "\\.zip$", "/usr/share/cdindex/tools/zip", NULL },
    { NULL, NULL, NULL }
};

void* cd_external_open(void* dummy, const char* file) {
    cd_external_cmd* cmd;
    for (cmd = cd_external_cmds; cmd->command; cmd++) {
        if (!cmd->__regex) {
            cmd->__regex = (regex_t*)malloc(sizeof(regex_t));
            if (regcomp(cmd->__regex, cmd->regex, REG_EXTENDED|REG_ICASE|REG_NOSUB) != 0) {
                printf("[error] regcomp failed: %s\n", cmd->regex);
                free(cmd->__regex);
                cmd->__regex = NULL;
                continue;
            }
        }
        if (cmd->__regex) {
            if (regexec(cmd->__regex, file, 0, NULL, 0) == 0) break;
        }
    }
    if (cmd->command) {
        char* command = (char*)malloc(strlen(cmd->command) + strsafelen(file) + 9);
        strcpy(command, cmd->command);
        strcat(command, " list \"");
        strsafecat(command, file);
        strcat(command, "\"");
        printf("[external] %s...\n", command);
        FILE* pipe = popen(command, "r");
        free(command);
        return pipe;
    }
    return NULL;
}

int cd_external_read(void* pipe, const char** name, const char** link, struct stat64* stat) {
    static char buf[CD_LINE_BUFFER];
    while (fgets(buf, CD_LINE_BUFFER, pipe)) {
        int length = strlen(buf);
        char* s = buf;
        // Mode
        if (*s == 'd') stat->st_mode = S_IFDIR;
        else if (*s == 'l') stat->st_mode = S_IFLNK;
        else stat->st_mode = S_IFREG;
        s++;
        int group;
        for (group = 2; group >= 0; group--) {
            if (*s++ != '-') stat->st_mode |= CD_READ << (group * 3);
            if (*s++ != '-') stat->st_mode |= CD_WRITE << (group * 3);
            if (*s++ != '-') stat->st_mode |= CD_EXEC << (group * 3);
        }
        if (*s++ != ' ') return -1;
        // UID/GID, Size and Date
        int offset;
        struct tm tm;
        memset(&tm, '\0', sizeof(struct tm));
        if (sscanf(s, "%u %u %ld %04d-%02d-%02d %02d:%02d %n",
            &stat->st_uid, &stat->st_gid, &stat->st_size,
            &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min,
            &offset) == EOF) return -1;
        tm.tm_year -= 1900;
        tm.tm_mon--;
        stat->st_mtime = mktime(&tm);
        s += offset;
        // Name and Link
        *name = s;
        s[strlen(s)-1] = '\0';  // overwrite ending '\n'
        if (S_ISLNK(stat->st_mode)) {
            char* slink = strstr(s, " -> ");
            if (slink) {
                *slink = '\0';
                slink += 4;
                *link = slink;
            }
        }
        return length;
    }
    return -1;
}

void cd_external_close(void* pipe) {
    pclose(pipe);
}

static cd_plugin_info cd_external = {
    "external",
    ".+$",
    TRUE,
    NULL,
    cd_external_open,
    cd_external_read,
    cd_external_close,
    NULL,
    NULL,
    NULL
};

void cd_plugin_load_external() {
    cd_add_plugin(&cd_external);
}

void cd_plugin_unload_external() {
    cd_external_cmd* cmd;
    for (cmd = cd_external_cmds; cmd->command; cmd++) {
        if (cmd->__regex) {
            regfree(cmd->__regex);
            free(cmd->__regex);
        }
    }
}
