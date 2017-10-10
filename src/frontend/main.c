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
#ifdef HAVE_LIBINTERCEPT
        GtkWidget *check_intercept = NULL;
#endif
        GtkWidget *label = NULL;
        GtkSettings *settings = NULL;
        int response = 0;
        bool is_x86_64 = lsi_system_is_64bit();
        LsiConfig lconfig = { 0 };

        dialog = gtk_dialog_new();
        g_object_set(G_OBJECT(dialog),
                     "title",
                     "Linux Steam® Integration",
                     "icon-name",
                     "steam",
                     NULL);

        settings = gtk_settings_get_default();
        g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);

        /* Sort out the buttons */
        (void)gtk_dialog_add_button(GTK_DIALOG(dialog), "Cancel", GTK_RESPONSE_REJECT);
        button = gtk_dialog_add_button(GTK_DIALOG(dialog), "OK", GTK_RESPONSE_ACCEPT);
        gtk_style_context_add_class(gtk_widget_get_style_context(button), "suggested-action");

        /* Pack the grid */
        grid = gtk_grid_new();
        gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
        gtk_grid_set_row_spacing(GTK_GRID(grid), 12);

        container = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_container_add(GTK_CONTAINER(container), grid);
        gtk_container_set_border_width(GTK_CONTAINER(container), 6);

        /* Add a splash of colour. */
        image = gtk_image_new_from_icon_name("steam", GTK_ICON_SIZE_DIALOG);
        gtk_image_set_pixel_size(GTK_IMAGE(image), 64);
        gtk_grid_attach(GTK_GRID(grid), image, 0, 0, 1, 1);

        label = gtk_label_new(
            "<big>Linux Steam® Integration</big>\n"
            "Note that settings are not applied until the next time Steam® starts.\n"
            "Use the \'Exit Steam\' option on the Steam® tray icon to exit the application.");
        _align_label(label);
        gtk_grid_attach(GTK_GRID(grid), label, 1, 0, 2, 1);
        gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
        gtk_widget_set_margin_bottom(label, 8);

        label = gtk_label_new("Use the native runtime provided by the Operating System");
        _align_label(label);
        gtk_widget_set_tooltip_text(label,
                                    "Alternatively, the default Steam® runtime "
                                    "will be used, which may cause issues.");
        gtk_grid_attach(GTK_GRID(grid), label, 1, 1, 1, 1);
        check_native = gtk_switch_new();
        gtk_grid_attach(GTK_GRID(grid), check_native, 2, 1, 1, 1);

        /* Handle the 32-bit cruft */
        label = gtk_label_new("Force 32-bit mode for Steam®");
        _align_label(label);
        if (is_x86_64) {
                gtk_widget_set_tooltip_text(label,
                                            "Some games may run better using 32-bit than 64-bit");
        } else {
                gtk_widget_set_tooltip_text(label, "You are using a 32-bit operating system");
        }

        gtk_grid_attach(GTK_GRID(grid), label, 1, 2, 1, 1);
        check_emul32 = gtk_switch_new();
        gtk_grid_attach(GTK_GRID(grid), check_emul32, 2, 2, 1, 1);

        gtk_widget_set_sensitive(label, is_x86_64);
        gtk_widget_set_sensitive(check_emul32, is_x86_64);

#ifdef HAVE_LIBINTERCEPT
        label = gtk_label_new("Use liblsi-intercept to override Steam's library loading mechanism");
        _align_label(label);
        gtk_widget_set_tooltip_text(label,
                                    "This can help with many library issues with the core client");
        gtk_grid_attach(GTK_GRID(grid), label, 1, 3, 1, 1);
        check_intercept = gtk_switch_new();
        gtk_grid_attach(GTK_GRID(grid), check_intercept, 2, 3, 1, 1);
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

        /* Cleanup */
        gtk_widget_destroy(dialog);

        /* Try to write new config */
        if (!lsi_config_store(&lconfig)) {
                lsi_report_failure("Failed to save configuration: %s", strerror(errno));
                return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
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
