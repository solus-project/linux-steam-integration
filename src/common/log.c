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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "nica/util.h"

static const char *_log_id = "__init__";

void lsi_log_set_id(const char *id)
{
        _log_id = id;
}

void lsi_log_debug(const char *format, ...)
{
        if (!getenv("LSI_DEBUG")) {
                return;
        }

        autofree(char) *p = NULL;
        va_list va;
        va_start(va, format);

        if (!vasprintf(&p, format, va)) {
                /* At least print *something */
                fprintf(stderr, "%s\n", format);
                goto end;
        }

        fprintf(stderr, "\033[32;1m[lsi:%s]\033[0m %s\n", _log_id, p);

end:
        va_end(va);
}

void lsi_log_info(const char *format, ...)
{
        autofree(char) *p = NULL;
        va_list va;
        va_start(va, format);

        if (!vasprintf(&p, format, va)) {
                /* At least print *something */
                fprintf(stderr, "%s\n", format);
                goto end;
        }

        fprintf(stderr, "\033[34;1m[lsi:%s]\033[0m %s\n", _log_id, p);

end:
        va_end(va);
}

void lsi_log_warn(const char *format, ...)
{
        autofree(char) *p = NULL;
        va_list va;
        va_start(va, format);

        if (!vasprintf(&p, format, va)) {
                /* At least print *something */
                fprintf(stderr, "%s\n", format);
                goto end;
        }

        fprintf(stderr, "\033[33;1m[lsi:%s]\033[0m %s\n", _log_id, p);

end:
        va_end(va);
}

void lsi_log_error(const char *format, ...)
{
        autofree(char) *p = NULL;
        va_list va;
        va_start(va, format);

        if (!vasprintf(&p, format, va)) {
                /* At least print *something */
                fprintf(stderr, "%s\n", format);
                goto end;
        }

        fprintf(stderr, "\033[31;1m[lsi:%s]\033[0m %s\n", _log_id, p);

end:
        va_end(va);
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
