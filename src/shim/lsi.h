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

#pragma once

#define _GNU_SOURCE

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../common.h"
#include "config.h"

/**
 * Current Linux Steam Integration settings.
 */
typedef struct LsiConfig {
        bool force_32;           /**<Do we force 32-bit? */
        bool use_native_runtime; /**<Do we force our native runtime? */
} LsiConfig;

/**
 * Attempt to load the LsiConfiguration from storage.
 *
 * @returns true if the load succeeded, otherwise false.
 */
bool lsi_config_load(LsiConfig *config);

/**
 * Set the defaults for the LsiConfiguration
 */
void lsi_config_load_defaults(LsiConfig *config);

/**
 * Attempt to write the user config to disk.
 * On failure, this function will return false, and errno will be set
 * appropriately.
 */
bool lsi_config_store(LsiConfig *config);

/**
 * Determine if the host system is 64-bit.
 *
 * @returns true if 64-bit, otherwise false
 */
__lsi_inline__ static inline bool lsi_system_is_64bit(void)
{
#if UINTPTR_MAX == 0xffffffffffffffff
        return true;
#endif
        return false;
}

/**
 * Determine if the system being used requires the use of LD_PRELOAD to
 * ensure that steam works. Essentially this is when the new C++ 11 ABI
 * is in use, as the old Steam libraries will not be able to load.
 */
__lsi_inline__ static inline bool lsi_system_requires_preload(void)
{
#ifdef LSI_USE_NEW_ABI
        return true;
#endif
        return false;
}

/**
 * Return the required preload list for running Steam via their runtime
 * This is static .text - do not free
 */
__lsi_inline__ static inline const char *lsi_preload_list(void)
{
        /* Respect existing LD_PRELOAD in environment */
        if (getenv("LD_PRELOAD")) {
                return "$LD_PRELOAD:" LSI_PRELOAD_LIBS;
        }
        return LSI_PRELOAD_LIBS;
}

/**
 * Report a failure to the user. In the instance that DISPLAY is set, we'll
 * attempt to make use of zenity
 */
void lsi_report_failure(const char *s, ...);

/**
 * Quick helper to determine if the path exists
 */
__lsi_inline__ static inline bool lsi_file_exists(const char *path)
{
        __lsi_unused__ struct stat st = { 0 };
        return lstat(path, &st) == 0;
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
