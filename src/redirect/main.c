/*
 * This file is part of linux-steam-integration.
 *
 * Copyright © 2017 Ikey Doherty <ikey@solus-project.com>
 *
 * linux-steam-integration is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nica/util.h"

static volatile bool lsi_init = false;

/**
 * Determine the basename'd process
 */
static inline char *get_process_name(void)
{
        autofree(char) *realp = NULL;
        char *basep = NULL;

        realp = realpath("/proc/self/exe", NULL);
        if (!realp) {
                return false;
        }

        basep = basename(realp);
        return strdup(basep);
}

/**
 * We'll only perform teardown code if the process doesn't `abort()` or `_exit()`
 */
__attribute__((destructor)) static void lsi_redirect_shutdown(void)
{
        if (!lsi_init) {
                return;
        }
        lsi_init = false;
        fprintf(stderr, "Unloading lsi_redirect\n");
}

/**
 * Main entry point into this redirect module.
 *
 * We'll check the process name and determine if we're interested in installing
 * some redirect hooks into this library.
 * If we register interest, we'll install an `atexit` handler and mark ourselves
 * with `lsi_init`, so that we'll attempt to cleanly tear down resources on exit.
 */
__attribute__((constructor)) static void lsi_redirect_init(void)
{
        autofree(char) *process_name = get_process_name();
        if (!process_name) {
                fprintf(stderr, "Failure to allocate memory! %s\n", strerror(errno));
                return;
        }

        fprintf(stderr, "Loading lsi_redirect for: %s\n", process_name);
        lsi_init = true;

        /* We might not get an unload, so chain onto atexit */
        atexit(lsi_redirect_shutdown);
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
