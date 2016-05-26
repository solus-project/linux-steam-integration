/*
 * This file is part of linux-steam-integration.
 *
 * Copyright (C) 2016 Ikey Doherty
 *
 * linux-steam-integration is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#include <stdint.h>
#include <stdio.h>

#include "lsi.h"

bool lsi_system_is_64bit(void)
{
#if UINTPTR_MAX == 0xffffffffffffffff
        return true;
#endif
        return false;
}

bool lsi_config_load(__lsi_unused__ LsiConfig *config)
{
        fputs("lsi_config_load(): Not yet implemented", stderr);
        return false;
}

bool lsi_config_store(__lsi_unused__ LsiConfig *config)
{
        fputs("lsi_config_store(): Not yet implemented", stderr);
        return false;
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
