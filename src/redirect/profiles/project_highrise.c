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

#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../common/files.h"
#include "../common/log.h"
#include "nica/util.h"
#include "profile.h"
#include "redirect.h"

#define PHR_PREF_DIR "unity3d/SomaSim/Project Highrise/prefs"
#define PHR_PREF_FILE "unity3d/SomaSim/Project Highrise/prefs/prefs.txt"

#if UINTPTR_MAX == 0xffffffffffffffff
#define PHR_BINARY "steamapps/common/Project Highrise/Game.x86_64"
#else
#define PHR_BINARY "steamapps/common/Project Highrise/Game.x86"
#endif

/**
 * This generator function is responsible for supporting: Project Highrise
 *
 * Currently a bug exists in Projec Highrise, where on the second launch, i.e.
 * preferences tree exists, it will try to load/map the preferences directory
 * as a file, and fails to use the correct *file path*
 *
 * This is a temporary workaround and we'll let them know what we found.
 */
LsiRedirectProfile *lsi_redirect_profile_new_project_highrise(char *process_name, char *steam_path)
{
        LsiRedirectProfile *p = NULL;
        LsiRedirect *redirect = NULL;
        autofree(char) *p_source = NULL;
        autofree(char) *p_target = NULL;
        autofree(char) *match_process = NULL;
        autofree(char) *test_process = NULL;
        autofree(char) *config_dir = NULL;

        if (asprintf(&match_process, "%s/%s", steam_path, PHR_BINARY) < 0) {
                return NULL;
        }

        /* Check it even exists */
        test_process = realpath(match_process, NULL);
        if (!test_process) {
                return NULL;
        }

        /* Check the process name now */
        if (strcmp(test_process, process_name) != 0) {
                return NULL;
        }

        config_dir = lsi_get_user_config_dir();
        if (!config_dir) {
                return NULL;
        }

        p = lsi_redirect_profile_new("Project Highrise");
        if (!p) {
                fputs("OUT OF MEMORY\n", stderr);
                return NULL;
        }

        if (asprintf(&p_source, "%s/%s", config_dir, PHR_PREF_DIR) < 0) {
                goto failed;
        }
        if (asprintf(&p_target, "%s/%s", config_dir, PHR_PREF_FILE) < 0) {
                goto failed;
        }

        /* Don't add a redirect if the paths don't exist. */
        redirect = lsi_redirect_new_path_replacement(p_source, p_target);
        if (!redirect) {
                goto unused;
        }

        /* We're in, set our log ID now */
        lsi_log_set_id("ProjectHighrise");

        lsi_redirect_profile_insert_rule(p, redirect);

        return p;

failed:
        /* Dead in the water.. */
        fputs("OUT OF MEMORY\n", stderr);
unused:
        lsi_redirect_profile_free(p);
        return NULL;
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
