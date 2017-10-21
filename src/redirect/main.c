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

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nica/util.h"

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

__attribute__((constructor)) static void lsi_redirect_init(void)
{
        autofree(char) *process_name = get_process_name();
        if (!process_name) {
                fprintf(stderr, "Failure to allocate memory! %s\n", strerror(errno));
                return;
        }
        fprintf(stderr, "Loading lsi_redirect for: %s\n", process_name);
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
