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
#include <stdio.h>

/**
 * Handle definitions
 */
typedef int (*lsi_open_file)(const char *p, int flags, mode_t mode);

typedef FILE *(*lsi_fopen64_file)(const char *p, const char *modes);

/**
 * Global storage of handles for nicer organisation.
 */
typedef struct LsiRedirectTable {
        lsi_open_file open;
        lsi_fopen64_file fopen64;

        /* Allow future handle opens.. */
        struct {
                void *libc;
        } handles;

        /* Our shm_open() unity3d redirect.. */
        struct {
                char *original_config_path;
                char *config_path;
                char *shm_path;
                bool enabled;
                bool failed;
                bool had_init;
        } unity3d;
} LsiRedirectTable;

void lsi_unity_startup(LsiRedirectTable *lsi_table);
void lsi_unity_cleanup(LsiRedirectTable *lsi_table);

/* API Definitions for Unity handlers */
void lsi_maybe_init_unity3d(LsiRedirectTable *lsi_table, const char *p);
void lsi_unity_backup_config(LsiRedirectTable *lsi_table);
FILE *lsi_unity_redirect(LsiRedirectTable *lsi_table, const char *p, const char *modes);
FILE *lsi_unity_get_config_file(LsiRedirectTable *lsi_table, const char *modes);
void lsi_unity_trim_copy_config(FILE *from, FILE *to);
bool is_unity3d_prefs_file(LsiRedirectTable *lsi_table, const char *p);

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
