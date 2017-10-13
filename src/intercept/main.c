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

#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../common.h"
#include "nica/util.h"

static inline bool lsi_file_exists(const char *path)
{
        struct stat st = { .st_ino = 0 };
        return (lstat(path, &st) == 0);
}

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

        /* general problem causer. */
        "libopenal.so.",

        /* thou shalt not replace drivers (swiftshader) ! */
        "libGLESv2.so",
        "libGL.so",

        /* glews (provide glew + glew110 in your distro for full compat) */
        "libGLEW.so.",

        /* libglu has stable soname */
        "libGLU.so.",

        /* Nuh uh. Sorry, Feral games. libcurl MUST be up to date for security! */
        "libcurl.so.",
};

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
                        if (!getenv("LSI_DEBUG")) {
                                return true;
                        }
                        fprintf(stderr,
                                "\033[32;1m[LSI:%s]\033[0m: debug: loading libintercept from LSI\n",
                                matched_process);
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

        nom = get_process_name();
        if (!nom) {
                return;
        }

        if (is_in_process_set(nom, wanted_steam_processes, ARRAY_SIZE(wanted_steam_processes))) {
                work_mode = INTERCEPT_MODE_STEAM;
        } else {
                work_mode = INTERCEPT_MODE_VENDOR_OFFENDER;
                matched_process = "vendor_offender";
        }
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
 * emit_overriden lib is used to pretty-print the debug when we blacklist
 * a vendored library
 */
static void emit_overriden_lib(const char *lib_name)
{
        fprintf(stderr,
                "\033[32;1m[LSI:%s]\033[0m: debug: blacklisted loading of vendored library: "
                "\033[34;1m%s\033[0m\n",
                matched_process,
                lib_name);
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
                /* If LSI_DEBUG is set, spam it. */
                if (getenv("LSI_DEBUG") && file_exists) {
                        emit_overriden_lib(name);
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
        "libSDL2-2.0.",
        "libSDL2_image-2.0.",
};

/**
 * The transmute target is the intended replacement for any source name, assuming
 * that the soname doesn't identically match the currently requested soname.
 * This allows us to do on the fly replacements for re-soname'd libraries.
 */
static const char *vendor_transmute_target[] = {
        "libSDL2-2.0.so.0",
        "libSDL2_image-2.0.so.0",
};

/**
 * emit_replaced_lib is used to pretty-print the debug when we blacklist and
 * transform a soname to a system name
 */
static void emit_replaced_name(const char *lib_name, const char *new_name)
{
        fprintf(stderr,
                "\033[32;1m[LSI:%s]\033[0m: debug: transforming vendor soname: "
                "\033[31;1m%s\033[0m -> \033[34;1m%s\033[0m\n",
                matched_process,
                lib_name,
                new_name);
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

                /* If LSI_DEBUG is set, spam it. */
                if (getenv("LSI_DEBUG")) {
                        emit_replaced_name(orig_name, *soname);
                }

                return true;
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

        /* Find out if its a Steam private lib.. These are relative "./" files too! */
        if (name && (strstr(name, "/Steam/") || strncmp(name, "./", 2) == 0)) {
                for (size_t i = 0; i < ARRAY_SIZE(vendor_blacklist); i++) {
                        if (!strstr(name, vendor_blacklist[i])) {
                                continue;
                        }

                        /* If LSI_DEBUG is set, spam it. */
                        if (getenv("LSI_DEBUG") && file_exists) {
                                emit_overriden_lib(name);
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
