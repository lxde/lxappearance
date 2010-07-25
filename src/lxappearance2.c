/*
 *      lxappearance2.c
 *
 *      Copyright 2010 PCMan <pcman.tw@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "widget-theme.h"
#include "icon-theme.h"
#include "cursor-theme.h"

static GOptionEntry option_entries[] =
{
    { NULL }
};

int main(int argc, char** argv)
{
    GError* err = NULL;
    GtkBuilder* b;
    GtkWidget* win;

    /* gettext support */
#ifdef ENABLE_NLS
    bindtextdomain ( GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR );
    bind_textdomain_codeset ( GETTEXT_PACKAGE, "UTF-8" );
    textdomain ( GETTEXT_PACKAGE );
#endif

    /* init threading support */
    /*
    g_thread_init(NULL);
    gdk_threads_init();
    */

    /* initialize GTK+ and parse the command line arguments */
    if( G_UNLIKELY( ! gtk_init_with_args( &argc, &argv, "", option_entries, GETTEXT_PACKAGE, &err ) ) )
    {
        g_print( "Error: %s\n", err->message );
        return 1;
    }

    /* create GUI here */
    b = gtk_builder_new();
    if(!gtk_builder_add_from_file(b, PACKAGE_UI_DIR "/lxappearance.ui", NULL))
        return 1;

    widget_theme_init(b);
    icon_theme_init(b);
    cursor_theme_init(b);

    win = gtk_builder_get_object(b, "dlg");

    gtk_window_present(GTK_WINDOW(win));
    g_object_unref(b);

    gtk_main();

    return 0;
}
