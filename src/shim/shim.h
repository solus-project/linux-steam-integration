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

#include <stdbool.h>

/**
 * Attempt to bootstrap the shim environment prior to execution of
 * a command.
 */
bool shim_bootstrap(void);

/**
 * Execute the given command
 *
 * @note If this succeeds, we will not return
 */
int shim_execute(const char *command, int argc, char **argv);

/**
 * Execute the given command, respecting the $PATH variable
 *
 * @note If this succeeds, we will not return
 */
int shim_execute_path(const char *command, int argc, char **argv);

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
