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

#include <stdlib.h>

/**
 * Emit some debug information if LSI_DEBUG is set
 */
void lsi_log_debug(const char *format, ...) __attribute__((format(printf, 1, 2)));

/**
 * Emit information, always shown
 */
void lsi_log_info(const char *format, ...) __attribute__((format(printf, 1, 2)));

/**
 * Emit a warning, always shown
 */
void lsi_log_warn(const char *format, ...) __attribute__((format(printf, 1, 2)));

/**
 * Emit an error, always shown
 */
void lsi_log_error(const char *format, ...) __attribute__((format(printf, 1, 2)));

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
