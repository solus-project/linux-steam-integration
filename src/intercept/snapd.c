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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/common.h"
#include "../common/files.h"
#include "../common/log.h"
#include "intercept.h"

/**
 * Contains the exact soname paths that the linker will request
 */
static const char *libgl_source_table[] = {
#if UINTPTR_MAX == 0xffffffffffffffff
        "/usr/lib64/libGL.so.1",
        "/usr/lib64/libEGL.so.1",
        /* Duped due to lib vs lib64 differences between solus + snap */
        "/usr/lib/libGL.so.1",
        "/usr/lib/libEGL.so.1",
#else
        /* 32bit support */
        "/usr/lib32/libGL.so.1",
        "/usr/lib32/libEGL.so.1",
#endif
};

/**
 * Contains the exact soname paths that we'll return for biarch situations
 *
 * TODO: Also support Ubuntu/Debian multiarch overlay
 */
static const char *libgl_target_table[] = {
#if UINTPTR_MAX == 0xffffffffffffffff
        "/var/lib/snapd/lib/gl/libGL.so.1",
        "/var/lib/snapd/lib/gl/libEGL.so.1",
        /* Duped due to lib vs lib64 differences between solus + snap */
        "/var/lib/snapd/lib/gl/libGL.so.1",
        "/var/lib/snapd/lib/gl/libEGL.so.1",
#else
        /* 32bit support */
        "/var/lib/snapd/lib/gl32/libGL.so.1",
        "/var/lib/snapd/lib/gl32/libEGL.so.1",
#endif
};

/**
 * If the hostfs provided links don't exist, then we didn't get any links from
 * the snapd setup routine, so we'll just repoint back to mesa.
 */
static const char *libgl_mesa_table[] = {
#if UINTPTR_MAX == 0xffffffffffffffff
        "/usr/lib64/glx-provider/default/libGL.so.1",
        "/usr/lib64/glx-provider/default/libEGL.so.1",
        /* Duped due to lib vs lib64 differences between solus + snap */
        "/usr/lib/glx-provider/default/libGL.so.1",
        "/usr/lib/glx-provider/default/libEGL.so.1",
#else
        /* 32bit support */
        "/usr/lib32/glx-provider/default/libGL.so.1",
        "/usr/lib32/glx-provider/default/libEGL.so.1",
#endif
};

/**
 * These are the NVIDIA libraries will attempt to redirect on demand.
 */
static const char *libgl_nvidia_matches[] = {
        "libGLdispatch", "libnv", "NVIDIA", "nvidia.so", "cuda.", "GLX",
};

bool lsi_override_snapd_nvidia(const char *name, const char **soname)
{
        const char *nvidia_target_dir = NULL;
        static char path_lookup[PATH_MAX];
        static char path_copy[PATH_MAX];
        char *small_name = NULL;
        bool match = false;

        /* Only mangle when we start looking for paths */
        if (!strstr(name, "/")) {
                return false;
        }

        /* Must be proper versioned libs */
        if (!strstr(name, ".so.")) {
                return false;
        }

        /* If this guy exists we don't actually care.. */
        if (lsi_file_exists(name)) {
                return false;
        }

        /* Check if we have some basic pattern for an NVIDIA library here */
        for (size_t i = 0; i < ARRAY_SIZE(libgl_nvidia_matches); i++) {
                if (strstr(name, libgl_nvidia_matches[i])) {
                        match = true;
                        break;
                }
        }

        /* Unwanted library */
        if (!match) {
                return false;
        }

#if UINTPTR_MAX == 0xffffffffffffffff
        /* 64-bit libdir */
        nvidia_target_dir = "/var/lib/snapd/lib/gl";
#else
        /* 32-bit libdir */
        nvidia_target_dir = "/var/lib/snapd/lib/gl32";
#endif

        /* Grab the link name */
        if (!strcpy(path_copy, name)) {
                return false;
        }
        small_name = basename(path_copy);

        if (snprintf(path_lookup, sizeof(path_lookup), "%s/%s", nvidia_target_dir, small_name) <
            0) {
                return false;
        }

        /* Sod all we can do here */
        if (!lsi_file_exists(path_lookup)) {
                lsi_log_error("Missing NVIDIA file: %s (%s)", name, path_lookup);
                return false;
        }

        *soname = path_lookup;
        lsi_log_debug(
            "Enforcing NVIDIA snapd driver links: \033[31;1m%s\033[0m -> \033[34;1m%s\033[0m",
            name,
            path_lookup);

        return true;
}

bool lsi_override_snapd_dri(const char *name, const char **soname)
{
        const char *lib_dir = NULL;
        static char path_lookup[PATH_MAX];
        static char path_copy[PATH_MAX];
        char *small_name = NULL;

        /* Only mangle when we start looking for paths */
        if (!strstr(name, "/")) {
                return false;
        }

        /* Must be a proper dri lookup */
        if (!strstr(name, "_dri.so") && !strstr(name, "_drv_video.so")) {
                return false;
        }

        /* Never deal with source names! */
        if (strstr(name, "/var/lib/snapd/")) {
                return false;
        }

        /* If this guy exists we don't actually care.. */
        if (lsi_file_exists(name)) {
                return false;
        }

#if UINTPTR_MAX == 0xffffffffffffffff
        /* 64-bit libdir */
        lib_dir = "/usr/lib/dri";
#else
        /* 32-bit libdir */
        lib_dir = "/usr/lib32/dri";
#endif

        /* Grab the link name */
        if (!strcpy(path_copy, name)) {
                return false;
        }
        small_name = basename(path_copy);

        if (snprintf(path_lookup, sizeof(path_lookup), "%s/%s", lib_dir, small_name) < 0) {
                return false;
        }

        /* Sod all we can do here */
        if (!lsi_file_exists(path_lookup)) {
                lsi_log_error("Missing DRI file: %s (%s)", name, path_lookup);
                return false;
        }

        *soname = path_lookup;
        lsi_log_debug("Enforcing snapd DRI links: \033[31;1m%s\033[0m -> \033[34;1m%s\033[0m",
                      name,
                      path_lookup);

        return true;
}

bool lsi_override_snapd_gl(const char *name, const char **soname)
{
        for (size_t i = 0; i < ARRAY_SIZE(libgl_source_table); i++) {
                const char *source = libgl_source_table[i];
                const char *target = NULL;

                if (strcmp(name, source) != 0) {
                        continue;
                }

                if (lsi_file_exists(libgl_target_table[i])) {
                        target = libgl_target_table[i];
                        lsi_log_debug(
                            "Enforcing hostfs snapd driver links: \033[31;1m%s\033[0m -> "
                            "\033[34;1m%s\033[0m",
                            name,
                            target);
                } else {
                        target = libgl_mesa_table[i];
                        lsi_log_debug(
                            "Enforcing Mesa snapd driver links: \033[31;1m%s\033[0m -> "
                            "\033[34;1m%s\033[0m",
                            name,
                            target);
                }
                *soname = target;
                return true;
        }

        /* We didn't get a match. Allow to continue */
        return false;
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
