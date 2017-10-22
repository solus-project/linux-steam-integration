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

/**
 * The type of redirect required
 */
typedef enum {
        LSI_REDIRECT_MIN = 1,
        LSI_REDIRECT_PATH,
} LsiRedirectType;

/**
 * LsiRedirect describes the configuration required for each operation.
 */
typedef struct LsiRedirect {
        LsiRedirectType type;
        union {
                /* Path replacement */
                struct {
                        const char *path_source;
                        const char *path_target;
                        bool path_relative;
                };
        };
        struct LsiRedirect *next;
} LsiRedirect;

/**
 * LsiRedirectOperation specifies the syscall we're intended for.
 */
typedef enum { LSI_OPERATION_OPEN = 0, LSI_NUM_OPERATIONS } LsiRedirectOperation;

/**
 * We build an LsiRedirectProfile to match the current process.
 * It contains a table of LsiRedirect members corresponding to a given
 * op, i.e:
 *
 *      op_table[LSI_OPERATION_OPEN]
 */
typedef struct LsiRedirectProfile {
        char *name; /**< Name for this profile */

        LsiRedirect op_table[LSI_NUM_OPERATIONS]; /* vtable information */
} LsiRedirectProfile;


/**
 * Construct a new LsiRedirectProfile
 *
 * @param name Profile name
 * @returns A newly allocated LsiRedirectProfile
 */
LsiRedirectProfile *lsi_redirect_profile_new(const char *name);

/**
 * Free a previously allocated LsiRedirectProfile
 *
 * @param profile Pointer to an allocated LsiRedirectProfile
 */
void lsi_redirect_profile_free(LsiRedirectProfile *profile);


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
