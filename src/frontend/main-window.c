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

struct _LsiSettingsWindow {
        GtkWindow parent;
        int __reserved1;
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
