/*
 * This file is part of linux-steam-integration.
 *
 * Copyright © 2016 Ikey Doherty
 *
 * linux-steam-integration is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#define _GNU_SOURCE

#include <assert.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "lsi.h"
#include "nica/hashmap.h"
#include "nica/inifile.h"

/**
 * Configuration file suffix, i.e. ~/.config/linux-steam-integration.conf
 */
#define LSI_CONFIG_FILE "linux-steam-integration.conf"

#define LSI_SYSTEM_CONFIG_FILE SYSTEMCONFDIR "/linux-steam-integration.conf"

#define LSI_VENDOR_CONFIG_FILE VENDORDIR "/linux-steam-integration.conf"

/**
 * Determine the home directory
 */
static const char *lsi_get_home_dir(void)
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

/**
 * Determine the home config directory
 */
static char *lsi_get_user_config_dir(void)
{
        const char *home = lsi_get_home_dir();
        char *c = NULL;
        if (asprintf(&c, "%s/.config", home) < 0) {
                return NULL;
        }
        return c;
}

/**
 * Build the user config file path
 */
static char *lsi_get_user_config_file(void)
{
        autofree(char) *dir = lsi_get_user_config_dir();
        char *c = NULL;
        if (asprintf(&c, "%s/%s", dir, LSI_CONFIG_FILE) < 0) {
                return NULL;
        }
        return c;
}

/**
 * Quick helper to determine if the path exists
 */
__lsi_inline__ static inline bool lsi_file_exists(const char *path)
{
        __lsi_unused__ struct stat st = { 0 };
        return lstat(path, &st) == 0;
}

/**
 * A subset of values equate to a true boolean, everything else is false.
 */
static inline bool lsi_is_boolean_true(const char *compare)
{
        const char *yesses[] = { "yes", "true", "YES", "TRUE", "ON", "on" };
        if (!compare) {
                return false;
        }
        for (size_t i = 0; i < ARRAY_SIZE(yesses); i++) {
                if (streq(compare, yesses[i])) {
                        return true;
                }
        }
        return false;
}

bool lsi_config_load(LsiConfig *config)
{
        char *paths[] = { NULL, LSI_SYSTEM_CONFIG_FILE, LSI_VENDOR_CONFIG_FILE };

        autofree(NcHashmap) *mconfig = NULL;
        char *map_val = NULL;

        paths[0] = lsi_get_user_config_file();
        for (size_t i = 0; i < ARRAY_SIZE(paths); i++) {
                const char *filepath = (const char *)paths[i];
                if (!filepath) {
                        continue;
                }
                /* Don't bother loading non existant paths */
                if (!lsi_file_exists(filepath)) {
                        continue;
                }
                mconfig = nc_ini_file_parse(filepath);
                if (mconfig) {
                        break;
                }
        }
        free((char *)paths[0]);

        /* Found no valid INI config */
        if (!mconfig) {
                return false;
        }

        /* Populate defaults now */
        lsi_config_load_defaults(config);

        /* Determine if native runtime should be used */
        map_val = nc_hashmap_get(nc_hashmap_get(mconfig, "Steam"), "use-native-runtime");
        if (map_val) {
                config->use_native_runtime = lsi_is_boolean_true(map_val);
                map_val = NULL;
        }

        /* Check if 32-bit is being forced */
        map_val = nc_hashmap_get(nc_hashmap_get(mconfig, "Steam"), "force-32bit");
        if (map_val) {
                config->force_32 = lsi_is_boolean_true(map_val);
        }
        return true;
}

bool lsi_config_store(__lsi_unused__ LsiConfig *config)
{
        fputs("lsi_config_store(): Not yet implemented\n", stderr);
        return false;
}

void lsi_config_load_defaults(LsiConfig *config)
{
        assert(config != NULL);

        /* Very simple right now, but in future we'll expand the options and
         * things that LSI knows about */
        config->force_32 = false;
        config->use_native_runtime = true;
}

bool lsi_system_requires_preload()
{
#ifdef LSI_USE_NEW_ABI
        /* Right now we only return preload when using the new C++ ABI. In
         * future we'll expand this to return false when using the nvidia
         * proprietary driver
         */
        return true;
#endif
        return false;
}

const char *lsi_preload_list()
{
        return "/usr/$LIB/libX11.so.6:/usr/$LIB/libstdc++.so.6";
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
