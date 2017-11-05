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
        g_signal_connect(self, "delete-event", gtk_main_quit, NULL);

        /* Sort out window bits */
        gtk_window_set_title(GTK_WINDOW(self), "Linux Steam® Integration");
        gtk_window_set_icon_name(GTK_WINDOW(self), "steam");
        gtk_window_set_position(GTK_WINDOW(self), GTK_WIN_POS_CENTER);

        /* Add main layout */
        layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(self), layout);

        /* Header box */
        header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_box_pack_start(GTK_BOX(layout), header, FALSE, FALSE, 0);

        /* header label */
        label = g_strdup_printf("<big>%s</big>", "Linux Steam® Integration");
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
            "Control the behaviour of the Steam client and games. Settings will not take effect "
            "until Steam is restarted.");
        g_object_set(widget, "xalign", 0.0, NULL);
        gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);
        gtk_label_set_line_wrap_mode(GTK_LABEL(widget), PANGO_WRAP_WORD);
        _align_label(widget);
        gtk_box_pack_start(GTK_BOX(layout), widget, FALSE, FALSE, 0);
        style = gtk_widget_get_style_context(widget);
        gtk_style_context_add_class(style, GTK_STYLE_CLASS_DIM_LABEL);
#if GTK_MINOR_VERSION <= 12
        gtk_widget_set_margin_right(widget, 100);
#else
        gtk_widget_set_margin_end(widget, 100);
#endif
        /* add a separator now */
        widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_box_pack_start(GTK_BOX(layout), widget, FALSE, FALSE, 0);
        g_object_set(widget, "margin-top", 12, "margin-bottom", 4, NULL);

        /* Start populating main grid with controls */
        grid = gtk_grid_new();
        gtk_box_pack_start(GTK_BOX(layout), grid, TRUE, TRUE, 0);

        /* Start populating options into the UI */
        self->check_native = insert_grid_toggle(
            grid,
            &row,
            "Use native runtime",
            "Alternatively, the default Steam runtime will be used, which may cause issues");
        self->check_emul32 = insert_grid_toggle(
            grid,
            &row,
            "Force 32-bit mode for Steam",
            "This may workaround some broken games, but will in turn stop the Steam store working");

#ifdef HAVE_LIBINTERCEPT
        self->check_intercept =
            insert_grid_toggle(grid,
                               &row,
                               "Use the intercept library",
                               "Override how library files are loaded to maximise "
                               "compatibility, this option is recommended");
        set_row_sensitive(self->check_intercept, FALSE);
#endif

#ifdef HAVE_LIBREDIRECT
        self->check_redirect = insert_grid_toggle(grid,
                                                  &row,
                                                  "Use the redirect library",
                                                  "Override system calls to fix known bugs in some "
                                                  "Linux ports. This option is recommended.");
        set_row_sensitive(self->check_redirect, FALSE);
#endif

        /* Finally, make sure we're visible will we. */
        gtk_container_set_border_width(GTK_CONTAINER(self), 12);
        gtk_widget_show_all(GTK_WIDGET(self));
}

static void _align_label(GtkWidget *label)
{
#if GTK_MINOR_VERSION <= 12
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
#else
        gtk_widget_set_halign(label, GTK_ALIGN_START);
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
#if GTK_MINOR_VERSION <= 12
        gtk_widget_set_margin_right(desc, 12);
#else
        gtk_widget_set_margin_end(desc, 12);
#endif
        /* Deprecated but line wrap is busted without it.. */
        g_object_set(desc, "xalign", 0.0, NULL);
        gtk_label_set_line_wrap(GTK_LABEL(desc), TRUE);
        gtk_label_set_line_wrap_mode(GTK_LABEL(desc), PANGO_WRAP_WORD);

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
