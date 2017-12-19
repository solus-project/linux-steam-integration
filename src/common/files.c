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

#include <ctype.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "files.h"
#include "log.h"
#include "nica/array.h"
#include "nica/util.h"
#include "vdf.h"

DEF_AUTOFREE(VdfFile, vdf_file_close)

bool lsi_file_exists(const char *path)
{
        __lsi_unused__ struct stat st = { 0 };
        return lstat(path, &st) == 0;
}

const char *lsi_get_home_dir()
{
        char *c = NULL;
        struct passwd *p = NULL;

        c = getenv("HOME");
        if (c) {
                return c;
        }
        p = getpwuid(getuid());
        if (!p) {
                return NULL;
        }
        return p->pw_dir;
}

char *lsi_get_user_config_dir()
{
        const char *home = NULL;
        char *c = NULL;
        char *xdg_config = getenv("XDG_CONFIG_HOME");
        char *p = NULL;

        /* Respect the XDG_CONFIG_HOME variable if it is set */
        if (xdg_config) {
                p = realpath(xdg_config, NULL);
                if (p) {
                        return p;
                }
                return strdup(xdg_config);
        }

        home = lsi_get_home_dir();
        if (asprintf(&c, "%s/.config", home) < 0) {
                return NULL;
        }
        p = realpath(c, NULL);
        if (p) {
                free(c);
                return p;
        }
        return c;
}

/**
 * Just use .local/share/Steam at this point..
 */
static inline char *lsi_get_fallback_steam_dir(const char *home)
{
        char *p = NULL;
        char *xdg_data = getenv("XDG_DATA_HOME");
        int r = 0;

        /* Respect the XDG_CONFIG_HOME variable if it is set */
        if (xdg_data) {
                r = asprintf(&p, "%s/Steam", xdg_data);
        } else {
                r = asprintf(&p, "%s/.local/share/Steam", home);
        }

        if (r < 0) {
                return NULL;
        }
        return p;
}

char *lsi_get_steam_dir()
{
        const char *homedir = NULL;
        autofree(char) *candidate = NULL;
        autofree(char) *resolv = NULL;

        homedir = lsi_get_home_dir();
        if (asprintf(&candidate, "%s/.steam/root", homedir) < 0) {
                return NULL;
        }

        resolv = realpath(candidate, NULL);
        if (!resolv || !lsi_file_exists(resolv)) {
                return lsi_get_fallback_steam_dir(homedir);
        }
        return strdup(resolv);
}

char *lsi_get_process_name(void)
{
        char *realp = NULL;

        realp = realpath("/proc/self/exe", NULL);
        if (!realp) {
                return NULL;
        }
        return realp;
}

char *lsi_get_process_base_name(void)
{
        autofree(char) *p = NULL;
        char *basep = NULL;

        p = lsi_get_process_name();
        if (!p) {
                return NULL;
        }

        basep = basename(p);
        return strdup(basep);
}

/**
 * Quickly determine if a string is numeric..
 */
static inline bool lsi_is_string_numeric(char *p)
{
        char *s;

        for (s = p; *s; s++) {
                if (!isdigit(*s)) {
                        return false;
                }
        }

        /* Didn't wind, empty string */
        if (s == p) {
                return false;
        }
        return true;
}

char **lsi_get_steam_paths(void)
{
        autofree(char) *steam_root = NULL;
        autofree(char) *lib_conf = NULL;
        autofree(VdfFile) *vdf = NULL;
        VdfNode *root = NULL;
        VdfNode *node = NULL;
        NcArray *array = NULL;

        steam_root = lsi_get_steam_dir();
        if (!steam_root) {
                return NULL;
        }

        array = nc_array_new();
        if (!nc_array_add(array, strdup(steam_root))) {
                goto bail;
        }

        if (asprintf(&lib_conf, "%s/steamapps/libraryfolders.vdf", steam_root) < 0) {
                return NULL;
        }

        vdf = vdf_file_open(lib_conf);
        if (!vdf) {
                goto fallback;
        }

        if (!vdf_file_parse(vdf)) {
                goto fallback;
        }

        /* Look up LibraryFolders under root */
        root = vdf_file_get_root(vdf);
        node = vdf_node_get(root, "LibraryFolders", NULL);
        if (!node) {
                goto fallback;
        }

        /* Walk all nodes immediately within LibraryFolders and ignore sections */
        for (VdfNode *n = node->child; n; n = n->sibling) {
                if (!n->value) {
                        continue;
                }
                if (lsi_is_string_numeric(n->key)) {
                        if (!nc_array_add(array, strdup(n->value))) {
                                goto bail;
                        }
                        lsi_log_debug("vdf: discovered LibraryFolders: %s", n->value);
                }
        }

fallback:
        /* Trick nica into storing empty value to so we can terminate the array */
        if (!nc_array_add(array, (void *)(long)1)) {
                goto bail;
        }

        /* Steal the blob straight out of the array */
        char **data = (char **)array->data;
        data[array->len - 1] = NULL;
        free(array);
        return data;

bail:
        nc_array_free(&array, free);
        return NULL;
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
