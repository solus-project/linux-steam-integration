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

#include <errno.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "lsi.h"

static inline void _align_label(GtkWidget *label)
{
#if GTK_MINOR_VERSION <= 12
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
#else
        gtk_widget_set_halign(label, GTK_ALIGN_START);
#endif
        gtk_widget_set_hexpand(label, FALSE);
        gtk_widget_set_valign(label, GTK_ALIGN_START);
}

static GtkWidget *insert_grid_toggle(GtkWidget *grid, int *row, const char *title,
                                     const char *description);

int main(int argc, char **argv)
{
        gtk_init(&argc, &argv);
        GtkWidget *dialog = NULL;
        GtkWidget *grid = NULL;
        GtkWidget *container = NULL;
        GtkWidget *button = NULL;
        GtkWidget *image = NULL;
        GtkWidget *check_native = NULL;
        GtkWidget *check_emul32 = NULL;
        GtkWidget *sep = NULL;
#ifdef HAVE_LIBINTERCEPT
        GtkWidget *check_intercept = NULL;
#endif
        GtkWidget *label = NULL;
        int response = 0;
        int row = 0;
        bool is_x86_64 = lsi_system_is_64bit();
        LsiConfig lconfig = { 0 };

        dialog = gtk_dialog_new();
        g_object_set(G_OBJECT(dialog),
                     "title",
                     "Linux Steam® Integration",
                     "icon-name",
                     "steam",
                     NULL);

        /* Sort out the buttons */
        (void)gtk_dialog_add_button(GTK_DIALOG(dialog), "Cancel", GTK_RESPONSE_REJECT);
        button = gtk_dialog_add_button(GTK_DIALOG(dialog), "OK", GTK_RESPONSE_ACCEPT);
        gtk_style_context_add_class(gtk_widget_get_style_context(button), "suggested-action");

        /* Pack the grid */
        grid = gtk_grid_new();

        container = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_container_add(GTK_CONTAINER(container), grid);
        gtk_container_set_border_width(GTK_CONTAINER(container), 6);

        label = gtk_label_new(
            "<big>Linux Steam® Integration</big>\n"
            "Note that settings are not applied until the next time Steam® starts.\n"
            "Use the \'Exit Steam\' option on the Steam® tray icon to exit the application.");
        _align_label(label);
        gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
        gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
        gtk_widget_set_margin_bottom(label, 12);

        /* Add a splash of colour. */
        image = gtk_image_new_from_icon_name("steam", GTK_ICON_SIZE_DIALOG);
        gtk_image_set_pixel_size(GTK_IMAGE(image), 64);
        gtk_grid_attach(GTK_GRID(grid), image, 1, row, 1, 1);
        gtk_widget_set_margin_bottom(image, 12);

        /* Add visual separation. */
        ++row;
        sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_grid_attach(GTK_GRID(grid), sep, 0, row, 2, 1);
        ++row;

        check_native = insert_grid_toggle(
            grid,
            &row,
            "Use native runtime",
            "Alternatively, the default Steam® runtime will be used, which may cause issues");
        check_emul32 = insert_grid_toggle(
            grid,
            &row,
            "Force 32-bit mode for Steam®",
            "This may workaround some broken games, but will in turn stop the Steam store working");

#ifdef HAVE_LIBINTERCEPT
        check_intercept = insert_grid_toggle(grid,
                                             &row,
                                             "Use the intercept library",
                                             "Override how library files are loaded to maximise "
                                             "compatibility, this option is recommended");
#endif

        gtk_container_set_border_width(GTK_CONTAINER(grid), 6);
        gtk_widget_set_margin_bottom(grid, 18);

        /* Load the config */
        if (!lsi_config_load(&lconfig)) {
                lsi_config_load_defaults(&lconfig);
        }

        /* Adapt UI to match config */
        gtk_switch_set_active(GTK_SWITCH(check_native), lconfig.use_native_runtime);
        gtk_switch_set_active(GTK_SWITCH(check_emul32), lconfig.force_32);

#ifdef HAVE_LIBINTERCEPT
        gtk_switch_set_active(GTK_SWITCH(check_intercept), lconfig.use_libintercept);
#endif

        /* Run the dialog */
        gtk_widget_show_all(container);
        response = gtk_dialog_run(GTK_DIALOG(dialog));

        if (response == GTK_RESPONSE_REJECT) {
                gtk_widget_destroy(dialog);
                return EXIT_SUCCESS;
        }

        /* Get the config from the UI */
        lconfig.use_native_runtime = gtk_switch_get_active(GTK_SWITCH(check_native));
        lconfig.force_32 = gtk_switch_get_active(GTK_SWITCH(check_emul32));
#ifdef HAVE_LIBINTERCEPT
        lconfig.use_libintercept = gtk_switch_get_active(GTK_SWITCH(check_intercept));
#endif

        /* Cleanup */
        gtk_widget_destroy(dialog);

        /* Try to write new config */
        if (!lsi_config_store(&lconfig)) {
                lsi_report_failure("Failed to save configuration: %s", strerror(errno));
                return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
}

static GtkWidget *insert_grid_toggle(GtkWidget *grid, int *row, const char *title,
                                     const char *description)
{
        GtkWidget *label = NULL;
        GtkWidget *desc = NULL;
        GtkWidget *toggle = NULL;
        GtkStyleContext *style = NULL;

        /* Sort out title label */
        label = gtk_label_new(title);
        _align_label(label);
        g_object_set(label, "margin-top", 12, "hexpand", TRUE, NULL);
        gtk_grid_attach(GTK_GRID(grid), label, 0, *row, 1, 1);

        /* Sort out widget */
        toggle = gtk_switch_new();
        g_object_set(toggle,
                     "halign",
                     GTK_ALIGN_END,
                     "valign",
                     GTK_ALIGN_END,
                     "vexpand",
                     FALSE,
                     NULL);
        gtk_grid_attach(GTK_GRID(grid), toggle, 1, *row, 1, 1);

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
