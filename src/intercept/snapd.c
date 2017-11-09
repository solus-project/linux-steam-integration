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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../common/common.h"
#include "../common/files.h"
#include "../common/log.h"
#include "intercept.h"

/**
 * Contains the exact soname paths that the linker will request
 */
static const char *libgl_source_table[] = {
#if UINTPTR_MAX == 0xffffffffffffffff
        "/usr/lib64/libGL.so.1",
        "/usr/lib64/libEGL.so.1",
        /* Duped due to lib vs lib64 differences between solus + snap */
        "/usr/lib/libGL.so.1",
        "/usr/lib/libEGL.so.1",
#else
        /* 32bit support */
        "/usr/lib32/libGL.so.1",
        "/usr/lib32/libEGL.so.1",
#endif
};

/**
 * Contains the exact soname paths that we'll return for biarch situations
 *
 * TODO: Also support Ubuntu/Debian multiarch overlay
 */
static const char *libgl_target_table[] = {
#if UINTPTR_MAX == 0xffffffffffffffff
        "/var/lib/snapd/lib/gl/libGL.so.1",
        "/var/lib/snapd/lib/gl/libEGL.so.1",
        /* Duped due to lib vs lib64 differences between solus + snap */
        "/var/lib/snapd/lib/gl/libGL.so.1",
        "/var/lib/snapd/lib/gl/libEGL.so.1",
#else
        /* 32bit support */
        "/var/lib/snapd/lib/gl/32/libGL.so.1",
        "/var/lib/snapd/lib/gl/32/libEGL.so.1",
#endif
};

/**
 * If the hostfs provided links don't exist, then we didn't get any links from
 * the snapd setup routine, so we'll just repoint back to mesa.
 */
static const char *libgl_mesa_table[] = {
#if UINTPTR_MAX == 0xffffffffffffffff
        "/usr/lib64/glx-provider/default/libGL.so.1",
        "/usr/lib64/glx-provider/default/libEGL.so.1",
        /* Duped due to lib vs lib64 differences between solus + snap */
        "/usr/lib/glx-provider/default/libGL.so.1",
        "/usr/lib/glx-provider/default/libEGL.so.1",
#else
        /* 32bit support */
        "/usr/lib32/glx-provider/default/libGL.so.1",
        "/usr/lib32/glx-provider/default/libEGL.so.1",
#endif
};

bool lsi_override_snapd(const char *name, __lsi_unused__ unsigned int flag, const char **soname)
{
        for (size_t i = 0; i < ARRAY_SIZE(libgl_source_table); i++) {
                const char *source = libgl_source_table[i];
                const char *target = NULL;

                if (strcmp(name, source) != 0) {
                        continue;
                }

                if (lsi_file_exists(libgl_target_table[i])) {
                        target = libgl_target_table[i];
                        lsi_log_debug(
                            "Enforcing hostfs snapd driver links: \033[31;1m%s\033[0m -> "
                            "\033[34;1m%s\033[0m",
                            name,
                            target);
                } else {
                        target = libgl_mesa_table[i];
                        lsi_log_debug(
                            "Enforcing Mesa snapd driver links: \033[31;1m%s\033[0m -> "
                            "\033[34;1m%s\033[0m",
                            name,
                            target);
                }
                *soname = target;
                return true;
        }

        /* We didn't get a match. Allow to continue */
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
