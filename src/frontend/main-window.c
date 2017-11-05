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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct _LsiSettingsWindow {
        GtkWindow parent;
        LsiConfig config; /**<Current LSI config */

        /* Only when libintercept is enabled will we have this option */
#ifdef HAVE_LIBINTERCEPT
        GtkWidget *check_intercept;
#endif

        /* Only when libredirect is enabled will we have this option */
#ifdef HAVE_LIBREDIRECT
        GtkWidget *check_redirect;
#endif

        /* Always have these guys */
        GtkWidget *check_native;
        GtkWidget *check_emul32;
};

struct _LsiSettingsWindowClass {
        GtkWindowClass parent_class;
};

G_DEFINE_TYPE(LsiSettingsWindow, lsi_settings_window, GTK_TYPE_WINDOW)

static void _align_label(GtkWidget *label);
static void insert_grid(GtkWidget *grid, int *row, const char *title, const char *description,
                        GtkWidget *widget);
static GtkWidget *insert_grid_toggle(GtkWidget *grid, int *row, const char *title,
                                     const char *description);
static void set_row_sensitive(GtkWidget *toggle, gboolean sensitive);
static void lsi_native_swapped(LsiSettingsWindow *window, gpointer v);
static gboolean lsi_window_closed(LsiSettingsWindow *window, GdkEvent *event, gpointer v);

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
        GtkWidget *grid = NULL;
        GtkWidget *layout = NULL;
        GtkWidget *header = NULL;
        GtkWidget *widget = NULL;
        GtkStyleContext *style = NULL;
        const gchar *xdg_desktop = NULL;
        const gchar *emul32_desc = NULL;
        bool is_64bit = false;
        gchar *label = NULL;
        int row = 0;

        /* Ensure we correctly clean this up.. */
        memset(&self->config, 0, sizeof(LsiConfig));

        /* Load the LSI configuration now */
        if (!lsi_config_load(&self->config)) {
                lsi_config_load_defaults(&self->config);
        }

                /* Conditionally apply CSD */
#if GTK_CHECK_VERSION(3, 12, 0)
        xdg_desktop = g_getenv("XDG_CURRENT_DESKTOP");
        if (xdg_desktop && (strstr(xdg_desktop, "GNOME:") || strstr(xdg_desktop, ":GNOME") ||
                            g_str_equal(xdg_desktop, "GNOME"))) {
                widget = gtk_header_bar_new();
                gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(widget), TRUE);
                gtk_window_set_titlebar(GTK_WINDOW(self), widget);
        }
#endif

        /* For now we'll just exit on close */
        g_signal_connect(self, "delete-event", G_CALLBACK(lsi_window_closed), NULL);

        /* Sort out window bits */
        gtk_window_set_title(GTK_WINDOW(self), _("Linux Steam® Integration"));
        gtk_window_set_icon_name(GTK_WINDOW(self), "steam");
        gtk_window_set_position(GTK_WINDOW(self), GTK_WIN_POS_CENTER);
        gtk_widget_set_size_request(GTK_WIDGET(self), 320, 500);
        gtk_window_set_resizable(GTK_WINDOW(self), FALSE);

        /* Add main layout */
        layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(self), layout);
        gtk_container_set_border_width(GTK_CONTAINER(self), 12);
        gtk_widget_set_valign(GTK_WIDGET(layout), GTK_ALIGN_START);

        /* Header box */
        header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_box_pack_start(GTK_BOX(layout), header, FALSE, FALSE, 0);

        /* header label */
        label = g_strdup_printf("<big>%s</big>", _("Linux Steam® Integration"));
        widget = gtk_label_new(label);
        gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
        g_free(label);
        _align_label(widget);
        gtk_box_pack_start(GTK_BOX(header), widget, FALSE, FALSE, 0);

        /* header image */
        widget = gtk_image_new_from_icon_name("steam", GTK_ICON_SIZE_DIALOG);
        gtk_widget_set_valign(widget, GTK_ALIGN_START);
        gtk_box_pack_end(GTK_BOX(header), widget, FALSE, FALSE, 0);

        /* small label */
        widget = gtk_label_new(
            _("Control the behaviour of the Steam client and games. Settings will not take effect "
              "until the Steam Client is restarted. Use the 'Exit Steam' option to ensure it "
              "closes."));
        g_object_set(widget, "xalign", 0.0, NULL);
        gtk_label_set_max_width_chars(GTK_LABEL(widget), 80);
        gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);
        gtk_label_set_line_wrap_mode(GTK_LABEL(widget), PANGO_WRAP_WORD);
        _align_label(widget);
        gtk_box_pack_start(GTK_BOX(layout), widget, TRUE, TRUE, 0);
        style = gtk_widget_get_style_context(widget);
        gtk_style_context_add_class(style, GTK_STYLE_CLASS_DIM_LABEL);
#if GTK_CHECK_VERSION(3, 12, 0)
        gtk_widget_set_margin_end(widget, 100);
#else
        gtk_widget_set_margin_right(widget, 100);
#endif
        /* add a separator now */
        widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_box_pack_start(GTK_BOX(layout), widget, FALSE, FALSE, 0);
        g_object_set(widget, "margin-top", 12, "margin-bottom", 4, NULL);

        /* Start populating main grid with controls */
        grid = gtk_grid_new();
        gtk_box_pack_start(GTK_BOX(layout), grid, FALSE, FALSE, 0);

        /* Start populating options into the UI */
        self->check_native = insert_grid_toggle(
            grid,
            &row,
            _("Use native runtime"),
            _("Switch between the native runtime and the bundled Steam runtime."));

        /* Load in the existing configuration */
        gtk_switch_set_active(GTK_SWITCH(self->check_native), self->config.use_native_runtime);

        is_64bit = lsi_system_is_64bit();
        /* Can only force on a 64-bit system. */
        if (!is_64bit) {
                /* Label is shown to indicate we can't enable 32-bit option */
                emul32_desc = _("This option has been disabled as the system is already 32-bit");
        } else {
                /* Label is shown on 64-bit systems only */
                emul32_desc =
                    _("This may workaround some broken games, but will disable the Steam store "
                      "browser.");
        }

        self->check_emul32 =
            insert_grid_toggle(grid, &row, _("Force 32-bit mode for Steam"), emul32_desc);
        set_row_sensitive(self->check_emul32, is_64bit);
        gtk_switch_set_active(GTK_SWITCH(self->check_emul32), self->config.force_32);

#ifdef HAVE_LIBINTERCEPT
        self->check_intercept = insert_grid_toggle(
            grid,
            &row,
            _("Use the intercept library"),
            _("Force Steam applications to use more native libraries to maximise compatibility."));
        set_row_sensitive(self->check_intercept, FALSE);
        gtk_switch_set_active(GTK_SWITCH(self->check_intercept), self->config.use_libintercept);
#endif

#ifdef HAVE_LIBREDIRECT
        self->check_redirect =
            insert_grid_toggle(grid,
                               &row,
                               _("Use the redirect library"),
                               _("Override system calls to fix known bugs in some "
                                 "Linux ports."));
        set_row_sensitive(self->check_redirect, FALSE);
        gtk_switch_set_active(GTK_SWITCH(self->check_redirect), self->config.use_libredirect);
#endif

        /* Hook up our signals. Basically the "native runtime" option controls the two library
         * options, they must be used in conjunction.
         */
        g_signal_connect_swapped(self->check_native,
                                 "notify::active",
                                 G_CALLBACK(lsi_native_swapped),
                                 self);

        /* Ensure our properties are now in sync. */
        lsi_native_swapped(self, self->check_native);

        /* Finally, make sure we're visible will we. */
        gtk_widget_show_all(GTK_WIDGET(self));
}

static void _align_label(GtkWidget *label)
{
#if GTK_CHECK_VERSION(3, 12, 0)
        gtk_widget_set_halign(label, GTK_ALIGN_START);
#else
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
#endif
        gtk_widget_set_hexpand(label, FALSE);
        gtk_widget_set_valign(label, GTK_ALIGN_START);
}

/**
 * Generic function to add titled description box with a custom widget into the grid.
 */
static void insert_grid(GtkWidget *grid, int *row, const char *title, const char *description,
                        GtkWidget *widget)
{
        GtkWidget *label = NULL;
        GtkWidget *desc = NULL;
        GtkStyleContext *style = NULL;

        /* Sort out title label */
        label = gtk_label_new(title);
        _align_label(label);
        g_object_set(label, "margin-top", 12, "hexpand", TRUE, NULL);
        gtk_grid_attach(GTK_GRID(grid), label, 0, *row, 1, 1);

        g_object_set(widget,
                     "halign",
                     GTK_ALIGN_END,
                     "valign",
                     GTK_ALIGN_END,
                     "vexpand",
                     FALSE,
                     NULL);
        gtk_grid_attach(GTK_GRID(grid), widget, 1, *row, 1, 1);

        *row += 1;

        /* Sort out description label */
        desc = gtk_label_new(description);
        _align_label(desc);
        style = gtk_widget_get_style_context(desc);
        gtk_style_context_add_class(style, GTK_STYLE_CLASS_DIM_LABEL);
#if GTK_CHECK_VERSION(3, 12, 0)
        gtk_widget_set_margin_end(desc, 12);
#else
        gtk_widget_set_margin_right(desc, 12);
#endif
        /* Deprecated but line wrap is busted without it.. */
        g_object_set(desc, "xalign", 0.0, NULL);
        gtk_label_set_line_wrap(GTK_LABEL(desc), TRUE);
        gtk_label_set_line_wrap_mode(GTK_LABEL(desc), PANGO_WRAP_WORD);
        gtk_label_set_max_width_chars(GTK_LABEL(desc), 90);

        gtk_grid_attach(GTK_GRID(grid), desc, 0, *row, 1, 1);
        *row += 1;

        /* Allow accessing these dynamic items again */
        g_object_set_data(G_OBJECT(widget), "lsi_title", label);
        g_object_set_data(G_OBJECT(widget), "lsi_desc", desc);
}

/**
 * Set the complete row within the grid to the given sensitivity state
 */
static void set_row_sensitive(GtkWidget *toggle, gboolean sensitive)
{
        GtkWidget *title = NULL;
        GtkWidget *desc = NULL;

        title = g_object_get_data(G_OBJECT(toggle), "lsi_title");
        desc = g_object_get_data(G_OBJECT(toggle), "lsi_desc");

        gtk_widget_set_sensitive(toggle, sensitive);
        gtk_widget_set_sensitive(title, sensitive);
        gtk_widget_set_sensitive(desc, sensitive);
}

/**
 * Builds on the insert_grid function to insert a GtkSwitch and return only the
 * switch widget.
 */
static GtkWidget *insert_grid_toggle(GtkWidget *grid, int *row, const char *title,
                                     const char *description)
{
        GtkWidget *toggle = NULL;

        /* Sort out widget */
        toggle = gtk_switch_new();
        insert_grid(grid, row, title, description, toggle);
        return toggle;
}

/**
 * Update the sensitivity of the UI controls based on whether or not we have
 * native runtime turned on
 */
static void lsi_native_swapped(LsiSettingsWindow *self, __lsi_unused__ gpointer v)
{
        gboolean native_runtime;

        native_runtime = gtk_switch_get_active(GTK_SWITCH(self->check_native));

#ifdef HAVE_LIBREDIRECT
        set_row_sensitive(self->check_redirect, native_runtime);
#endif

#ifdef HAVE_LIBINTERCEPT
        set_row_sensitive(self->check_intercept, native_runtime);
#endif
}

/**
 * Window is closed, so let's write our config out
 */
static gboolean lsi_window_closed(LsiSettingsWindow *self, __lsi_unused__ GdkEvent *event,
                                  __lsi_unused__ gpointer v)
{
        /* Just pop the properties back into the struct */
        self->config.force_32 = gtk_switch_get_active(GTK_SWITCH(self->check_emul32));
        self->config.use_native_runtime = gtk_switch_get_active(GTK_SWITCH(self->check_native));

#ifdef HAVE_LIBREDIRECT
        self->config.use_libredirect = gtk_switch_get_active(GTK_SWITCH(self->check_redirect));
#endif

#ifdef HAVE_LIBINTERCEPT
        self->config.use_libredirect = gtk_switch_get_active(GTK_SWITCH(self->check_redirect));
#endif

        /* Try to write new config */
        if (!lsi_config_store(&self->config)) {
                lsi_report_failure("%s: %s", _("Failed to save configuration"), strerror(errno));
        }

        gtk_main_quit();
        return FALSE;
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
