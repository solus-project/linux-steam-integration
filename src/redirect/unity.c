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

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../common/files.h"
#include "../common/log.h"
#include "nica/util.h"

#include "private.h"

/**
 * Cheap and cheerful, this is our default fake unity3d config which we write
 * out to trick unity3d games into reading a non broken config on Linux.
 * This forces the equivalent of "-screen-fullscreen 0" because the older
 * Unity builds will set fullscreen to 1, but they'll also set the resolution
 * width and height to 0 (which is then clamped)
 */
static const char *unity3d_config =
    "\
<unity_prefs version_major=\"1\" version_minor=\"1\">\n\
	<pref name=\"Screenmanager Is Fullscreen mode\" type=\"int\">0</pref>\n\
</unity_prefs>\n\
";

/**
 * When we rewrite the config file in place we'll repllace the "Is Fullscreen mode"
 * line with this one so that we forcibly disable it.
 */
static const char *unity3d_snippet =
    "	<pref name=\"Screenmanager Is Fullscreen mode\" type=\"int\">0</pref>\n";

static inline bool str_has_prefix(const char *a, const char *b)
{
        size_t lena, lenb;
        lena = strlen(a);
        lenb = strlen(b);
        if (lenb > lena) {
                return false;
        }
        return strncmp(a, b, lenb) == 0;
}

/**
 * The unity3d config path has been accessed, so we need to turn on our
 * workaround for unity3d prefs
 */
void lsi_maybe_init_unity3d(LsiRedirectTable *lsi_table, const char *p)
{
        if (lsi_table->unity3d.enabled) {
                return;
        }

        if (!lsi_table->unity3d.config_path) {
                return;
        }

        if (!str_has_prefix(p, lsi_table->unity3d.config_path)) {
                return;
        }

        /* Set by the main shim */
        if (!getenv("LSI_USE_UNITY_HACK")) {
                return;
        }

        /* We're in action */
        lsi_table->unity3d.enabled = true;
        lsi_log_set_id("unity3d");
        lsi_log_info("Activating \"black screen of nope\" workaround");
}

/**
 * Determine if the input path is a unity3d "prefs" file
 */
bool is_unity3d_prefs_file(LsiRedirectTable *lsi_table, const char *p)
{
        autofree(char) *clone = NULL;
        char *basenom = NULL;

        /* Must only happen when enabled */
        if (!lsi_table->unity3d.enabled) {
                return false;
        }

        /* We found it already */
        if (lsi_table->unity3d.original_config_path) {
                return false;
        }

        /* Must be in config path */
        if (!str_has_prefix(p, lsi_table->unity3d.config_path)) {
                return false;
        }

        clone = strdup(p);
        if (!clone) {
                return false;
        }
        basenom = basename(clone);
        if (!basenom) {
                return false;
        }

        /* Must be called "prefs" */
        return strcmp(basenom, "prefs") == 0 ? true : false;
}

/**
 * Copy the config from the @from file to the @to file, which
 * will also strip out any "evil" lines, i.e. the fullscreen
 * stanza.
 *
 * If the input file does not exist, the default configuration
 * will be used.
 */
void lsi_unity_trim_copy_config(FILE *from, FILE *to)
{
        char *buf = NULL;
        size_t sn = 0;
        ssize_t r = 0;

        /* No input? Write the default configuration then */
        if (!from) {
                if (fwrite(unity3d_config, strlen(unity3d_config), 1, to) != 1) {
                        lsi_log_error("Failed to create initial Unity3D config: %s",
                                      strerror(errno));
                }
                return;
        }

        /* Have an input file, write to the output but omit all the bad lines */
        while ((r = getline(&buf, &sn, from)) != -1) {
                const char *to_write = NULL;

                /* Rewrite this line, it breaks games. */
                if (strstr(buf, "Screenmanager Is Fullscreen mode")) {
                        to_write = unity3d_snippet;
                        r = strlen(to_write);
                } else {
                        to_write = buf;
                }

                if (fwrite(to_write, r, 1, to) != 1) {
                        lsi_log_error("Failed to write Unity3D config: %s", strerror(errno));
                        goto failed;
                }

                if (buf) {
                        free(buf);
                        buf = NULL;
                }
        }

        fflush(to);

failed:
        if (buf) {
                free(buf);
        }
}

/**
 * Clone the existing shm configuration into the real config path, and copy
 * all lines that we "like"
 *
 * Basically this ensures we force the dumped config to omit the fullscreen
 * options as they break so many games.
 */
void lsi_unity_backup_config(LsiRedirectTable *lsi_table)
{
        autofree(FILE) *shm_file = NULL;
        autofree(FILE) *dest_file = NULL;

        if (!lsi_table->unity3d.enabled || !lsi_table->unity3d.original_config_path) {
                return;
        }

        shm_file = lsi_unity_get_config_file(lsi_table, "r");
        if (!shm_file) {
                return;
        }

        dest_file = lsi_table->fopen64(lsi_table->unity3d.original_config_path, "w");
        if (!dest_file) {
                return;
        }

        lsi_log_debug("Saved Unity3D config to %s", lsi_table->unity3d.original_config_path);
        lsi_unity_trim_copy_config(shm_file, dest_file);
}

/**
 * Get the shm file as a FILE stream
 */
FILE *lsi_unity_get_config_file(LsiRedirectTable *lsi_table, const char *modes)
{
        int perm = O_RDONLY;

        if (strstr(modes, "w")) {
                perm = O_RDWR | O_CREAT | O_TRUNC;
        }

        /* Attempt to shm_open our fake path */
        int fd = shm_open(lsi_table->unity3d.shm_path, perm, 0666);
        if (!fd) {
                return NULL;
        }
        return fdopen(fd, modes);
}

/**
 * We now need to initialise the unity3d config if we haven't done so
 */
static void lsi_unity_init_config(LsiRedirectTable *lsi_table)
{
        autofree(FILE) *source = NULL;
        autofree(FILE) *dest = NULL;

        if (lsi_table->unity3d.had_init || lsi_table->unity3d.failed) {
                return;
        }

        lsi_table->unity3d.had_init = true;

        dest = lsi_unity_get_config_file(lsi_table, "w");
        source = lsi_table->fopen64(lsi_table->unity3d.original_config_path, "r");

        lsi_unity_trim_copy_config(source, dest);
}

/**
 * Redirect requests for non-existant config files in Unity3D to a temporary
 * shm file which we'll write our initial "sane" configuration to.
 */
FILE *lsi_unity_redirect(LsiRedirectTable *lsi_table, const char *p, const char *modes)
{
        FILE *ret = NULL;

        /* Preserve this path for later cloning.. */
        lsi_table->unity3d.original_config_path = strdup(p);

        lsi_unity_init_config(lsi_table);

        ret = lsi_unity_get_config_file(lsi_table, modes);
        if (!ret) {
                return NULL;
        }

        lsi_log_debug("fopen64(%s): Redirecting unity config '%s' to shm(%s)",
                      modes,
                      p,
                      lsi_table->unity3d.shm_path);
        return ret;
}

/**
 * Set up any needed variables for future unity usage
 */
void lsi_unity_startup(LsiRedirectTable *lsi_table)
{
        autofree(char) *xdg_config_dir = NULL;

        /* Ensure we know the path to unity3d config */
        xdg_config_dir = lsi_get_user_config_dir();
        if (asprintf(&lsi_table->unity3d.config_path, "%s/unity3d", xdg_config_dir) < 0) {
                fputs("OOM\n", stderr);
                abort();
        }

        /* shm path for our fake unity3d config path */
        if (asprintf(&lsi_table->unity3d.shm_path,
                     "/u%d-LinuxSteamIntegration.unity3d.%d",
                     getuid(),
                     getpgrp()) < 0) {
                fputs("OOM\n", stderr);
                abort();
        }

        /* Symbolic, we don't just use this for anyone, yknow. :P */
        lsi_table->unity3d.enabled = false;
        lsi_table->unity3d.failed = false;
}

void lsi_unity_cleanup(LsiRedirectTable *lsi_table)
{
        if (lsi_table->unity3d.original_config_path) {
                lsi_unity_backup_config(lsi_table);
                free(lsi_table->unity3d.original_config_path);
                lsi_table->unity3d.original_config_path = NULL;
        }

        if (lsi_table->unity3d.config_path) {
                free(lsi_table->unity3d.config_path);
                lsi_table->unity3d.config_path = NULL;
        }

        if (lsi_table->unity3d.shm_path) {
                (void)shm_unlink(lsi_table->unity3d.shm_path);
                free(lsi_table->unity3d.shm_path);
                lsi_table->unity3d.shm_path = NULL;
        }
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
