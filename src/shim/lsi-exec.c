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

#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lsi.h"
#include "shim.h"

int main(int argc, char **argv)
{
        const char *command = NULL;

        if (!shim_bootstrap()) {
                return EXIT_FAILURE;
        }

        /* Drop our own binary from the passed in options */
        --argc;
        ++argv;

        if (argc < 1) {
                lsi_report_failure("lsi-exec was not passed a valid command");
                return EXIT_FAILURE;
        }

        command = argv[0];
        --argc;
        ++argv;

        /* Now execute. */
        return shim_execute(command, argc, argv);
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
