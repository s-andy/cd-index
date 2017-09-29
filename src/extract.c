/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#include <stdio.h>

#include "extract.h"

cd_extractor_info* cd_extractors;

void cd_init_extractors(cd_base* base) {
    cd_extractors = NULL;
    cd_extractor_load_audio(base);
    cd_extractor_load_rawimage(base);
}

void cd_free_extractors() {
    cd_extractor_info* extractor;
    for (extractor = cd_extractors; extractor; extractor = extractor->next) {
        if (extractor->__regex) {
            regfree(extractor->__regex);
            free(extractor->__regex);
        }
        if (extractor->finish) {
            if (extractor->__udata) extractor->finish(extractor->__udata);
            else {
                printf("[error] user data disappeared: %s\n", extractor->name);
            }
        }
    }
}

void cd_add_extractor(cd_extractor_info* info, cd_base* base) {
    if (info->next) info->next = NULL;
    if (info->init) {
        if (!info->finish) {
            printf("[error] missing finish function: %s\n", info->name);
            return;
        }
        info->__udata = info->init(base);
        if (!info->__udata) {
            printf("[error] failed to load extractor: %s\n", info->name);
            return;
        }
    }
    if (cd_extractors) {
        cd_extractor_info* extractor;
        for (extractor = cd_extractors; extractor->next; extractor = extractor->next);
        extractor->next = info;
    } else {
        cd_extractors = info;
    }
}

cd_extractor_info* cd_find_extractor(const char* file) {
    cd_extractor_info* extractor;
    for (extractor = cd_extractors; extractor; extractor = extractor->next) {
        if (!extractor->__regex) {
            extractor->__regex = (regex_t*)malloc(sizeof(regex_t));
            if (regcomp(extractor->__regex, extractor->regex, REG_EXTENDED|REG_ICASE|REG_NOSUB) != 0) {
                printf("[error] regcomp failed: %s\n", extractor->regex);
                free(extractor->__regex);
                extractor->__regex = NULL;
                continue;
            }
        }
        if (extractor->__regex) {
            if (regexec(extractor->__regex, file, 0, NULL, 0) == 0) break;
        }
    }
    return extractor;
}
