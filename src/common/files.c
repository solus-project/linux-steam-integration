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

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "files.h"
#include "nica/util.h"

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
        const char *home = lsi_get_home_dir();
        char *c = NULL;
        if (asprintf(&c, "%s/.config", home) < 0) {
                return NULL;
        }
        return c;
}

/**
 * Just use .local/share/Steam at this point..
 */
static inline char *lsi_get_fallback_steam_dir(const char *home)
{
        char *p = NULL;
        if (asprintf(&p, "%s/.local/share/Steam", home) < 0) {
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
        autofree(char) *realp = NULL;
        char *basep = NULL;

        realp = realpath("/proc/self/exe", NULL);
        if (!realp) {
                return false;
        }

        basep = basename(realp);
        return strdup(basep);
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
