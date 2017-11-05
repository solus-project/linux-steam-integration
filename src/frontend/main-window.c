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

#include "main-window.h"
#include "config.h"
#include "lsi.h"

#include <stdlib.h>
#include <string.h>

struct _LsiSettingsWindow {
        GtkWindow parent;
        LsiConfig config; /**<Current LSI config */

        /* Only when libintercept is enabled will we have this option */
#ifdef HAVE_LIBINTERCEPT
        GtkWidget *check_intercept;
#endif

        /* Always have these guys */
        GtkWidget *check_native;
        GtkWidget *check_emul32;
};

struct _LsiSettingsWindowClass {
        GtkWindowClass parent_class;
};

G_DEFINE_TYPE(LsiSettingsWindow, lsi_settings_window, GTK_TYPE_WINDOW)

/**
 * lsi_settings_window_new:
 *
 * Construct a new LsiSettingsWindow object
 */
GtkWidget *lsi_settings_window_new()
{
        return g_object_new(LSI_TYPE_SETTINGS_WINDOW, "type", GTK_WINDOW_TOPLEVEL, NULL);
}

/**
 * lsi_settings_window_dispose:
 *
 * Clean up a LsiSettingsWindow instance
 */
static void lsi_settings_window_dispose(GObject *obj)
{
        G_OBJECT_CLASS(lsi_settings_window_parent_class)->dispose(obj);
}

/**
 * lsi_settings_window_class_init:
 *
 * Handle class initialisation
 */
static void lsi_settings_window_class_init(LsiSettingsWindowClass *klazz)
{
        GObjectClass *obj_class = G_OBJECT_CLASS(klazz);

        /* gobject vtable hookup */
        obj_class->dispose = lsi_settings_window_dispose;
}

/**
 * lsi_settings_window_init:
 *
 * Handle construction of the LsiSettingsWindow
 */
static void lsi_settings_window_init(LsiSettingsWindow *self)
{
        /* Ensure we correctly clean this up.. */
        memset(&self->config, 0, sizeof(LsiConfig));

        /* Load the LSI configuration now */
        if (!lsi_config_load(&self->config)) {
                lsi_config_load_defaults(&self->config);
        }

        /* For now we'll just exit on close */
        g_signal_connect(self, "delete-event", gtk_main_quit, NULL);

        /* Sort out window bits */
        gtk_window_set_title(GTK_WINDOW(self), "Linux Steam® Integration");
        gtk_window_set_icon_name(GTK_WINDOW(self), "steam");
        gtk_window_set_position(GTK_WINDOW(self), GTK_WIN_POS_CENTER);

        /* TODO: Just about anything */
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
