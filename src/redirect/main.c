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

#include "config.h"

/* Expose getpwuid override in snapd */
#ifdef HAVE_SNAPD_SUPPORT
#include <pwd.h>
#endif

#include "../common/common.h"
#include "../common/files.h"
#include "../common/log.h"
#include "nica/util.h"

#include "private.h"
#include "profile.h"
#include "redirect.h"

#define _STRINGIFY(x) #x

#define SYMBOL_BINDING(l, x)                                                                       \
        {                                                                                          \
                .handle = &lsi_table.handles.l, .name = _STRINGIFY(x),                             \
                .func = (void **)(&lsi_table.x), .func_size = sizeof(lsi_table.x)                  \
        }

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
 * Known profiles
 */
static lsi_profile_generator_func generators[] = {
        &lsi_redirect_profile_new_ark,
        &lsi_redirect_profile_new_project_highrise,
};

/**
 * Easy mapping to make it quick and easy to add new function overrides
 */
typedef struct LsiSymbolBinding {
        void **handle;
        char *name;
        void **func;
        size_t func_size;
} LsiSymbolBinding;

/**
 * Our redirect instance, stack only.
 */
static LsiRedirectTable lsi_table = { 0 };

/**
 * Contains all of our replacement rules.
 */
static LsiRedirectProfile *lsi_profile = NULL;

/**
 * Our current set of libc bindings. This sets up the LsiRedirectTable with
 * all of our needed dlsym() functions from libc.so.6.
 */
static LsiSymbolBinding lsi_libc_bindings[] = {
        SYMBOL_BINDING(libc, open),
        SYMBOL_BINDING(libc, fopen64),
#ifdef HAVE_SNAPD_SUPPORT
        SYMBOL_BINDING(libc, getpwuid),
#endif
};

/**
 * We'll only perform teardown code if the process doesn't `abort()` or `_exit()`
 */
__attribute__((destructor)) static void lsi_redirect_shutdown(void)
{
        if (lsi_profile) {
                lsi_redirect_profile_free(lsi_profile);
                lsi_profile = NULL;
        }

        if (!lsi_init) {
                return;
        }
        lsi_init = false;

        lsi_unity_cleanup(&lsi_table);

        if (lsi_table.handles.libc) {
                dlclose(lsi_table.handles.libc);
                lsi_table.handles.libc = NULL;
        }
}

/**
 * Internal helper to perform the symbol binding
 */
static bool lsi_redirect_bind_function(void *handle, const char *name, void **out_func,
                                       size_t func_size)
{
        void *symbol_lookup = NULL;
        char *dl_error = NULL;

        /* Safely look up the symbol */
        symbol_lookup = dlsym(handle, name);
        dl_error = dlerror();
        if (dl_error || !symbol_lookup) {
                fprintf(stderr, "Failed to bind '%s': %s\n", name, dl_error);
                return false;
        }

        /* Have the symbol correctly, copy it to make it usable */
        memcpy(out_func, &symbol_lookup, func_size);
        return true;
}

/**
 * Responsible for setting up the vfunc redirect table so that we're able to
 * get `open` and such working before we've hit the constructor, and ensure
 * we only init once.
 */
static void lsi_redirect_init_tables(void)
{
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

        /* Hook up all necessary symbol binding */
        for (size_t i = 0; i < ARRAY_SIZE(lsi_libc_bindings); i++) {
                LsiSymbolBinding *binding = &lsi_libc_bindings[i];
                if (!lsi_redirect_bind_function(*(binding->handle),
                                                binding->name,
                                                binding->func,
                                                binding->func_size)) {
                        goto failed;
                }
        }

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
        char **paths = NULL;
        char **orig = NULL;

        /* Absolute path to the process for fine-grained matching */
        process_name = lsi_get_process_name();

        if (!process_name) {
                fputs("Out of memory!\n", stderr);
                return;
        }

        lsi_unity_startup(&lsi_table);

        /* Grab the steam installation directories */
        orig = paths = lsi_get_steam_paths();

        /* For each path try to form a valid profile for the process name and Steam directory */
        while (*paths) {
                for (size_t i = 0; i < ARRAY_SIZE(generators); i++) {
                        lsi_profile = generators[i](process_name, *paths);
                        if (lsi_profile) {
                                goto use_profile;
                        }
                }
                ++paths;
        }

use_profile:
        /* Rewind path set */
        paths = orig;

        if (!lsi_profile) {
                goto clean_array;
        }

        lsi_override = true;
        lsi_log_debug("Enable lsi_redirect for '%s'", lsi_profile->name);

clean_array:

        /* Free them again */
        if (paths) {
                while (*paths) {
                        free(*paths);
                        ++paths;
                }
                free(orig);
        }
}

/**
 * Get a redirect path from the table if it exists, otherwise return NULL
 */
static char *lsi_get_redirect_path(const char *syscall_id, LsiRedirectOperation op, const char *p)
{
        autofree(char) *path = NULL;
        LsiRedirect *redirect = NULL;

        /* Get the absolute path here */
        path = realpath(p, NULL);
        if (!path) {
                return NULL;
        }

        /* find a valid replacement */
        for (redirect = lsi_profile->op_table[op]; redirect; redirect = redirect->next) {
                /* Currently we only known path replacements */
                if (redirect->type != LSI_REDIRECT_PATH) {
                        continue;
                }
                if (strcmp(redirect->path_source, path) != 0) {
                        continue;
                }
                if (!lsi_file_exists(redirect->path_target)) {
                        lsi_log_warn("Replacement path does not exist: %s", redirect->path_target);
                        return NULL;
                }
                lsi_log_info("%s(): Replaced '%s' with '%s'",
                             syscall_id,
                             path,
                             redirect->path_target);
                return strdup(redirect->path_target);
        }

        /* Got nothin' */
        return NULL;
}

_nica_public_ int open(const char *p, int flags, ...)
{
        va_list va;
        mode_t mode;
        LsiRedirectOperation op = LSI_OPERATION_OPEN;
        autofree(char) *replacement = NULL;

        /* Must ensure we're **really** initialised, as we might see open happen
         * before the constructor..
         */
        lsi_redirect_init_tables();

        /* Grab the mode_t */
        va_start(va, flags);
        mode = va_arg(va, mode_t);
        va_end(va);

        lsi_maybe_init_unity3d(&lsi_table, p);

        /* Not interested in this guy apparently */
        if (!lsi_override) {
                goto fallback_open;
        }

        replacement = lsi_get_redirect_path("open", op, p);
        if (replacement) {
                return lsi_table.open(replacement, flags, mode);
        }

fallback_open:
        return lsi_table.open(p, flags, mode);
}

_nica_public_ FILE *fopen64(const char *p, const char *modes)
{
        LsiRedirectOperation op = LSI_OPERATION_OPEN;
        autofree(char) *replacement = NULL;

        /* Must ensure we're **really** initialised, as we might see open happen
         * before the constructor..
         */
        lsi_redirect_init_tables();

        lsi_maybe_init_unity3d(&lsi_table, p);

        /* Not interested in this guy apparently */
        if (!lsi_override) {
                goto fallback_open;
        }

        replacement = lsi_get_redirect_path("fopen64", op, p);
        if (replacement) {
                return lsi_table.fopen64(replacement, modes);
        }

fallback_open:

        if (is_unity3d_prefs_file(&lsi_table, p)) {
                return lsi_unity_redirect(&lsi_table, p, modes);
        }
        return lsi_table.fopen64(p, modes);
}

#ifdef HAVE_SNAPD_SUPPORT

#include <unistd.h>

_nica_public_ struct passwd *getpwuid(uid_t uid)
{
        /* Must ensure we're **really** initialised, as we might see open happen
         * before the constructor..
         */
        lsi_redirect_init_tables();

        struct passwd *ret = NULL;
        const char *snap_root = getenv("SNAP_USER_COMMON");

        /* If they're requesting our uid and SNAP_USER_COMMON is set, then
         * let us override the home directory to be correct.
         */
        ret = lsi_table.getpwuid(uid);
        if (!ret) {
                return NULL;
        }

        if (uid == getuid() && snap_root && *snap_root) {
                ret->pw_dir = (char *)snap_root;
        }
        return ret;
}

#endif

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
