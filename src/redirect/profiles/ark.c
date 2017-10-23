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

#include <stdio.h>
#include <stdlib.h>

#include "nica/util.h"
#include "profile.h"
#include "redirect.h"

#define ARK_BASE "steamapps/common/ARK/ShooterGame"
#define ARK_CONTENT ARK_BASE "Content"
#define ARK_BINARY ARK_BASE "Binaries/Linux/ShooterGame"

/**
 * This generator function is responsible for supporting ARK: Survival Evolved
 *
 * Simply it redirects the executable to open the correct asset when TheCenter
 * DLC is installed.
 */
LsiRedirectProfile *lsi_redirect_profile_new_ark(char *process_name, char *steam_path)
{
        LsiRedirectProfile *p = NULL;
        LsiRedirect *redirect = NULL;
        autofree(char) *mic_source = NULL;
        autofree(char) *mic_target = NULL;
        autofree(char) *match_process = NULL;
        autofree(char) *test_process = NULL;

        if (asprintf(&match_process, "%s/%s", steam_path, ARK_BINARY)) {
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

        static const char *def_mic_source =
            ARK_CONTENT "/PrimalEarth/Environment/Water/Water_DepthBlur_MIC.uasset";
        static const char *def_mic_target =
            ARK_CONTENT "/Mods/TheCenter/Assets/Mic/Water_DepthBlur_MIC.uasset";

        p = lsi_redirect_profile_new("ARK: Survival Evolved");
        if (!p) {
                fputs("OUT OF MEMORY\n", stderr);
                return NULL;
        }

        if (asprintf(&mic_source, "%s/%s", steam_path, def_mic_source) < 0) {
                goto failed;
        }
        if (asprintf(&mic_target, "%s/%s", steam_path, def_mic_target) < 0) {
                goto failed;
        }

        /* Don't add a redirect if the paths don't exist. */
        redirect = lsi_redirect_new_path_replacement(mic_source, mic_target);
        if (!redirect) {
                goto unused;
        }

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
