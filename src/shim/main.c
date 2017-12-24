/*
 * This file is part of linux-steam-integration.
 *
 * Copyright © 2016-2017 Ikey Doherty <ikey@solus-project.com>
 *
 * linux-steam-integration is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#define _GNU_SOURCE

#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/files.h"
#include "config.h"
#include "lsi.h"
#include "shim.h"

/**
 * Helper to get the Steam binary, respecting "$SNAP" if needed
 */
static const char *shim_get_steam_binary(const char *prefix)
{
        static char tgt[PATH_MAX] = { 0 };

        if (snprintf(tgt, sizeof(tgt), "%s%s", prefix ? prefix : "", STEAM_BINARY) < 0) {
                return STEAM_BINARY;
        }

        return tgt;
}

int main(int argc, char **argv)
{
        const char *operation_prefix = NULL;
        const char *steam_binary = NULL;
        __attribute__((unused)) const char *tdir = NULL;
        __attribute__((unused)) int dir_ret = 0;

        if (!shim_bootstrap()) {
                return EXIT_FAILURE;
        }

#ifdef HAVE_SNAPD_SUPPORT
        operation_prefix = getenv("SNAP");
        tdir = getenv("SNAP_USER_COMMON");
#endif

        steam_binary = shim_get_steam_binary(operation_prefix);

        if (!lsi_file_exists(steam_binary)) {
                lsi_report_failure("Steam isn't currently installed at %s", steam_binary);
                return EXIT_FAILURE;
        }

        if (tdir) {
                dir_ret = chdir(tdir);
        }

        return shim_execute(steam_binary, --argc, ++argv);
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 expandtab:
 * :indentSize=8:tabSize=8:noTabs=true:
 */
