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

#include <assert.h>
#include <stdio.h>

#include "config.h"
#include "lsi.h"

bool lsi_config_load(__lsi_unused__ LsiConfig *config)
{
        fputs("lsi_config_load(): Not yet implemented\n", stderr);
        return false;
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
