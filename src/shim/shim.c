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
#include <glob.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../common/files.h"
#include "../common/log.h"
#include "../nica/files.h"
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
 * Potential for the host Vulkan ICD files (old vs new)
 */
#define VK_GLOB "/var/lib/snapd/lib/gl/*nvidia*.json"
#define VK_GLOB_2 "/var/lib/snapd/lib/vulkan/*nvidia*.json"

/**
 * Used to update a value in the environment, and perform a prepend if the variable
 * is already set.
 */
static void shim_export_merge_vars(const char *var_name, const char *prefix, const char *value)
{
        static char copy_buffer[PATH_MAX] = { 0 };
        const char *env_exist = NULL;
        int ret = 0;

        env_exist = getenv(var_name);

        ret = snprintf(copy_buffer,
                       sizeof(copy_buffer),
                       "%s%s%s%s",
                       prefix ? prefix : "",
                       value,
                       env_exist ? ":" : "",
                       env_exist ? env_exist : "");
        if (ret < 0) {
                lsi_log_error("failed to update variable '%s'", var_name);
                return;
        }

        lsi_log_debug("%s = %s", var_name, copy_buffer);

        setenv(var_name, copy_buffer, 1);
}

/**
 * Set up the LD_AUDIT environment - respecting $SNAP if set
 */
static void shim_set_audit_path(const char *prefix)
{
        shim_export_merge_vars("LD_AUDIT", prefix, AUDIT_PATH);
}

/**
 * Set up LD_PRELOAD, respecting an existing LD_PRELOAD and forcing ourselves
 * to be first in the list.
 */
static void shim_set_ld_preload(const char *prefix)
{
        shim_export_merge_vars("LD_PRELOAD", prefix, REDIRECT_PATH);
}

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

#ifdef HAVE_SNAPD_SUPPORT
/**
 * This function is only used during our initial bootstrap phase to ensure
 * we're able to set up the environment and directories correctly under the
 * snapd system.
 */
static void shim_init_user(const char *userdir)
{
        static const char *paths[] = {
                ".local/share",
                ".config",
                ".cache",
        };
        static const char *vars[] = {
                "XDG_DATA_HOME",
                "XDG_CONFIG_HOME",
                "XDG_CACHE_HOME",
        };
        static char tgt[PATH_MAX] = { 0 };

        for (size_t i = 0; i < ARRAY_SIZE(paths); i++) {
                if (snprintf(tgt, sizeof(tgt), "%s/%s", userdir, paths[i]) < 0) {
                        lsi_log_error("memory failure");
                        return;
                }
                if (!lsi_file_exists(tgt)) {
                        if (!nc_mkdir_p(tgt, 00755)) {
                                lsi_log_error("failed to construct %s: %s", tgt, strerror(errno));
                                goto write_var;
                        }
                        lsi_log_debug("Constructing %s: %s", vars[i], tgt);
                }
        write_var:
                setenv(vars[i], tgt, 1);
        }
}

/**
 * Attempt to set up Vulkan in the environment by looking for the ICD files
 * passed from the hostfs
 */
static bool shim_init_vulkan(const char *glob_path)
{
        glob_t glo = { 0 };

        glob(glob_path, GLOB_DOOFFS, NULL, &glo);

        if (glo.gl_pathc < 1) {
                globfree(&glo);
                return false;
        }

        /* Preserve glob order and insert environment in reverse */
        for (int i = glo.gl_pathc - 1; i >= 0; i--) {
                shim_export_merge_vars("VK_ICD_FILENAMES", NULL, glo.gl_pathv[i]);
        }

        globfree(&glo);

        return true;
}
#endif

#ifdef HAVE_SNAPD_SUPPORT

/**
 * Attempt to push a path into the variable name if it actually exists
 */
static void shim_export_ld_dir(const char *var_name, const char *dir)
{
        if (!lsi_file_exists(dir)) {
                return;
        }
        shim_export_merge_vars(var_name, NULL, dir);
}

/**
 * Set up any extra environment pieces that might need fixing
 *
 * Currently this only sets up the snapd environmental variables, so that
 * we don't rely on separate bootstrap scripts out of tree.
 */
static void shim_export_extra(const char *prefix)
{
        static const char *snap_user = NULL;
        static const char *xdg_home = NULL;

        /* Add all of these guys to LD_LIBRARY_PATH if they exist.
         * This allows us to handle the special-case multiarch mounts.
         */
        static const char *ld_library_dirs[] = {
                "/var/lib/snapd/lib/gl/vdpau",     /**<64-bit vdpau */
                "/var/lib/snapd/lib/gl32/vdpau",   /**<32bit vdpau */
                "/usr/lib/glx-provider/default",   /**<Solus mesa, 64-bit */
                "/usr/lib32/glx-provider/default", /**<Solus mesa, 32bit */
                "/var/lib/snapd/lib/gl",           /**<Potential host NVIDIA libraries */
                "/var/lib/snapd/lib/gl32", /**<Potential host NVIDIA 32-bit libraries (new location) */
        };

        /* Add any necessary DRI drivers to the path, though in reality this
         * shouldn't REALLY be needed, but let's just play it safe.
         */
        static const char *dri_drivers_extra[] = {
                "/usr/lib32/dri",
                "/usr/lib/dri",
        };

        unsetenv("LIBGL_DRIVERS_PATH");
        unsetenv("LD_LIBRARY_PATH");

        /* Include all GLI driver paths */
        for (size_t i = 0; i < ARRAY_SIZE(dri_drivers_extra); i++) {
                shim_export_ld_dir("LIBGL_DRIVERS_PATH", dri_drivers_extra[i]);
                shim_export_ld_dir("LD_LIBRARY_PATH", dri_drivers_extra[i]);
        }

        /* Now set the library path accordingly */
        for (size_t i = 0; i < ARRAY_SIZE(ld_library_dirs); i++) {
                shim_export_ld_dir("LD_LIBRARY_PATH", ld_library_dirs[i]);
        }

        /* the vdpau directory only exists on multiarch */
        shim_export_ld_dir("VDPAU_DRIVER_PATH", "/var/lib/snapd/lib/gl/vdpau");
        shim_export_ld_dir("VDPAU_DRIVER_PATH", "/var/lib/snapd/lib/gl");
        shim_export_ld_dir("VDPAU_DRIVER_PATH", "/usr/lib/vdpau");

        /* Path */
        shim_export_merge_vars("PATH", prefix, "/usr/bin");
        shim_export_merge_vars("PATH", prefix, "/bin");

        /* Try both known Vulkan ICD paths */
        if (!shim_init_vulkan(VK_GLOB_2)) {
                shim_init_vulkan(VK_GLOB);
        }

        /* XDG */
        shim_export_merge_vars("XDG_CONFIG_DIRS", NULL, "/etc/xdg");
        shim_export_merge_vars("XDG_CONFIG_DIRS", NULL, "/usr/share/xdg");
        shim_export_merge_vars("XDG_CONFIG_DIRS", prefix, "/etc/xdg");
        shim_export_merge_vars("XDG_CONFIG_DIRS", prefix, "/usr/xdg");

        shim_export_merge_vars("XDG_DATA_DIRS", prefix, "/usr/share");

        snap_user = getenv("SNAP_USER_COMMON");
        shim_export_merge_vars("XDG_DATA_DIRS", NULL, "/usr/share");
        shim_export_merge_vars("XDG_DATA_DIRS", prefix, "/usr/share");
        if (snap_user) {
                shim_export_merge_vars("XDG_DATA_DIRS", NULL, snap_user);
                shim_init_user(snap_user);
        }

        /* Ensure XDG_RUNTIME_DIR really exists */
        xdg_home = getenv("XDG_RUNTIME_DIR");
        if (!xdg_home) {
                return;
        }
        if (lsi_file_exists(xdg_home)) {
                return;
        }
        if (!nc_mkdir_p(xdg_home, 00755)) {
                lsi_log_error("Failed to setup XDG_RUNTIME_DIR %s: %s", xdg_home, strerror(errno));
                return;
        }
        lsi_log_debug("Constructed XDG_RUNTIME_DIR: %s", xdg_home);
}
#else
static void shim_export_extra(__lsi_unused__ const char *prefix)
{
}
#endif

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
        const char *operation_prefix = NULL;

#ifdef HAVE_SNAPD_SUPPORT
        operation_prefix = getenv("SNAP");
#endif

        lsi_exec_bin = shim_get_steam_binary(operation_prefix);

        /* Initialise config */
        if (!lsi_config_load(&config)) {
                lsi_config_load_defaults(&config);
        }

        is_x86_64 = lsi_system_is_64bit();

        if (!lsi_file_exists(lsi_exec_bin)) {
                lsi_report_failure("Steam isn't currently installed at %s", lsi_exec_bin);
                return EXIT_FAILURE;
        }

        /* We might have additional variables we need to export */
        shim_export_extra(operation_prefix);

        /* Force STEAM_RUNTIME into the environment */
        if (config.use_native_runtime) {
                /* Explicitly disable the runtime */
                setenv("STEAM_RUNTIME", "0", 1);
#ifdef HAVE_LIBINTERCEPT
                /* Only use libintercept in combination with native runtime! */
                if (config.use_libintercept) {
                        shim_set_audit_path(operation_prefix);
                }
#endif
#ifdef HAVE_LIBREDIRECT
                /* Only use libredirect in combination with native runtime! */
                if (config.use_libredirect) {
                        shim_set_ld_preload(operation_prefix);
                }
#endif
        } else {
                /* Only preload when needed. */
                if (lsi_system_requires_preload()) {
                        shim_export_merge_vars("LD_PRELOAD", operation_prefix, lsi_preload_list());
                }
                setenv("STEAM_RUNTIME", "1", 1);
        }

        /* Vanilla dbus users suffer a segfault on Steam exit, due to incorrect
         * usage of dbus by Steam. Help them out */
        setenv("DBUS_FATAL_WARNINGS", "0", 1);

        /* Requires Solus patch to actually work */
        setenv("DBUS_SILENCE_WARNINGS", "1", 1);

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
