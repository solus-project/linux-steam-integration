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

#pragma once

#define _GNU_SOURCE

#include <stdbool.h>
#include <stdlib.h>

/**
 * Determine the home directory
 */
const char *lsi_get_home_dir(void);

/**
 * Determine the home config directory
 */
char *lsi_get_user_config_dir(void);

/**
 * Find out where Steam is installed
 */
char *lsi_get_steam_dir(void);

/**
 * Grab all the valid steam installation directories
 */
char **lsi_get_steam_paths(void);

/**
 * Quick helper to determine if the path exists
 */
bool lsi_file_exists(const char *path);

/**
 * Determine the basename'd process
 */
char *lsi_get_process_name(void);

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
