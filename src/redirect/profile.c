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

#include <stdlib.h>
#include <string.h>

#include "redirect.h"

LsiRedirectProfile *lsi_redirect_profile_new(const char *name)
{
        LsiRedirectProfile p = {
                .name = strdup(name),
                .op_table = {{ 0 }},
        };
        LsiRedirectProfile *ret = NULL;
        if (!p.name) {
                return NULL;
        }

        ret = calloc(1, sizeof(LsiRedirectProfile));
        if (!ret) {
                return NULL;
        }
        *ret = p;
        return ret;
}

void lsi_redirect_profile_free(LsiRedirectProfile *self)
{
        if (!self) {
                return;
        }
        free(self->name);
        free(self);
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
