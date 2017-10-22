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

        /* Free all chains */
        for (unsigned int i = 0; i < LSI_NUM_OPERATIONS; i++) {
                LsiRedirect *r = self->op_table[i];
                if (!r) {
                        continue;
                }
                lsi_redirect_free(r);
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

LsiRedirect *lsi_redirect_new_path_replacement(const char *source_path, const char *target_path)
{
        LsiRedirect *ret = NULL;

        ret = calloc(1, sizeof(LsiRedirect));
        if (!ret) {
                return NULL;
        }
        ret->type = LSI_REDIRECT_PATH;
        ret->path_source = strdup(source_path);
        ret->path_target = strdup(target_path);

        if (!ret->path_source || !ret->path_target) {
                lsi_redirect_free(ret);
                return NULL;
        }
        return ret;
}

void lsi_redirect_free(LsiRedirect *self)
{
        if (!self) {
                return;
        }

        /* Pass it along */
        if (self->next) {
                lsi_redirect_free(self->next);
        }

        switch (self->type) {
        case LSI_REDIRECT_PATH:
        default:
                free(self->path_source);
                free(self->path_target);
                break;
        }

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
