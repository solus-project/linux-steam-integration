/*
 * This file is part of linux-steam-integration.
 *
 * Copyright © 2016 Ikey Doherty
 *
 * linux-steam-integration is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "lsi.h"

/**
 * Required to force Steam into 32-bit detection mode, which is useful for
 * recent issues like the CS:GO 64-bit update with huge FPS drops
 */
#define EMUL32BIN "linux32"

int main(int argc, char **argv)
{
        LsiConfig config = { 0 };
        bool is_x86_64;
        const char *n_argv[argc + 2];
        const char *exec_command = NULL;
        int i = 1;
        int8_t off = 0;
        int (*vfunc)(const char *, char *const argv[]) = NULL;

        /* Initialise config */
        if (!lsi_config_load(&config)) {
                lsi_config_load_defaults(&config);
        }

        is_x86_64 = lsi_system_is_64bit();

        if (!lsi_file_exists(STEAM_BINARY)) {
                lsi_report_failure("Steam isn't currently installed at %s", STEAM_BINARY);
                return EXIT_FAILURE;
        }

        /* Force STEAM_RUNTIME into the environment */
        if (config.use_native_runtime) {
                /* Explicitly disable the runtime */
                setenv("STEAM_RUNTIME", "0", 1);
        } else {
                /* Only preload when needed. */
                if (lsi_system_requires_preload()) {
                        setenv("LD_PRELOAD", lsi_preload_list(), 1);
                }
                setenv("STEAM_RUNTIME", "1", 1);
        }

        /* Vanilla dbus users suffer a segfault on Steam exit, due to incorrect
         * usage of dbus by Steam. Help them out */
        setenv("DBUS_FATAL_WARNINGS", "0", 1);

        memset(&n_argv, 0, sizeof(char *) * (argc + 2));

        /* If we're 64-bit and 32-bit is forced, proxy via linux32 */
        if (config.force_32 && is_x86_64) {
                exec_command = EMUL32BIN;
                n_argv[0] = EMUL32BIN;
                n_argv[1] = STEAM_BINARY;
                off = 1;
                /* Use linux32 in the path */
                vfunc = execvp;
        } else {
                /* Directly call STEAM_BINARY */
                exec_command = STEAM_BINARY;
                n_argv[0] = STEAM_BINARY;
                /* Full path here due to shadow nature */
                vfunc = execv;
        }

        /* Point arguments to arguments passed to us */
        for (i = 1; i < argc; i++) {
                n_argv[i + off] = argv[i];
        }
        n_argv[i + 1 + off] = NULL;

        /* Go execute steam. */
        if (vfunc(exec_command, (char **)n_argv) < 0) {
                lsi_report_failure("Failed to launch Steam: %s [%s]",
                                   strerror(errno),
                                   STEAM_BINARY);
                return EXIT_FAILURE;
        }
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
