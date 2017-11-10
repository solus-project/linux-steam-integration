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

#include <stdbool.h>

/**
 * Attempt to handle snapd specific DRI path replacements to ensure that
 * we can always load DRI modules for open source drivers.
 *
 * @note This must be run for all processes
 *
 * @param name The original search name
 * @param soname Pointer to store the final soname in
 *
 * @returns True if we performed any redirection, to stop further processing.
 */
bool lsi_override_snapd_dri(const char *name, const char **soname);

/**
 * Attempt to handle snapd specific paths and perform any necessary redirections
 * for the solus-runtime-gaming based LSI snap package.
 *
 * @note This must run for all processes, as we handle libGL interception.
 *
 * @param name Original search name
 * @param soname Pointer to store the final soname in
 *
 * @returns True if we performed any redirection, to stop further processing.
 */
bool lsi_override_snapd_gl(const char *name, const char **soname);

/**
 * Handle redirection of snapd NVIDIA libraries when they can't be found
 * sanely due to compounded LD_LIBRARY_PATH abuse.
 *
 * @param name Original search name
 * @param soname Pointer to store the final soname in
 *
 * @returns True if we performed any redirection, to stop further processing.
 */
bool lsi_override_snapd_nvidia(const char *name, const char **soname);

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
