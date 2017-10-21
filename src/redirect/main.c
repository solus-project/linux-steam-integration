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
#include <libgen.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../common/files.h"
#include "../common/log.h"
#include "nica/util.h"

#include "redirect.h"

/**
 * Whether we've initialised yet or not.
 */
static volatile bool lsi_init = false;

/**
 * Whether we should actually perform overrides
 * This is determined by the process name
 */
static volatile bool lsi_override = false;

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
 * We'll only perform teardown code if the process doesn't `abort()` or `_exit()`
 */
__attribute__((destructor)) static void lsi_redirect_shutdown(void)
{
        if (!lsi_init) {
                return;
        }
        lsi_init = false;

        if (lsi_table.handles.libc) {
                dlclose(lsi_table.handles.libc);
                lsi_table.handles.libc = NULL;
        }
}

/**
 * Responsible for setting up the vfunc redirect table so that we're able to
 * get `open` and such working before we've hit the constructor, and ensure
 * we only init once.
 */
static void lsi_redirect_init_tables(void)
{
        void *symbol_lookup = NULL;
        char *dl_error = NULL;

        if (lsi_init) {
                return;
        }

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

        /* We might not get an unload, so chain onto atexit */
        atexit(lsi_redirect_shutdown);
        return;

failed:
        /* Pretty damn fatal. */
        lsi_redirect_shutdown();
        abort();
}

/**
 * Main entry point into this redirect module.
 *
 * We'll check the process name and determine if we're interested in installing
 * some redirect hooks into this library.
 * Otherwise we'll continue to operate in a pass-through mode
 */
__attribute__((constructor)) static void lsi_redirect_init(void)
{
        /* Ensure we're open. */
        lsi_redirect_init_tables();
        autofree(char) *process_name = NULL;

        process_name = lsi_get_process_name();

        if (!process_name) {
                fputs("Out of memory!\n", stderr);
                return;
        }

        /* Temporary hack, we'll make this more generic later */
        if (strcmp(process_name, "ShooterGame") == 0) {
                lsi_override = true;
        }

        if (!lsi_override) {
                return;
        }

        lsi_log_debug("Enable lsi_redirect for '%s'", process_name);
}

static char *lsi_redirect_replace_path(const char *path)
{
        char *dir = NULL;
        autofree(char) *tmp = NULL;
        char *transformed_path = NULL;

        /* This is where we'd need to look at what game we are, and what we'd need
         * to replace. For now, we're going to cheat, and give it a fixed path.
         */
        if (!strstr(path,
                    "/ARK/ShooterGame/Content/PrimalEarth/Environment/Water/"
                    "Water_DepthBlur_MIC.uasset")) {
                return strdup(path);
        }

        tmp = strdup(path);
        dir = dirname(tmp);

        /* We want this guy: */
        const char *new_path = "../../../Mods/TheCenter/Assets/Mic/Water_DepthBlur_MIC.uasset";
        if (asprintf(&transformed_path, "%s/%s", dir, new_path) < 0) {
                fputs("OUT OF MEMORY\n", stderr);
                abort();
        }

        if (!lsi_file_exists(transformed_path)) {
                lsi_log_warn("Target path does not exist: %s", new_path);
                free(transformed_path);
                return strdup(path);
        }

        lsi_log_info("Replaced '%s' with '%s'", path, transformed_path);
        return transformed_path;
}

_nica_public_ int open(const char *p, int flags, ...)
{
        va_list va;
        mode_t mode;
        autofree(char) *replaced_path = NULL;

        /* Must ensure we're **really** initialised, as we might see open happen
         * before the constructor..
         */
        lsi_redirect_init_tables();

        /* Grab the mode_t */
        va_start(va, flags);
        mode = va_arg(va, mode_t);
        va_end(va);

        /* Not interested in this guy apparently */
        if (!lsi_override) {
                return lsi_table.open(p, flags, mode);
        }

        replaced_path = lsi_redirect_replace_path(p);
        return lsi_table.open(replaced_path, flags, mode);
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
