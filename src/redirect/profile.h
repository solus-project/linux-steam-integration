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

#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "redirect.h"

/**
 * Generator function is used to attempt creation of profiles based on the
 * steam directory
 */
typedef LsiRedirectProfile *(*lsi_profile_generator_func)(char *process_name, char *steam_root);

/**
 * Profile generator for Ark survival evolved
 */
LsiRedirectProfile *lsi_redirect_profile_new_ark(char *process_name, char *steam_root);

/**
 * Profile generator for Project Highrise
 */
LsiRedirectProfile *lsi_redirect_profile_new_project_highrise(char *process_name, char *steam_root);

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
