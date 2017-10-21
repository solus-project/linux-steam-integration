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
#include <unistd.h>

#include "files.h"

const char *lsi_get_home_dir(void)
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

char *lsi_get_user_config_dir(void)
{
        const char *home = lsi_get_home_dir();
        char *c = NULL;
        if (asprintf(&c, "%s/.config", home) < 0) {
                return NULL;
        }
        return c;
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
