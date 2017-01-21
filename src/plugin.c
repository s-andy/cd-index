/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <regex.h>

#include "plugin.h"

cd_plugin_info* cd_plugins;

void cd_init_plugins() {
    cd_plugins = NULL;
}

void cd_free_plugins() {
    cd_plugin_info* plugin;
    for (plugin = cd_plugins; plugin; plugin = plugin->next) {
        if (plugin->__regex) {
            regfree(plugin->__regex);
            free(plugin->__regex);
        }
    }
}

void cd_add_plugin(cd_plugin_info* info) {
    if (info->next) info->next = NULL;
    if (cd_plugins) {
        cd_plugin_info* plugin;
        for (plugin = cd_plugins; plugin->next; plugin = plugin->next);
        plugin->next = info;
    } else {
        cd_plugins = info;
    }
}

cd_plugin_info* cd_find_plugin(const char* file) {
    cd_plugin_info* plugin;
    for (plugin = cd_plugins; plugin; plugin = plugin->next) {
        if (!plugin->__regex) {
            plugin->__regex = (regex_t*)malloc(sizeof(regex_t));
            if (regcomp(plugin->__regex, plugin->regex, REG_EXTENDED|REG_ICASE|REG_NOSUB) != 0) {
                printf("[error] regcomp failed: %s\n", plugin->regex);
                free(plugin->__regex);
                plugin->__regex = NULL;
                continue;
            }
        }
        if (plugin->__regex) {
            if (regexec(plugin->__regex, file, 0, NULL, 0) == 0) break;
        }
    }
    return plugin;
}
