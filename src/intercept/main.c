/*
 * This file is part of linux-steam-integration.
 *
 * Copyright © 2016-2017 Ikey Doherty
 *
 * linux-steam-integration is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#define _GNU_SOURCE

#include "../shim/lsi.h"
#include "nica/nica.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * Is this a process we're actually interested in?
 */
static bool process_supported = false;

/**
 * Determine the basename'd process
 */
static inline char *get_process_name(void)
{
        autofree(char) *realp = NULL;
        char *basep = NULL;

        realp = realpath("/proc/self/exe", NULL);
        if (!realp) {
                return false;
        }

        basep = basename(realp);
        return strdup(basep);
}

/**
 * Find out if we're being executed by a process we actually need to override,
 * otherwise we'd not be loaded by rtld-audit
 */
static bool is_intercept_candidate(void)
{
        static const char *wanted_processes[] = {
                "steam",
        };
        autofree(char) *nom = NULL;

        nom = get_process_name();
        if (!nom) {
                return false;
        }

        for (size_t i = 0; i < ARRAY_SIZE(wanted_processes); i++) {
                if (streq(wanted_processes[i], nom)) {
                        fprintf(stderr,
                                "debug: loading Linux Steam Integration libintercept for %s\n",
                                wanted_processes[i]);
                        return true;
                }
        }
        return false;
}

/**
 * la_version is our main entry point and will only continue if our
 * process is explicitly Steam
 */
_nica_public_ unsigned int la_version(unsigned int supported_version)
{
        /* Unfortunately glibc will die if we tell it to skip us .. */
        process_supported = is_intercept_candidate();
        return supported_version;
}
