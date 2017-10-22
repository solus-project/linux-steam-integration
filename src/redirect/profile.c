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

#include "../common/log.h"
#include "redirect.h"

LsiRedirectProfile *lsi_redirect_profile_new(const char *name)
{
        LsiRedirectProfile p = { .name = strdup(name), { 0 } };
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

void lsi_redirect_profile_insert_rule(LsiRedirectProfile *self, LsiRedirect *redirect)
{
        LsiRedirectOperation op;

        switch (redirect->type) {
        case LSI_REDIRECT_PATH:
                op = LSI_OPERATION_OPEN;
                break;
        default:
                lsi_log_error("Attempted insert of unknown rule into '%s'", self->name);
                break;
        }

        /* Set head or prepend the rule */
        if (self->op_table[op]) {
                self->op_table[op] = redirect->next = self->op_table[op];
        } else {
                self->op_table[op] = redirect;
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
