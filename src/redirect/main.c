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
#include <fcntl.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../common/common.h"
#include "../common/files.h"
#include "../common/log.h"
#include "nica/util.h"

#include "profile.h"
#include "redirect.h"

#define _STRINGIFY(x) #x

#define SYMBOL_BINDING(l, x)                                                                       \
        {                                                                                          \
                .handle = &lsi_table.handles.l, .name = _STRINGIFY(x),                             \
                .func = (void **)(&lsi_table.x), .func_size = sizeof(lsi_table.x)                  \
        }

/**
 * Cheap and cheerful, this is our default fake unity3d config which we write
 * out to trick unity3d games into reading a non broken config on Linux.
 * This forces the equivalent of "-screen-fullscreen 0" because the older
 * Unity builds will set fullscreen to 1, but they'll also set the resolution
 * width and height to 0 (which is then clamped)
 */
static const char *unity3d_config =
    "\
<unity_prefs version_major=\"1\" version_minor=\"1\">\n\
	<pref name=\"Screenmanager Is Fullscreen mode\" type=\"int\">0</pref>\n\
</unity_prefs>\n\
";

/**
 * When we rewrite the config file in place we'll repllace the "Is Fullscreen mode"
 * line with this one so that we forcibly disable it.
 */
static const char *unity3d_snippet =
    "	<pref name=\"Screenmanager Is Fullscreen mode\" type=\"int\">0</pref>\n";

static void lsi_maybe_init_unity3d(const char *p);
static void lsi_unity_backup_config(void);
static FILE *lsi_unity_redirect(const char *p, const char *modes);
static FILE *lsi_unity_get_config_file(const char *modes);
static void lsi_unity_trim_copy_config(FILE *from, FILE *to);

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

typedef FILE *(*lsi_fopen64_file)(const char *p, const char *modes);

/**
 * Known profiles
 */
static lsi_profile_generator_func generators[] = {
        &lsi_redirect_profile_new_ark,
        &lsi_redirect_profile_new_project_highrise,
};

/**
 * Global storage of handles for nicer organisation.
 */
typedef struct LsiRedirectTable {
        lsi_open_file open;
        lsi_fopen64_file fopen64;

        /* Allow future handle opens.. */
        struct {
                void *libc;
        } handles;

        /* Our shm_open() unity3d redirect.. */
        struct {
                char *original_config_path;
                char *config_path;
                char *shm_path;
                bool enabled;
                bool failed;
                bool had_init;
        } unity3d;
} LsiRedirectTable;

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

        if (lsi_table.unity3d.original_config_path) {
                lsi_unity_backup_config();
                free(lsi_table.unity3d.original_config_path);
                lsi_table.unity3d.original_config_path = NULL;
        }
        if (lsi_table.unity3d.config_path) {
                free(lsi_table.unity3d.config_path);
                lsi_table.unity3d.config_path = NULL;
        }

        if (lsi_table.unity3d.shm_path) {
                (void)shm_unlink(lsi_table.unity3d.shm_path);
                free(lsi_table.unity3d.shm_path);
                lsi_table.unity3d.shm_path = NULL;
        }

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
        autofree(char) *xdg_config_dir = NULL;

        /* Absolute path to the process for fine-grained matching */
        process_name = lsi_get_process_name();

        if (!process_name) {
                fputs("Out of memory!\n", stderr);
                return;
        }

        /* Ensure we know the path to unity3d config */
        xdg_config_dir = lsi_get_user_config_dir();
        if (asprintf(&lsi_table.unity3d.config_path, "%s/unity3d", xdg_config_dir) < 0) {
                fputs("OOM\n", stderr);
                abort();
        }

        /* shm path for our fake unity3d config path */
        if (asprintf(&lsi_table.unity3d.shm_path,
                     "/u%d-LinuxSteamIntegration.unity3d.%d",
                     getuid(),
                     getpgrp()) < 0) {
                fputs("OOM\n", stderr);
                abort();
        }

        /* Symbolic, we don't just use this for anyone, yknow. :P */
        lsi_table.unity3d.enabled = false;
        lsi_table.unity3d.failed = false;

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

/**
 * The unity3d config path has been accessed, so we need to turn on our
 * workaround for unity3d prefs
 */
static void lsi_maybe_init_unity3d(const char *p)
{
        if (lsi_table.unity3d.enabled) {
                return;
        }

        if (lsi_table.unity3d.config_path && !strstr(p, lsi_table.unity3d.config_path)) {
                return;
        }

        /* We're in action */
        lsi_table.unity3d.enabled = true;
        lsi_log_set_id("unity3d");
        lsi_log_error("Activating \"black screen of nope\" workaround");
}

static inline bool str_has_prefix(const char *a, const char *b)
{
        size_t lena, lenb;
        lena = strlen(a);
        lenb = strlen(b);
        if (lenb > lena) {
                return false;
        }
        return strncmp(a, b, lenb) == 0;
}

/**
 * Determine if the input path is a unity3d "prefs" file
 */
static bool is_unity3d_prefs_file(const char *p)
{
        autofree(char) *clone = NULL;
        char *basenom = NULL;

        /* Must only happen when enabled */
        if (!lsi_table.unity3d.enabled) {
                return false;
        }

        /* We found it already */
        if (lsi_table.unity3d.original_config_path) {
                return false;
        }

        /* Must be in config path */
        if (!str_has_prefix(p, lsi_table.unity3d.config_path)) {
                return false;
        }

        clone = strdup(p);
        if (!clone) {
                return false;
        }
        basenom = basename(clone);
        if (!basenom) {
                return false;
        }

        /* Must be called "prefs" */
        return strcmp(basenom, "prefs") == 0 ? true : false;
}

/**
 * Copy the config from the @from file to the @to file, which
 * will also strip out any "evil" lines, i.e. the fullscreen
 * stanza.
 *
 * If the input file does not exist, the default configuration
 * will be used.
 */
static void lsi_unity_trim_copy_config(FILE *from, FILE *to)
{
        char *buf = NULL;
        size_t sn = 0;
        ssize_t r = 0;

        /* No input? Write the default configuration then */
        if (!from) {
                if (fwrite(unity3d_config, strlen(unity3d_config), 1, to) != 1) {
                        lsi_log_error("Failed to create initial Unity3D config: %s",
                                      strerror(errno));
                }
                return;
        }

        /* Have an input file, write to the output but omit all the bad lines */
        while ((r = getline(&buf, &sn, from)) != -1) {
                const char *to_write = NULL;

                /* Rewrite this line, it breaks games. */
                if (strstr(buf, "Screenmanager Is Fullscreen mode")) {
                        to_write = unity3d_snippet;
                        r = strlen(to_write);
                } else {
                        to_write = buf;
                }

                if (fwrite(to_write, r, 1, to) != 1) {
                        lsi_log_error("Failed to write Unity3D config: %s", strerror(errno));
                        goto failed;
                }

                if (buf) {
                        free(buf);
                        buf = NULL;
                }
        }

        fflush(to);

failed:
        if (buf) {
                free(buf);
        }
}

/**
 * Clone the existing shm configuration into the real config path, and copy
 * all lines that we "like"
 *
 * Basically this ensures we force the dumped config to omit the fullscreen
 * options as they break so many games.
 */
static void lsi_unity_backup_config()
{
        autofree(FILE) *shm_file = NULL;
        autofree(FILE) *dest_file = NULL;

        if (!lsi_table.unity3d.enabled || !lsi_table.unity3d.original_config_path) {
                return;
        }

        shm_file = lsi_unity_get_config_file("r");
        if (!shm_file) {
                return;
        }

        dest_file = lsi_table.fopen64(lsi_table.unity3d.original_config_path, "w");
        if (!dest_file) {
                return;
        }

        lsi_log_debug("Saved Unity3D config to %s", lsi_table.unity3d.original_config_path);
        lsi_unity_trim_copy_config(shm_file, dest_file);
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

        lsi_maybe_init_unity3d(p);

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

/**
 * Get the shm file as a FILE stream
 */
static FILE *lsi_unity_get_config_file(const char *modes)
{
        int perm = O_RDONLY;

        if (strstr(modes, "w")) {
                perm = O_RDWR | O_CREAT | O_TRUNC;
        }

        /* Attempt to shm_open our fake path */
        int fd = shm_open(lsi_table.unity3d.shm_path, perm, 0666);
        if (!fd) {
                return NULL;
        }
        return fdopen(fd, modes);
}

/**
 * We now need to initialise the unity3d config if we haven't done so
 */
static void lsi_unity_init_config(void)
{
        autofree(FILE) *source = NULL;
        autofree(FILE) *dest = NULL;

        if (lsi_table.unity3d.had_init || lsi_table.unity3d.failed) {
                return;
        }

        lsi_table.unity3d.had_init = true;

        dest = lsi_unity_get_config_file("w");
        source = lsi_table.fopen64(lsi_table.unity3d.original_config_path, "r");

        lsi_unity_trim_copy_config(source, dest);
}

/**
 * Redirect requests for non-existant config files in Unity3D to a temporary
 * shm file which we'll write our initial "sane" configuration to.
 */
static FILE *lsi_unity_redirect(const char *p, const char *modes)
{
        FILE *ret = NULL;

        /* Preserve this path for later cloning.. */
        lsi_table.unity3d.original_config_path = strdup(p);

        lsi_unity_init_config();

        ret = lsi_unity_get_config_file(modes);
        if (!ret) {
                return NULL;
        }

        lsi_log_info("fopen64(%s): Redirecting unity config '%s' to shm(%s)",
                     modes,
                     p,
                     lsi_table.unity3d.shm_path);
        return ret;
}

_nica_public_ FILE *fopen64(const char *p, const char *modes)
{
        LsiRedirectOperation op = LSI_OPERATION_OPEN;
        autofree(char) *replacement = NULL;

        /* Must ensure we're **really** initialised, as we might see open happen
         * before the constructor..
         */
        lsi_redirect_init_tables();

        lsi_maybe_init_unity3d(p);

        /* Not interested in this guy apparently */
        if (!lsi_override) {
                goto fallback_open;
        }

        replacement = lsi_get_redirect_path("fopen64", op, p);
        if (replacement) {
                return lsi_table.fopen64(replacement, modes);
        }

fallback_open:

        if (is_unity3d_prefs_file(p)) {
                return lsi_unity_redirect(p, modes);
        }
        return lsi_table.fopen64(p, modes);
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
