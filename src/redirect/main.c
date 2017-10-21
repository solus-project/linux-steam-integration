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

#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nica/util.h"

static volatile bool lsi_init = false;

/**
 * Handle definitions
 */
typedef int (*lsi_open_file)(const char *p, int flags, mode_t mode);

/**
 * Global storage of handles for nicer organisation.
 */
typedef struct LsiRedirectTable {
        lsi_open_file open;

        /* Allow future handle opens.. */
        struct {
                void *libc;
        } handles;
} LsiRedirectTable;

/**
 * Our redirect instance, stack only.
 */
static LsiRedirectTable lsi_table = { 0 };

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

        if (lsi_table.handles.libc) {
                dlclose(lsi_table.handles.libc);
                lsi_table.handles.libc = NULL;
        }
}

/**
 * Main entry point into this redirect module.
 *
 * We'll check the process name and determine if we're interested in installing
 * some redirect hooks into this library.
 */
__attribute__((constructor)) static void lsi_redirect_init(void)
{
        if (lsi_init) {
                return;
        }

        autofree(char) *process_name = get_process_name();
        void *symbol_lookup = NULL;
        char *dl_error = NULL;

        if (!process_name) {
                fprintf(stderr, "Failure to allocate memory! %s\n", strerror(errno));
                return;
        }

        fprintf(stderr, "Loading lsi_redirect for: %s\n", process_name);
        lsi_init = true;

        /* Try to explicitly open libc. We can't safely rely on RTLD_NEXT
         * as we might be dealing with a strange link order
         */
        lsi_table.handles.libc = dlopen("libc.so.6", RTLD_LAZY);
        if (!lsi_table.handles.libc) {
                fprintf(stderr, "Unable to grab libc.so.6 handle: %s\n", dlerror());
                goto failed;
        }

        /* Safely look up the symbol */
        symbol_lookup = dlsym(lsi_table.handles.libc, "open");
        dl_error = dlerror();
        if (dl_error || !symbol_lookup) {
                fprintf(stderr, "Failed to bind 'open': %s\n", dl_error);
                goto failed;
        }

        /* Have the symbol correctly, copy it to make it usable */
        memcpy(&lsi_table.open, &symbol_lookup, sizeof(lsi_table.open));

        fprintf(stderr, "lsi_redirect now loaded\n");

        /* We might not get an unload, so chain onto atexit */
        atexit(lsi_redirect_shutdown);
        return;

failed:
        /* Pretty damn fatal. */
        lsi_redirect_shutdown();
        abort();
}

_nica_public_ int open(const char *p, int flags, ...)
{
        va_list va;
        mode_t mode;

        /* Must ensure we're **really** initialised, as we might see open happen
         * before the constructor..
         */
        lsi_redirect_init();

        /* Grab the mode_t */
        va_start(va, flags);
        mode = va_arg(va, mode_t);
        va_end(va);

        fprintf(stderr, "begin open: %s\n", p);
        return lsi_table.open(p, flags, mode);
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
