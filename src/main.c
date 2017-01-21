/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>

#include "index.h"
#include "base.h"
#include "plugin.h"
#include "extract.h"

#define CD_DEVICE       "/dev/cdrom"
#define CD_MOUNTPOINT   "/media/cdrom"

int main(int argc, char* argv[]) {
    if (argc >= 2) {
        cd_init_plugins();
        cd_plugin_load_archiver();
        cd_plugin_load_external();

        cd_offset offset = 1;
        cd_base* base = cd_base_open(argv[1]);
        if (base != NULL) {
            cd_init_extractors(base);

            cd_header((argc == 4) ? argv[3] : CD_DEVICE, base);
            cd_index((argc >= 3) ? argv[2] : CD_MOUNTPOINT, NULL, &offset, base);

            cd_free_extractors();
            cd_base_close(base);
        }

        cd_plugin_unload_external();
        cd_free_plugins();
    } else {
        printf("[error] please specify database name\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
