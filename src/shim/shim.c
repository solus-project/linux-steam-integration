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

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../common/files.h"
#include "config.h"
#include "lsi.h"

/**
 * Required to force Steam into 32-bit detection mode, which is useful for
 * recent issues like the CS:GO 64-bit update with huge FPS drops
 */
#define EMUL32BIN "linux32"

/**
 * Audit path is used for the libintercept library to ensure Steam only uses the
 * host SDL, etc.
 */
#define AUDIT_PATH "/usr/\$LIB/liblsi-intercept.so"

/**
 * Redirect path is used for the libredirect library to perform internal
 * redirections in functions like `open()` where needed to hotfix games
 * at runtime.
 */
#define REDIRECT_PATH "/usr/\$LIB/liblsi-redirect.so"

/**
 * Set up the LD_AUDIT environment - respecting $SNAP if set
 */
static void shim_set_audit_path(void)
{
        const char *extra = NULL;
        static char tgt[PATH_MAX] = { 0 };

#ifdef HAVE_SNAPD_SUPPORT
        /* For snapd, we need to prepend "$SNAP" into the path */
        extra = getenv("SNAP");
#endif

        if (snprintf(tgt, sizeof(tgt), "%s%s", extra ? extra : "", AUDIT_PATH) < 0) {
                setenv("LD_AUDIT", AUDIT_PATH, 1);
                return;
        }

        setenv("LD_AUDIT", tgt, 1);
}

/**
 * Set up LD_PRELOAD, respecting an existing LD_PRELOAD and forcing ourselves
 * to be first in the list.
 */
static void shim_set_ld_preload(void)
{
        const char *preload = NULL;
        static char tgt[PATH_MAX] = { 0 };
        const char *extra = NULL;

#ifdef HAVE_SNAPD_SUPPORT
        /* For snapd, we need to prepend "$SNAP" into the path */
        extra = getenv("SNAP");
#endif

        /* Always need to know about existing LD_PRELOAD */
        preload = getenv("LD_PRELOAD");

        /* Set up string to include any SNAP prefix and existing LD_PRELOAD */
        if (snprintf(tgt,
                     sizeof(tgt),
                     "%s%s%s%s",
                     extra ? extra : "",
                     REDIRECT_PATH,
                     preload ? ":" : "",
                     preload ? preload : "") < 0) {
                setenv("LD_PRELOAD", REDIRECT_PATH, 1);
                return;
        }

        setenv("LD_PRELOAD", tgt, 1);
}

/**
 * Helper to get the Steam binary, respecting "$SNAP" if needed
 */
static const char *shim_get_steam_binary(void)
{
        const char *extra = NULL;
        static char tgt[PATH_MAX] = { 0 };

#ifdef HAVE_SNAPD_SUPPORT
        /* For snapd, we need to prepend "$SNAP" into the path */
        extra = getenv("SNAP");
#endif

        if (snprintf(tgt, sizeof(tgt), "%s%s", extra ? extra : "", STEAM_BINARY) < 0) {
                return STEAM_BINARY;
        }

        return extra;
}

int main(int argc, char **argv)
{
        LsiConfig config = { 0 };
        bool is_x86_64;
        const char *n_argv[argc + 2];
        const char *exec_command = NULL;
        /* Use the appropriate Steam depending on configuration */
        const char *lsi_exec_bin = NULL;
        int i = 1;
        int8_t off = 0;
        int (*vfunc)(const char *, char *const argv[]) = NULL;

        lsi_exec_bin = shim_get_steam_binary();

        /* Initialise config */
        if (!lsi_config_load(&config)) {
                lsi_config_load_defaults(&config);
        }

        is_x86_64 = lsi_system_is_64bit();

        if (!lsi_file_exists(lsi_exec_bin)) {
                lsi_report_failure("Steam isn't currently installed at %s", lsi_exec_bin);
                return EXIT_FAILURE;
        }

        /* Force STEAM_RUNTIME into the environment */
        if (config.use_native_runtime) {
                /* Explicitly disable the runtime */
                setenv("STEAM_RUNTIME", "0", 1);
#ifdef HAVE_LIBINTERCEPT
                /* Only use libintercept in combination with native runtime! */
                if (config.use_libintercept) {
                        shim_set_audit_path();
                }
#endif
#ifdef HAVE_LIBREDIRECT
                /* Only use libredirect in combination with native runtime! */
                if (config.use_libredirect) {
                        shim_set_ld_preload();
                }
#endif
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

        /* A recent regression is to interpret XMODIFIERS and then fail to
         * make it beyond `SDL_InitSubSystem` due to `InitIME` failing to
         * properly use D-BUS ...
         */
        unsetenv("XMODIFIERS");
        unsetenv("GTK_MODULES");

        memset(&n_argv, 0, sizeof(char *) * (argc + 2));

        /* If we're 64-bit and 32-bit is forced, proxy via linux32 */
        if (config.force_32 && is_x86_64) {
                exec_command = EMUL32BIN;
                n_argv[0] = EMUL32BIN;
                n_argv[1] = lsi_exec_bin;
                off = 1;
                /* Use linux32 in the path */
                vfunc = execvp;
        } else {
                /* Directly call lsi_exec_bin */
                exec_command = lsi_exec_bin;
                n_argv[0] = lsi_exec_bin;
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
                                   lsi_exec_bin);
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
