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

#pragma once

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _LsiSettingsWindow LsiSettingsWindow;
typedef struct _LsiSettingsWindowClass LsiSettingsWindowClass;

#define LSI_TYPE_SETTINGS_WINDOW lsi_settings_window_get_type()
#define LSI_SETTINGS_WINDOW(o)                                                                     \
        (G_TYPE_CHECK_INSTANCE_CAST((o), LSI_TYPE_SETTINGS_WINDOW, LsiSettingsWindow))
#define LSI_IS_SETTINGS_WINDOW(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), LSI_TYPE_SETTINGS_WINDOW))
#define LSI_SETTINGS_WINDOW_CLASS(o)                                                               \
        (G_TYPE_CHECK_CLASS_CAST((o), LSI_TYPE_SETTINGS_WINDOW, LsiSettingsWindowClass))
#define LSI_IS_SETTINGS_WINDOW_CLASS(o) (G_TYPE_CHECK_CLASS_TYPE((o), LSI_TYPE_SETTINGS_WINDOW))
#define LSI_SETTINGS_WINDOW_GET_CLASS(o)                                                           \
        (G_TYPE_INSTANCE_GET_CLASS((o), LSI_TYPE_SETTINGS_WINDOW, LsiSettingsWindowClass))

GtkWidget *lsi_settings_window_new(void);

GType lsi_settings_window_get_type(void);

G_END_DECLS

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
