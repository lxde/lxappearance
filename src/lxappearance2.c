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

#include "lxappearance2.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>

#include "widget-theme.h"
#include "icon-theme.h"
#include "cursor-theme.h"

LXAppearance app = {0};

Atom lxsession_atom = 0;

static void check_lxsession()
{
    lxsession_atom = XInternAtom( GDK_DISPLAY(), "_LXSESSION", True );
    if( lxsession_atom != None )
    {
        XGrabServer( GDK_DISPLAY() );
        if( XGetSelectionOwner( GDK_DISPLAY(), lxsession_atom ) )
            app.use_lxsession = TRUE;
        XUngrabServer( GDK_DISPLAY() );
    }
}

static GOptionEntry option_entries[] =
{
    { NULL }
};

static void lxappearance_save_gtkrc()
{

}

static void lxappearance_save_lxsession()
{

}

static void on_dlg_response(GtkDialog* dlg, int res, gpointer user_data)
{
    switch(res)
    {
    case GTK_RESPONSE_APPLY:

        if(app.use_lxsession)
            lxappearance_save_lxsession();
        else
            lxappearance_save_gtkrc();

        app.changed = FALSE;
        gtk_dialog_set_response_sensitive(app.dlg, GTK_RESPONSE_APPLY, FALSE);
        break;
    case 1: /* about dialog */
        {
            GtkBuilder* b = gtk_builder_new();
            if(gtk_builder_add_from_file(b, PACKAGE_UI_DIR "/about.ui", NULL))
            {
                GtkWidget* dlg = GTK_WIDGET(gtk_builder_get_object(b, "dlg"));
                gtk_dialog_run(dlg);
                gtk_widget_destroy(dlg);
            }
            g_object_unref(b);
        }
        break;
    default:
        gtk_main_quit();
    }
}

static void settings_init()
{
    g_object_get(gtk_settings_get_default(), "gtk-theme-name", &app.widget_theme, NULL);
    g_object_get(gtk_settings_get_default(), "gtk-icon-theme-name", &app.icon_theme, NULL);

    /* try to figure out cursor theme used. */
    g_object_get(gtk_settings_get_default(), "gtk-cursor-theme-name", &app.cursor_theme, NULL);
    if(!app.cursor_theme || g_strcmp0(app.cursor_theme, "default") == 0)
    {
        /* get the real theme name from default. */
        GKeyFile* kf = g_key_file_new();
        char* fpath = g_build_filename(g_get_home_dir(), ".icons/default/index.theme", NULL);
        gboolean ret = g_key_file_load_from_file(kf, fpath, 0, NULL);
        g_free(fpath);

        if(!ret)
            ret = g_key_file_load_from_data_dirs(kf, "icons/default/index.theme", NULL, 0, NULL);

        if(ret)
        {
            app.cursor_theme = g_key_file_get_string(kf, "Icon Theme", "Inherits", NULL);
            g_debug("cursor theme name: %s", app.cursor_theme);
        }
        g_key_file_free(kf);
    }
}

int main(int argc, char** argv)
{
    GError* err = NULL;
    GtkBuilder* b;

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

    /* check if we're under LXSession */
    check_lxsession();

    /* load config values */
    settings_init();

    /* create GUI here */
    b = gtk_builder_new();
    if(!gtk_builder_add_from_file(b, PACKAGE_UI_DIR "/lxappearance.ui", NULL))
        return 1;

    widget_theme_init(b);
    icon_theme_init(b);
    cursor_theme_init(b);

    app.dlg = GTK_WIDGET(gtk_builder_get_object(b, "dlg"));
    g_signal_connect(app.dlg, "response", G_CALLBACK(on_dlg_response), NULL);

    gtk_window_present(GTK_WINDOW(app.dlg));
    g_object_unref(b);

    gtk_main();

    return 0;
}

void lxappearance_changed()
{
    if(!app.changed)
    {
        app.changed = TRUE;
        gtk_dialog_set_response_sensitive(app.dlg, GTK_RESPONSE_APPLY, TRUE);
    }
}
