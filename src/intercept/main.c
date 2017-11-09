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

#include <libgen.h>
#include <link.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../common/common.h"
#include "../common/files.h"
#include "../common/log.h"
#include "config.h"
#include "nica/util.h"

/**
 * What's the known process name?
 */
static const char *matched_process = NULL;

/**
 * We support a number of modes, but we mostly exist to make Steam behave
 */
typedef enum {
        INTERCEPT_MODE_NONE = 0,
        INTERCEPT_MODE_STEAM,
        INTERCEPT_MODE_VENDOR_OFFENDER,
} InterceptMode;

/**
 * by default, we're not "on"
 */
static InterceptMode work_mode = INTERCEPT_MODE_NONE;

/* Patterns we'll permit Steam to privately load */
static const char *steam_allowed[] = {
        /* general */
        "libicui18n.so",
        "libicuuc.so",
        "libavcodec.so.",
        "libavformat.so.",
        "libavresample.so.",
        "libavutil.so.",
        "libswscale.so.",
        "libx264.so.",

        /* core plugins */
        "chromehtml.so",
        "crashhandler.so",
        "filesystem_stdio.so",
        "friendsui.so",
        "gameoverlayrenderer.so",
        "gameoverlayui.so",
        "libaudio.so",
        "libmiles.so",
        "libopenvr_api.so",
        "liboverride.so",
        "libsteam.so",
        "libtier0_s.so",
        "libv8.so",
        "libvideo.so",
        "libvstdlib_s.so",
        "serverbrowser.so",
        "steamclient.so",
        "steamoverlayvulkanlayer.so",
        "steamservice.so",
        "steamui.so",
        "vgui2_s.so",

        /* big picture mode */
        "panorama",
        "libpangoft2-1.0.so",
        "libpango-1.0.so",

        /* steamwebhelper */
        "libcef.so",

        /* Swift shader */
        "libGLESv2.so",
};

static const char *wanted_steam_processes[] = {
        "html5app_steam",
        "opengl-program",
        "steam",
        "steamwebhelper",
};

/**
 * Vendor offendors should not be allowed to load replacements for libraries
 * that are KNOWN to cause issues, i.e. SDL + libstdc++
 */
static const char *vendor_blacklist[] = {
        /* base libraries being replaced will cause a C++ ABI issue when
         * loading the mesalib drivers. */
        "libgcc_",
        "libstdc++",
        "libSDL",

        /* vendor-owned */
        "libz.so.1",
        "libfreetype.so.6",
        "libmpg123.so.0",

        /* general problem causer. */
        "libopenal.so.",

        /* thou shalt not replace drivers (swiftshader) ! */
        "libGLESv2.so",
        "libGL.so",

        /* glews (provide glew + glew110 in your distro for full compat) */
        "libGLEW.so.1.10",
        "libGLEW.so.1.12",

        /* libglu has stable soname */
        "libGLU.so.",

        /* Security sensitive libraries should not be replaced */
        "libcurl.so.",

#ifdef LSI_OVERRIDE_LIBRESSL
        /* Sometimes libressl but this is handled separately. */
        "libcrypto.so.",
        "libssl.so.",
#else
        "libcrypto.so.1.0.0",
        "libssl.so.1.0.0",
#endif
        /* TODO/FUTURE:
        "libaudiofile.so.1", */
};

/**
 * Determine if we're in a given process set, i.e. the process name matches
 * the given table
 */
static bool is_in_process_set(char *process_name, const char **processes, size_t n_processes)
{
        /* Are we some portion of the process set? */
        for (size_t i = 0; i < n_processes; i++) {
                if (streq(processes[i], process_name)) {
                        work_mode = INTERCEPT_MODE_STEAM;
                        matched_process = processes[i];
                        lsi_log_debug("loading libintercept for '%s'", matched_process);
                        return true;
                }
        }
        return false;
}

/**
 * Find out if we're being executed by a process we actually need to override,
 * otherwise we'd not be loaded by rtld-audit
 */
static void check_is_intercept_candidate(void)
{
        autofree(char) *nom = NULL;

        nom = lsi_get_process_base_name();
        if (!nom) {
                return;
        }

        if (is_in_process_set(nom, wanted_steam_processes, ARRAY_SIZE(wanted_steam_processes))) {
                work_mode = INTERCEPT_MODE_STEAM;
        } else {
                work_mode = INTERCEPT_MODE_VENDOR_OFFENDER;
                matched_process = "vendor_offender";
        }
        lsi_log_set_id(matched_process);
}

/**
 * la_version is our main entry point and will only continue if our
 * process is explicitly Steam
 */
_nica_public_ unsigned int la_version(unsigned int supported_version)
{
        /* Unfortunately glibc will die if we tell it to skip us .. */
        check_is_intercept_candidate();
        return supported_version;
}

/**
 * lsi_search_steam handles whitelisting for the main Steam processes
 */
char *lsi_search_steam(const char *name)
{
        /* Find out if it exists */
        bool file_exists = lsi_file_exists(name);

        /* Find out if its a Steam private lib.. These are relative "./" files too! */
        if (name && (strstr(name, "/Steam/") || strncmp(name, "./", 2) == 0)) {
                for (size_t i = 0; i < ARRAY_SIZE(steam_allowed); i++) {
                        if (strstr(name, steam_allowed[i])) {
                                return (char *)name;
                        }
                }
                if (file_exists) {
                        lsi_log_debug("blacklisted loading of vendor library: \033[34;1m%s\033[0m",
                                      name);
                }

                return NULL;
        }

        return (char *)name;
}

/**
 * Source identifier patterns that indicate a soname replacement is happening.
 * Note that this is a basic pattern match for the soname and just allows us
 * to lookup the transmute target
 */
static const char *vendor_transmute_source[] = {
        /* Common */
        "libSDL2-2.0.",
        "libSDL2_image-2.0.",

        /* ".so" renames */
        "libSDL2_ttf.so",
        "libSDL2_image.so",
        "libSDL2_mixer.so",
        "libSDL2_net.so",
        "libSDL2_gfx.so",

/* libressl (security updates) */
#ifdef LSI_OVERRIDE_LIBRESSL
        "libcrypto.so.36",
        "libssl.so.37",
#endif

        /* old name for openal */
        "libopenal-soft.so.1",
};

/**
 * The transmute target is the intended replacement for any source name, assuming
 * that the soname doesn't identically match the currently requested soname.
 * This allows us to do on the fly replacements for re-soname'd libraries.
 */
static const char *vendor_transmute_target[] = {
        /* Common */
        "libSDL2-2.0.so.0",
        "libSDL2_image-2.0.so.0",

        /* ".so" renames */
        "libSDL2_ttf-2.0.so.0",
        "libSDL2_image-2.0.so.0",
        "libSDL2_mixer-2.0.so.0",
        "libSDL2_net-2.0.so.0",
        "libSDL2_gfx-1.0.so.0",

/* libressl (security updates) */
#if defined(LSI_LIBRESSL_MODE_SHIM)
        "libcrypto" LSI_LIBRESSL_SUFFIX ".so",
        "libssl" LSI_LIBRESSL_SUFFIX ".so",
#elif defined(LSI_LIBRESSL_MODE_NATIVE)
        "libcrypto.so.1.0.0",
        "libssl.so.1.0.0",
#endif

        /* new name for openal */
        "libopenal.so.1",
};

/**
 * Every so often a game comes along that does the following:
 *
 * open("path") ? dlopen("path").
 * Except: path = "*.dll", dlopen() is transformed to ".dll.so"
 *
 * This is unrelated to the ".la" errors
 */
static bool lsi_override_dll_fail(const char *orig_name, const char **soname)
{
        size_t len = strlen(orig_name);
        static char path_lookup[PATH_MAX];

        if (len < 7) {
                return false;
        }

        if (strncmp(orig_name + (len - 7), ".dll.so", 7) != 0) {
                return false;
        }

        if (!strncpy(path_lookup, orig_name, len - 3)) {
                return false;
        }

        if (!lsi_file_exists(path_lookup)) {
                return false;
        }

        *soname = path_lookup;
        lsi_log_debug("fixed invalid suffix dlopen() \033[31;1m%s\033[0m -> \033[34;1m%s\033[0m",
                      orig_name,
                      path_lookup);
        return true;
}

/**
 * Mono games might try looking for /x86/ plugin directory for a 64-bit process,
 * as seen with testing with Project Highrise.
 *
 * This function simply redirects the dlopen to the file in the x86_64 directory,
 * if it actually exists.
 *
 * Typically this is for libCSteamWorks, libsteam_api, and Unity ScreenSelector.so
 */
#if UINTPTR_MAX == 0xffffffffffffffff
static bool lsi_override_x86_derp(const char *orig_name, const char **soname)
{
        static char path_copy[PATH_MAX];
        static char path_lookup[PATH_MAX];
        char *small_name = NULL;
        char *dir = NULL;

        if (!(strstr(orig_name, "/Plugins/x86/") && strstr(orig_name, ".so"))) {
                return false;
        }

        if (!strcpy(path_copy, orig_name)) {
                return false;
        }

        small_name = basename(path_copy);
        dir = dirname(path_copy);

        if (snprintf(path_lookup, sizeof(path_lookup), "%s/../x86_64/%s", dir, small_name) < 0) {
                return false;
        }

        if (!lsi_file_exists(path_lookup)) {
                return false;
        }

        *soname = path_lookup;
        lsi_log_debug(
            "fixed invalid architecture dlopen() \033[31;1m%s\033[0m -> \033[34;1m%s\033[0m",
            orig_name,
            path_lookup);
        return true;
#else
static bool lsi_override_x86_derp(__lsi_unused__ const char *orig_name,
                                  __lsi_unused__ const char **soname)
{
        /* Don't perform x86 translation on 32-bit machines */
        return false;
#endif
}

/**
 * Internal helper for path replacement to host lib
 */
static bool lsi_override_replace_with_host(const char *orig_name, const char **soname,
                                           const char *msg)
{
        static char path_lookup[PATH_MAX];
        static char path_copy[PATH_MAX];
        char *small_name = NULL;
        static const char *library_paths[] = {
#if UINTPTR_MAX == 0xffffffffffffffff
                "/usr/lib64",
                "/usr/lib/x86_64-linux-gnu",
                "/usr/lib",
#else
                "/usr/lib32",
                "/usr/lib/i386-linux-gnu",
                "/usr/lib",
#endif
        };

        if (!strcpy(path_copy, orig_name)) {
                return false;
        }

        small_name = basename(path_copy);

        /* Iterate all of the library paths for this process architecture and try
         * to find a system variant of the library instead of allowing the process
         * to dlopen() the vendored version.
         */
        for (size_t i = 0; i < ARRAY_SIZE(library_paths); i++) {
                if (snprintf(path_lookup,
                             sizeof(path_lookup),
                             "%s/%s",
                             library_paths[i],
                             small_name) < 0) {
                        return false;
                }
                if (!lsi_file_exists(path_lookup)) {
                        continue;
                }

                /* We hit a match but it was identical to our expectation */
                if (strcmp(path_lookup, orig_name) == 0) {
                        return false;
                }

                *soname = path_lookup;
                lsi_log_debug("%s \033[31;1m%s\033[0m -> \033[34;1m%s\033[0m",
                              msg,
                              orig_name,
                              path_lookup);
                return true;
        }

        return false;
}

/**
 * lsi_override_dlopen is used to override simple dlopen() requests typically
 * used by Mono games, i.e.:
 *
 * <dllmap dll="SDL2.dll" os="linux" cpu="x86-64" target="./lib64/libSDL2-2.0.so.0"/>
 *
 * We'll attempt to do a trivial lookup for "/usr/./lib64/libSDL2-2.0.so.0 in this
 * case.
 */
static bool lsi_override_dlopen(const char *orig_name, const char **soname)
{
        if (lsi_override_dll_fail(orig_name, soname)) {
                return true;
        }

        if (!lsi_file_exists(orig_name)) {
                return false;
        }

        if (lsi_override_x86_derp(orig_name, soname)) {
                return true;
        }

        return lsi_override_replace_with_host(orig_name, soname, "intercepting vendor dlopen()");
}

/**
 * lsi_override_soname will deal with LA_SER_ORIG entries, i.e. the original
 * soname request when the linker tries to load a library.
 *
 * Certain Steam games (notably Feral Interactive ports) use their own vendored
 * SDL libraries with changed sonames, which are implicitly blacklisted by the
 * lsi_blacklist_vendor function.
 *
 * As such, we intercept those renamed libraries, and convert their names back
 * to the ABI stable system libraries on the fly.
 */
static bool lsi_override_soname(unsigned int flag, const char *orig_name, const char **soname)
{
        *soname = NULL;

        /* We only need to deal with LA_SER_ORIG */
        if ((flag & LA_SER_ORIG) != LA_SER_ORIG) {
                return false;
        }

        /* Don't transform dlopen */
        if (strstr(orig_name, "/")) {
                return lsi_override_dlopen(orig_name, soname);
        }

        for (size_t i = 0; i < ARRAY_SIZE(vendor_transmute_source); i++) {
                if (!strstr(orig_name, vendor_transmute_source[i])) {
                        continue;
                }
                /* Ensure we're not just replacing the same thing here as the
                 * string would be identical, no real replacement would happen,
                 * and ld will be confused about memory and die.
                 */
                if (streq(orig_name, vendor_transmute_target[i])) {
                        continue;
                }
                *soname = vendor_transmute_target[i];
                lsi_log_debug(
                    "transforming vendor soname: \033[31;1m%s\033[0m -> \033[34;1m%s\033[0m",
                    orig_name,
                    *soname);

                return true;
        }

        return false;
}

/**
 * If the library exists locally, and we're attempting to load this from a
 * relative location, strongly attempt to actually use the system-version instead.
 *
 * Thus, if our CWD is our LD_LIBRARY_PATH and contains "libfreetype.so.6", we'll
 * try to use the host version of the library if that exists, instead of relying
 * on the locally vendored, potentially insecure/buggy version.
 */
static bool lsi_override_local(unsigned int flag, const char *orig_name, const char **soname)
{
        *soname = NULL;

        /* We only need to deal with LA_SER_ORIG */
        if ((flag & LA_SER_ORIG) != LA_SER_ORIG) {
                return false;
        }

        /* We only care about relative paths */
        if (strstr(orig_name, "/")) {
                return false;
        }

        /* We also only care about relative paths */
        if (!lsi_file_exists(orig_name)) {
                return false;
        }

        /* Resolve all paths back to the real library path version if they exist */
        for (size_t i = 0; i < ARRAY_SIZE(vendor_blacklist); i++) {
                if (!strstr(orig_name, vendor_blacklist[i])) {
                        continue;
                }
                return lsi_override_replace_with_host(orig_name,
                                                      soname,
                                                      "forcing use of host library");
        }

        return false;
}

char *lsi_blacklist_vendor(unsigned int flag, const char *name)
{
        /* Find out if it exists */
        bool file_exists = lsi_file_exists(name);
        const char *override_soname = NULL;

        /* Find out if we have to rename some libraries on the fly */
        if (lsi_override_soname(flag, name, &override_soname)) {
                return (char *)override_soname;
        }

        /* Locally exists due to directory foobar */
        if (lsi_override_local(flag, name, &override_soname)) {
                return (char *)override_soname;
        }

        /* Find out if its a Steam private lib.. These are relative "./" files too! */
        if (name && (strstr(name, "/Steam/") || strncmp(name, "./", 2) == 0)) {
                for (size_t i = 0; i < ARRAY_SIZE(vendor_blacklist); i++) {
                        if (!strstr(name, vendor_blacklist[i])) {
                                continue;
                        }

                        if (file_exists) {
                                lsi_log_debug(
                                    "blacklisted loading of vendor library: \033[34;1m%s\033[0m",
                                    name);
                        }
                        return NULL;
                }
                /* Allowed to exist */
                return (char *)name;
        }

        return (char *)name;
}

/**
 * la_objsearch will allow us to blacklist certain LD_LIBRARY_PATH duplicate
 * libraries being loaded by the Steam client, such as the broken libSDL shipped
 * as a private vendored lib
 */
_nica_public_ char *la_objsearch(const char *name, __lsi_unused__ uintptr_t *cookie,
                                 unsigned int flag)
{
#ifdef HAVE_SNAPD_SUPPORT
        /*
         * Super special case. If we're a snap, and the file being requested is
         * actually within the private snapd trees, we must unconditionally permit
         * that file, as it is most likely a driver
         */
        if (strstr(name, "/var/lib/snapd/gl") || strstr(name, "/var/lib/snapd/hostfs")) {
                lsi_log_debug("skipping snapd file: %s", name);
                return name;
        }
        lsi_log_debug("snapd debug: %s", name);
#endif
        switch (work_mode) {
        case INTERCEPT_MODE_STEAM:
                return lsi_search_steam(name);
        case INTERCEPT_MODE_VENDOR_OFFENDER:
                return lsi_blacklist_vendor(flag, name);
        case INTERCEPT_MODE_NONE:
        default:
                return (char *)name;
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
