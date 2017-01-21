/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>

#include "find.h"
#include "search.h"

#define CD_DEFDIR   "/var/lib/cdindex"

#define false   0
#define true    1

/* find's options:
 *  -iname  - case insensitive search by name
 *  -iregex - case insensitive regular expression
 *  -mtime  - data was last modified n*24 hours ago
 *  -name   - search by name
 *  -regex  - regular expression search
 *  -size   - search by size
 *  -type   - search by file type
 *  -printf - print found files
 *
 * our options:
 *  -nodefdir - do not use default directory
 *  -noarc    - do not go inside archives
 */

void cd_find_freereq(cd_find_req* req) {
    cd_find_exp* exp;
    cd_find_exp* item;
    for (exp = req->exp; exp;) {
        item = exp;
        exp = exp->next;
        if ((item->flags & FIND_MASK) == FIND_REGEXP) {
            regfree(item->regex);
            free(item->regex);
        }
        free(item);
    }
    free(req);
}

cd_find_req* cd_find_getargs(int argc, char* argv[]) {
    int i = 1;
    cd_find_req* req = (cd_find_req*)calloc(1, sizeof(cd_find_req));
    if ((argc > 1) && (argv[i][0] != '-')) {
        if (argv[i][0] == '/') {
            if (argv[i][1]) req->path = argv[i] + 1;
        } else {
            req->path = strchr(argv[i], '/');
            if (req->path) {
                ((char*)req->path)[0] = '\0';
                if (!req->path[1]) req->path = NULL;
                else req->path++;
            }
            req->cdimask = argv[i];
        }
        i++;
    }
    cd_find_exp* exp = NULL;
    for (; i < argc; i++) {
        if ((argv[i][0] == '-') && !isdigit(argv[i][1])) {
            if (exp) {
                printf("cdfind: missing argument to `%s'\n", argv[i-1]);
                free(exp);
                cd_find_freereq(req);
                return NULL;
            } else {
                if (!strcmp(&argv[i][1], "nodefdir")) {
                    req->nodefdir = true;
                } else if (!strcmp(&argv[i][1], "noarc")) {
                    req->noarc = true;
                } else if (!strcmp(&argv[i][1], "printf")) {
                } else {
                    exp = (cd_find_exp*)malloc(sizeof(cd_find_exp));
                    exp->next = NULL;
                    if (!strcmp(&argv[i][1], "name")) {
                        exp->flags = FIND_WILDCARD;
                    } else if (!strcmp(&argv[i][1], "iname")) {
                        exp->flags = FIND_WILDCARD|FIND_ICASE;
                    } else if (!strcmp(&argv[i][1], "regex")) {
                        exp->flags = FIND_REGEXP;
                    } else if (!strcmp(&argv[i][1], "iregex")) {
                        exp->flags = FIND_REGEXP|FIND_ICASE;
                    } else if (!strcmp(&argv[i][1], "type")) {
                        exp->flags = FIND_TYPE;
                    } else if (!strcmp(&argv[i][1], "mtime")) {
                        exp->flags = FIND_MTIME;
                    } else if (!strcmp(&argv[i][1], "size")) {
                        exp->flags = FIND_SIZE;
                    } else {
                        printf("cdfind: invalid predicate `%s'\n", argv[i]);
                        free(exp);
                        cd_find_freereq(req);
                        return NULL;
                    }
                }
            }
        } else {
            if (exp) {
                if ((exp->flags & FIND_MASK) == FIND_WILDCARD) {
                    exp->wildcard = argv[i];
                } else if ((exp->flags & FIND_MASK) == FIND_REGEXP) {
                    int res;
                    regex_t* reg = (regex_t*)malloc(sizeof(regex_t));
                    int cflags = REG_EXTENDED|REG_NOSUB;
                    if (exp->flags & FIND_ICASE) cflags |= REG_ICASE;
                    if ((res = regcomp(reg, argv[i], cflags))) {
                        size_t bufsiz = regerror(res, reg, NULL, 0);
                        char error[bufsiz];
                        regerror(res, reg, error, bufsiz);
                        free(reg);
                        printf("cdfind: %s\n", error);
                        free(exp);
                        cd_find_freereq(req);
                        return NULL;
                    } else {
                        exp->regex = reg;
                    }
                } else if ((exp->flags & FIND_MASK) == FIND_TYPE) {
                    if (!strcmp(argv[i], "d")) {
                        exp->type = CD_DIR;
                    } else if (!strcmp(argv[i], "f")) {
                        exp->type = CD_REG;
                    } else if (!strcmp(argv[i], "l")) {
                        exp->type = CD_LNK;
                    } else if (!strcmp(argv[i], "a")) { // ours
                        exp->type = CD_ARC;
                    } else {
                        printf("cdfind: invalid argument `%s' to `-type'\n", argv[i]);
                        free(exp);
                        cd_find_freereq(req);
                        return NULL;
                    }
                } else if ((exp->flags & FIND_MASK) == FIND_MTIME) {
                    const char* days = argv[i];
                    if (days[0] == '-') {
                        exp->flags |= FIND_LESS;    // less than X days ago
                        days++;
                    } else if (days[0] == '+') {
                        exp->flags |= FIND_GREATER; // more than X days ago
                        days++;
                    }
                    char* end;
                    int ndays = strtoul(days, &end, 10);
                    if (*end) {
                        printf("cdfind: invalid argument `%s' to `-mtime'\n", argv[i]);
                        free(exp);
                        cd_find_freereq(req);
                        return NULL;
                    }
                    time_t now = time(NULL) - ndays * 86400;
                    struct tm* tm = localtime(&now);
                    tm->tm_sec = 0;
                    tm->tm_min = 0;
                    tm->tm_hour = 0;
                    exp->time = mktime(tm);
                } else if ((exp->flags & FIND_MASK) == FIND_SIZE) {
                    const char* size = argv[i];
                    if (size[0] == '-') {
                        exp->flags |= FIND_LESS;    // less than X days ago
                        size++;
                    } else if (size[0] == '+') {
                        exp->flags |= FIND_GREATER; // more than X days ago
                        size++;
                    }
                    if (!isdigit(size[0])) {
                        printf("cdfind: invalid argument `%s' to `-size'\n", argv[i]);
                        free(exp);
                        cd_find_freereq(req);
                        return NULL;
                    }
                    char* end;
                    cd_size nsize = strtoul(size, &end, 10);
                    if (end[0]) {
                        if (end[1]) {
                            printf("cdfind: invalid argument `%s' to `-size'\n", argv[i]);
                            free(exp);
                            cd_find_freereq(req);
                            return NULL;
                        } else if (end[0] == 'b') {
                            nsize *= 512;
                        } else if (end[0] == 'w') {
                            nsize *= 2;
                        } else if (end[0] == 'k') {
                            nsize *= 1024;
                        } else if (end[0] == 'M') {
                            nsize *= 1048576;
                        } else if (end[0] == 'G') {
                            nsize *= 1073741824;
                        } else if (end[0] != 'c') {
                            printf("cdfind: invalid argument `%s' to `-size'\n", argv[i]);
                            free(exp);
                            cd_find_freereq(req);
                            return NULL;
                        }
                    } else {
                        nsize *= 512;   // defaukl (see find(1))
                    }
                    exp->size = nsize;
                }
                if (req->exp) {
                    cd_find_exp* item;
                    for (item = req->exp; item->next; item = item->next);
                    item->next = exp;
                } else req->exp = exp;
                exp = NULL;
            } else if (!strcmp(argv[i-1], "-printf")) {
                req->format = argv[i];
            } else {
                printf("cdfind: paths must precede expression\n");
                cd_find_freereq(req);
                return NULL;
            }
        }
    }
    if (exp) {
        printf("cdfind: missing argument to `%s'\n", argv[i-1]);
        free(exp);
        cd_find_freereq(req);
        return NULL;
    }
    return req;
}

int main(int argc, char* argv[]) {
    cd_find_req* req = cd_find_getargs(argc, argv);
    if (req) {
        int result = cd_search((req->nodefdir) ? "./" : CD_DEFDIR, req);
        cd_find_freereq(req);
        if (result) return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
