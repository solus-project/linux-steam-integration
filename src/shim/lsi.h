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

#include <stdbool.h>
#include <stdint.h>

/* Mark a variable/argument as unused to skip warnings */
#define __lsi_unused__ __attribute__((unused))

/* Force inlining of a function */
#define __lsi_inline__ __attribute__((always_inline))
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
 * Attempt to write the LsiConfiguration to storage
 *
 * @returns true if the write succeeded, otherwise false.
 */
bool lsi_config_store(LsiConfig *config);

/**
 * Set the defaults for the LsiConfiguration
 */
void lsi_config_load_defaults(LsiConfig *config);

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
 * Return the required preload list for running Steam via their runtime
 * This is static .text - do not free
 */
const char *lsi_preload_list(void);

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
