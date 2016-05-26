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

#include <stdio.h>
#include <stdlib.h>

#include "lsi.h"

int main(__lsi_unused__ int argc, __lsi_unused__ char **argv)
{
        LsiConfig config = { 0 };
        bool is_x86_64;

        if (!lsi_config_load(&config)) {
                lsi_config_load_defaults(&config);
        }

        is_x86_64 = lsi_system_is_64bit();

        /* Debug printers */
        fprintf(stderr, "emul32? %s\n", config.force_32 ? "yes" : "no");
        fprintf(stderr, "native-runtime? %s\n", config.use_native_runtime ? "yes" : "no");
        fprintf(stderr, "x86-64? %s\n", is_x86_64 ? "yes" : "no");

        return EXIT_FAILURE;
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
