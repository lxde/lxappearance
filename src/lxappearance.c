/*
 *      lxappearance.c
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

#include "lxappearance.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <cairo/cairo-xlib.h>
#include <gdk/gdkx.h>
#include <string.h>

#if ENABLE_DBUS
#include <dbus/dbus.h>
#endif

#include "widget-theme.h"
#include "color-scheme.h"
#include "icon-theme.h"
#include "cursor-theme.h"
#include "font.h"
#include "other.h"
#include "plugin.h"

LXAppearance app = {0};

Atom lxsession_atom = 0;
static const char* lxsession_name = NULL;

/*  Dbus functions Copy from lxsession-logout
    TODO Create a library fro this ?
*/

static gboolean check_lxde_dbus()
{
#if GLIB_CHECK_VERSION(2, 26, 0)
    GError *error = NULL;
    GDBusConnection * connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

    if (connection == NULL)
    {
        g_warning(G_STRLOC ": Failed to connect to the session message bus: %s",
                  error->message);
        g_error_free(error);
        return FALSE;
    }

    GVariant *reply = g_dbus_connection_call_sync(connection, "org.freedesktop.DBus",
                                                  "/org/freedesktop/DBus",
                                                  "org.freedesktop.DBus",
                                                  "GetNameOwner",
                                                  g_variant_new ("(s)",
                                                                 "org.lxde.SessionManager"),
                                                  NULL,
                                                  G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                                  -1, NULL, NULL);

    if (reply != NULL)
    {
        g_variant_unref(reply);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#elif ENABLE_DBUS
    DBusError error;
    dbus_error_init(&error);
    DBusConnection * connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    if (connection == NULL)
    {
        g_warning(G_STRLOC ": Failed to connect to the session message bus: %s", error.message);
        dbus_error_free(&error);
        return FALSE;
    }

    dbus_bool_t ret = dbus_bus_name_has_owner(connection,"org.lxde.SessionManager",NULL);

    if (ret == TRUE)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#else
    return FALSE;
#endif
}

static void check_lxsession()
{
    GdkDisplay *display = gdk_display_get_default();
#if GTK_CHECK_VERSION(3, 0, 0)
    if (GDK_IS_X11_DISPLAY(display))
#endif
    {
        lxsession_atom = XInternAtom( GDK_DISPLAY_XDISPLAY(display), "_LXSESSION", True );
        if( lxsession_atom != None )
        {
            XGrabServer( GDK_DISPLAY_XDISPLAY(display) );
            if( XGetSelectionOwner( GDK_DISPLAY_XDISPLAY(display), lxsession_atom ) )
            {
                app.use_lxsession = TRUE;
                lxsession_name = g_getenv("DESKTOP_SESSION");
            }
            XUngrabServer( GDK_DISPLAY_XDISPLAY(display) );
        }
    }

    /* Check Lxsession also with dbus */
    if (check_lxde_dbus())
    {
        app.use_lxsession = TRUE;
        lxsession_name = g_getenv("DESKTOP_SESSION");
    }

}

static GOptionEntry option_entries[] =
{
    { NULL }
};

static gboolean verify_cursor_theme(GKeyFile *kf, const char *cursor_theme,
                                    const char *test)
{
    char *fpath;
    gboolean ret;

    /* get the inherited theme name. */
    fpath = g_build_filename(g_get_home_dir(), ".icons", cursor_theme,
                             "index.theme", NULL);
    ret = g_key_file_load_from_file(kf, fpath, 0, NULL);
    g_free(fpath);

    if (!ret)
    {
        fpath = g_build_filename("icons", cursor_theme, "index.theme", NULL);
        ret = g_key_file_load_from_data_dirs(kf, fpath, NULL, 0, NULL);
        g_free(fpath);
    }

    if (ret)
    {
        fpath = g_key_file_get_string(kf, "Icon Theme", "Inherits", NULL);
        if (fpath == NULL) /* end of chain, success */
            return TRUE;
        if (strcmp(fpath, test) == 0) /* recursion */
            ret = FALSE;
        else if (!verify_cursor_theme(kf, fpath, test)) /* recursion */
            ret = FALSE;
        else /* check recursion against this one too */
            ret = verify_cursor_theme(kf, fpath, cursor_theme);
        g_free(fpath);
    }
    return ret;
}

static void save_cursor_theme_name()
{
    char* dir_path;
    GKeyFile* kf;

    if (app.cursor_theme == NULL || strcmp(app.cursor_theme, "default") == 0)
        return;

    dir_path = g_build_filename(g_get_home_dir(), ".icons/default", NULL);
    kf = g_key_file_new();
    /* test if cursor theme isn't recursed and don't use it otherwise */
    if (!verify_cursor_theme(kf, app.cursor_theme, "default"))
    {
        g_free(app.cursor_theme);
        app.cursor_theme = NULL; /* FIXME: replace with "default"? */
        /* FIXME: show an error message */
    }
    /* SF bug #614: ~/.icons/default may be symlink so remove symlink */
    else if (g_file_test(dir_path, G_FILE_TEST_IS_SYMLINK) &&
                         g_unlink(dir_path) != 0)
    {
        /* FIXME: show an error message */
    }
    else if (0 == g_mkdir_with_parents(dir_path, 0700))
    {
        char* index_theme = g_build_filename(dir_path, "index.theme", NULL);
        char* content = g_strdup_printf(
                        "# This file is written by LXAppearance. Do not edit.\n"
                        "[Icon Theme]\n"
                        "Name=Default\n"
                        "Comment=Default Cursor Theme\n"
                        "Inherits=%s\n", app.cursor_theme);
        g_file_set_contents(index_theme, content, -1, NULL);
        g_free(content);
        g_free(index_theme);
    }
    g_key_file_free(kf);
    g_free(dir_path);

    /*
    dir_path = g_build_filename(g_get_home_dir(), ".Xdefaults", NULL);
    Xcursor.theme: name
    Xcursor.size: [size]
    g_file_set_contents(dir_path, "", -1, NULL);
    g_free(dir_path);
    */
}

static void reload_all_programs()
{
#if GTK_CHECK_VERSION(3, 0, 0)

/* TODO Port this to something else than gdk_event_send_clientmessage_toall */

#else
    GdkEventClient event;
    event.type = GDK_CLIENT_EVENT;
    event.send_event = TRUE;
    event.window = NULL;

    if( app.use_lxsession )
    {
        event.message_type = gdk_atom_intern_static_string("_LXSESSION");
        event.data.b[0] = 0;    /* LXS_RELOAD */
    }
    else
    {
        /* if( icon_only )
            event.message_type = gdk_atom_intern("_GTK_LOAD_ICONTHEMES", FALSE);
        */
        event.message_type = gdk_atom_intern("_GTK_READ_RCFILES", FALSE);
    }
    event.data_format = 8;
    gdk_event_send_clientmessage_toall((GdkEvent *)&event);
#endif
}

static void lxappearance_save_gtkrc()
{
    static const char* tb_styles[]={
        "GTK_TOOLBAR_ICONS",
        "GTK_TOOLBAR_TEXT",
        "GTK_TOOLBAR_BOTH",
        "GTK_TOOLBAR_BOTH_HORIZ"
    };
    static const char* tb_icon_sizes[]={
        "GTK_ICON_SIZE_INVALID",
        "GTK_ICON_SIZE_MENU",
        "GTK_ICON_SIZE_SMALL_TOOLBAR",
        "GTK_ICON_SIZE_LARGE_TOOLBAR",
        "GTK_ICON_SIZE_BUTTON",
        "GTK_ICON_SIZE_DND",
        "GTK_ICON_SIZE_DIALOG"
    };

    char **gtkrc_files = gtk_rc_get_default_files();
    char *file_path = NULL;
    GString* content = g_string_sized_new(512);

    /* check for availability of path via GTK2_RC_FILES */
    while (gtkrc_files && gtkrc_files[0])
        if (g_str_has_prefix(gtkrc_files[0], g_get_home_dir()))
            break;
        else
            gtkrc_files++;
    /* if none found then use hardcoded one */
    if (gtkrc_files == NULL || gtkrc_files[0] == NULL)
        file_path = g_build_filename(g_get_home_dir(), ".gtkrc-2.0", NULL);
    g_string_append(content,
        "# DO NOT EDIT! This file will be overwritten by LXAppearance.\n"
        "# Any customization should be done in ~/.gtkrc-2.0.mine instead.\n\n");

    /* include ~/.gtkrc-2.0.mine first to be able to apply changes done
       by LXAppearance if the same settings exist in that file */
    g_string_append_printf(content,
        "include \"%s/.gtkrc-2.0.mine\"\n",
        g_get_home_dir());

    if(app.widget_theme)
        g_string_append_printf(content,
            "gtk-theme-name=\"%s\"\n", app.widget_theme);
    if(app.icon_theme)
        g_string_append_printf(content,
            "gtk-icon-theme-name=\"%s\"\n", app.icon_theme);
    if(app.default_font)
        g_string_append_printf(content,
            "gtk-font-name=\"%s\"\n", app.default_font);
    if(app.cursor_theme)
        g_string_append_printf(content,
            "gtk-cursor-theme-name=\"%s\"\n", app.cursor_theme);
    save_cursor_theme_name();

    g_string_append_printf(content,
        "gtk-cursor-theme-size=%d\n"
        "gtk-toolbar-style=%s\n"
        "gtk-toolbar-icon-size=%s\n"
        "gtk-button-images=%d\n"
        "gtk-menu-images=%d\n"
#if GTK_CHECK_VERSION(2, 14, 0)
        "gtk-enable-event-sounds=%d\n"
        "gtk-enable-input-feedback-sounds=%d\n"
#endif
        "gtk-xft-antialias=%d\n"
        "gtk-xft-hinting=%d\n"

        , app.cursor_theme_size,
        tb_styles[app.toolbar_style],
        tb_icon_sizes[app.toolbar_icon_size],
        app.button_images ? 1 : 0,
        app.menu_images ? 1 : 0,
#if GTK_CHECK_VERSION(2, 14, 0)
        app.enable_event_sound ? 1 : 0,
        app.enable_input_feedback ? 1 : 0,
#endif
        app.enable_antialising ? 1 : 0,
        app.enable_hinting ? 1 : 0
        );

    if(app.hinting_style)
        g_string_append_printf(content,
            "gtk-xft-hintstyle=\"%s\"\n", app.hinting_style);

    if(app.font_rgba)
        g_string_append_printf(content,
            "gtk-xft-rgba=\"%s\"\n", app.font_rgba);

    if(app.modules && app.modules[0])
        g_string_append_printf(content, "gtk-modules=\"%s\"\n", app.modules);

#if 0
    /* unfortunately we cannot set colors without XSETTINGS daemon,
       themes will override any custom settings in .gtkrc-2.0 file */
    /* FIXME: we should support other xsettings daemons too */
    if(app.color_scheme)
    {
        char* escaped = g_strescape(app.color_scheme, NULL);
        g_string_append_printf(content,
            "gtk-color-scheme=\"%s\"\n",
            escaped);
        g_free(escaped);
    }
#endif

    if (file_path)
        g_file_set_contents(file_path, content->str, content->len, NULL);
    else
        g_file_set_contents(gtkrc_files[0], content->str, content->len, NULL);

    /* Save also in GTK3 folder
       Content shold be different from the gtk2 one
    */
    GKeyFile *content_gtk3 = g_key_file_new();
    char* file_path_gtk3 = g_build_filename(g_get_user_config_dir(), "gtk-3.0", NULL);
    char* file_path_settings = g_build_filename(file_path_gtk3, "settings.ini", NULL);

    if (!g_file_test(file_path_gtk3, G_FILE_TEST_IS_DIR))
    {
        g_mkdir_with_parents(file_path_gtk3, 0755);
    }

    g_key_file_load_from_file(content_gtk3, file_path_settings, /* ignoring errors */
                              G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

    if(app.widget_theme)
        g_key_file_set_string(content_gtk3, "Settings",
                              "gtk-theme-name", app.widget_theme);
    if(app.icon_theme)
        g_key_file_set_string(content_gtk3, "Settings",
                              "gtk-icon-theme-name", app.icon_theme);
    if(app.default_font)
        g_key_file_set_string(content_gtk3, "Settings",
                              "gtk-font-name", app.default_font);
    if(app.cursor_theme)
        g_key_file_set_string(content_gtk3, "Settings",
                              "gtk-cursor-theme-name", app.cursor_theme);
    save_cursor_theme_name();

    g_key_file_set_integer(content_gtk3, "Settings",
                           "gtk-cursor-theme-size", app.cursor_theme_size);
    g_key_file_set_string(content_gtk3, "Settings",
                          "gtk-toolbar-style", tb_styles[app.toolbar_style]);
    g_key_file_set_string(content_gtk3, "Settings",
                          "gtk-toolbar-icon-size", tb_icon_sizes[app.toolbar_icon_size]);
    g_key_file_set_integer(content_gtk3, "Settings",
                           "gtk-button-images", app.button_images ? 1 : 0);
    g_key_file_set_integer(content_gtk3, "Settings",
                           "gtk-menu-images", app.menu_images ? 1 : 0);
#if GTK_CHECK_VERSION(2, 14, 0)
    g_key_file_set_integer(content_gtk3, "Settings",
                           "gtk-enable-event-sounds", app.enable_event_sound ? 1 : 0);
    g_key_file_set_integer(content_gtk3, "Settings",
                           "gtk-enable-input-feedback-sounds", app.enable_input_feedback ? 1 : 0);
#endif
    g_key_file_set_integer(content_gtk3, "Settings",
                           "gtk-xft-antialias", app.enable_antialising ? 1 : 0);
    g_key_file_set_integer(content_gtk3, "Settings",
                           "gtk-xft-hinting", app.enable_hinting ? 1 : 0);

    if(app.hinting_style)
        g_key_file_set_string(content_gtk3, "Settings",
                              "gtk-xft-hintstyle", app.hinting_style);

    if(app.font_rgba)
        g_key_file_set_string(content_gtk3, "Settings",
                              "gtk-xft-rgba", app.font_rgba);

    if(app.modules && app.modules[0])
        g_key_file_set_string(content_gtk3, "Settings", "gtk-modules", app.modules);
    else
        g_key_file_remove_key(content_gtk3, "Settings", "gtk-modules", NULL);

#if 0
    /* unfortunately we cannot set colors without XSETTINGS daemon,
       themes will override any custom settings in .gtkrc-2.0 file */
    /* FIXME: we should support other xsettings daemons too */
    if(app.color_scheme)
    {
        char* escaped = g_strescape(app.color_scheme, NULL);
        g_string_append_printf(content_gtk3,
            "gtk-color-scheme=%s\n",
            escaped);
        g_free(escaped);
    }
#endif

#if GLIB_CHECK_VERSION(2, 40, 0)
    g_key_file_save_to_file(content_gtk3, file_path_settings, NULL);
#else
    char *contents;
    gsize s;

    contents = g_key_file_to_data(content_gtk3, &s, NULL);
    if (contents)
        g_file_set_contents(file_path_settings, contents, s, NULL);
    g_free(contents);
#endif

    g_free(file_path_gtk3);
    g_free(file_path_settings);
    g_string_free(content, TRUE);
    g_key_file_free(content_gtk3);
    g_free(file_path);
}

static void lxappearance_save_lxsession()
{
    char* rel_path = g_strconcat("lxsession/", lxsession_name, "/desktop.conf", NULL);
    char* user_config_file = g_build_filename(g_get_user_config_dir(), rel_path, NULL);
    char* buf;
    gsize len;
    GKeyFile* kf = g_key_file_new();

    if(!g_key_file_load_from_file(kf, user_config_file, G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
    {
        /* the user config file doesn't exist, create its parent dir */
        len = strlen(user_config_file) - strlen("/desktop.conf");
        user_config_file[len] = '\0';
        g_debug("user_config_file = %s", user_config_file);
        g_mkdir_with_parents(user_config_file, 0700);
        user_config_file[len] = '/';

        g_key_file_load_from_dirs(kf, rel_path, (const char**)g_get_system_config_dirs(), NULL,
                                  G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
    }

    g_free(rel_path);

    g_key_file_set_string( kf, "GTK", "sNet/ThemeName", app.widget_theme );
    g_key_file_set_string( kf, "GTK", "sGtk/FontName", app.default_font );

    g_key_file_set_string( kf, "GTK", "sGtk/ColorScheme", app.color_scheme ? app.color_scheme : "" );

    g_key_file_set_string( kf, "GTK", "sNet/IconThemeName", app.icon_theme );

    g_key_file_set_string( kf, "GTK", "sGtk/CursorThemeName", app.cursor_theme );
    g_key_file_set_integer( kf, "GTK", "iGtk/CursorThemeSize", app.cursor_theme_size );
    save_cursor_theme_name();

    g_key_file_set_integer( kf, "GTK", "iGtk/ToolbarStyle", app.toolbar_style );
    g_key_file_set_integer( kf, "GTK", "iGtk/ToolbarIconSize", app.toolbar_icon_size );

    g_key_file_set_integer( kf, "GTK", "iGtk/ToolbarStyle", app.toolbar_style );
    g_key_file_set_integer( kf, "GTK", "iGtk/ToolbarIconSize", app.toolbar_icon_size );

    g_key_file_set_integer( kf, "GTK", "iGtk/ButtonImages", app.button_images );
    g_key_file_set_integer( kf, "GTK", "iGtk/MenuImages", app.menu_images );

#if GTK_CHECK_VERSION(2, 14, 0)
    /* "Net/SoundThemeName\0"      "gtk-sound-theme-name\0" */
    g_key_file_set_integer( kf, "GTK", "iNet/EnableEventSounds", app.enable_event_sound);
    g_key_file_set_integer( kf, "GTK", "iNet/EnableInputFeedbackSounds", app.enable_input_feedback);
#endif
    g_key_file_set_integer( kf, "GTK", "iXft/Antialias", app.enable_antialising);
    g_key_file_set_integer( kf, "GTK", "iXft/Hinting", app.enable_hinting);
    g_key_file_set_string( kf, "GTK", "sXft/HintStyle", app.hinting_style);
    g_key_file_set_string( kf, "GTK", "sXft/RGBA", app.font_rgba);

    buf = g_key_file_to_data( kf, &len, NULL );
    g_key_file_free(kf);

    g_file_set_contents(user_config_file, buf, len, NULL);
    g_free(buf);
    g_free(user_config_file);
}

static void on_dlg_response(GtkDialog* dlg, int res, gpointer user_data)
{
    switch(res)
    {
    case GTK_RESPONSE_APPLY:

        if(app.use_lxsession)
            lxappearance_save_lxsession();
        lxappearance_save_gtkrc();

        reload_all_programs();

        app.changed = FALSE;
        gtk_dialog_set_response_sensitive(GTK_DIALOG(app.dlg), GTK_RESPONSE_APPLY, FALSE);
        break;
    case 1: /* about dialog */
        {
            GtkBuilder* b = gtk_builder_new();
            if(gtk_builder_add_from_file(b, PACKAGE_UI_DIR "/about.ui", NULL))
            {
                GtkWidget* dlg = GTK_WIDGET(gtk_builder_get_object(b, "dlg"));
                gtk_dialog_run(GTK_DIALOG(dlg));
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
    GtkSettings* settings = gtk_settings_get_default();
    g_object_get(settings,
                "gtk-theme-name", &app.widget_theme,
                "gtk-font-name", &app.default_font,
                "gtk-icon-theme-name", &app.icon_theme,
                "gtk-cursor-theme-name", &app.cursor_theme,
                "gtk-cursor-theme-size", &app.cursor_theme_size,
                "gtk-toolbar-style", &app.toolbar_style,
                "gtk-toolbar-icon-size", &app.toolbar_icon_size,
                "gtk-button-images", &app.button_images,
                "gtk-menu-images", &app.menu_images,
#if GTK_CHECK_VERSION(2, 14, 0)
                "gtk-enable-event-sounds", &app.enable_event_sound,
                "gtk-enable-input-feedback-sounds", &app.enable_input_feedback,
#endif
                "gtk-xft-antialias", &app.enable_antialising,
                "gtk-xft-hinting", &app.enable_hinting,
                "gtk-xft-hintstyle", &app.hinting_style,
                "gtk-xft-rgba", &app.font_rgba,
                "gtk-modules", &app.modules,
                NULL);
    /* try to figure out cursor theme used. */
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
            g_free(app.cursor_theme);
            app.cursor_theme = g_key_file_get_string(kf, "Icon Theme", "Inherits", NULL);
            g_debug("cursor theme name: %s", app.cursor_theme);
        }
        g_key_file_free(kf);
    }

    app.color_scheme_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    /* try to load custom color scheme if available */
    if(app.use_lxsession)
    {
        char* rel_path = g_strconcat("lxsession/", lxsession_name, "/desktop.conf", NULL);
        char* user_config_file = g_build_filename(g_get_user_config_dir(), rel_path, NULL);
        GKeyFile* kf = g_key_file_new();
        if(g_key_file_load_from_file(kf, user_config_file, 0, NULL))
            app.color_scheme = g_key_file_get_string(kf, "GTK", "sGtk/ColorScheme", NULL);
        else if(g_key_file_load_from_dirs(kf, rel_path, (const char**)g_get_system_config_dirs(), NULL, 0, NULL))
            app.color_scheme = g_key_file_get_string(kf, "GTK", "sGtk/ColorScheme", NULL);
        g_key_file_free(kf);
        g_free(rel_path);
        g_free(user_config_file);

        if(app.color_scheme)
        {
            if(*app.color_scheme)
                color_scheme_str_to_hash(app.color_scheme_hash, app.color_scheme);
            else
            {
                g_free(app.color_scheme);
                app.color_scheme = NULL;
            }
        }
    }
#if 0
    /* unfortunately we cannot set colors without XSETTINGS daemon,
       themes will override any custom settings in .gtkrc-2.0 file */
    /* FIXME: we should support other xsettings daemons too */
    else
    {
        char* gtkrc_file = g_build_filename(g_get_home_dir(), ".gtkrc-2.0", NULL);
        gtkrc_file_get_color_scheme(gtkrc_file, app.color_scheme_hash);
        g_free(gtkrc_file);
        if(g_hash_table_size(app.color_scheme_hash) > 0)
            app.color_scheme = color_scheme_hash_to_str(app.color_scheme_hash);
    }
#endif
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

#if !GLIB_CHECK_VERSION(2, 32, 0)
    g_thread_init(NULL);
#endif
    /* initialize GTK+ and parse the command line arguments */
    if( G_UNLIKELY( ! gtk_init_with_args( &argc, &argv, "", option_entries, GETTEXT_PACKAGE, &err ) ) )
    {
        g_print( "Error: %s\n", err->message );
        return 1;
    }

    app.abi_version = LXAPPEARANCE_ABI_VERSION;

    /* check if we're under LXSession */
    check_lxsession();

    /* create GUI here */
    b = gtk_builder_new();
    if(!gtk_builder_add_from_file(b, PACKAGE_UI_DIR "/lxappearance.ui", NULL))
        return 1;

    /* NOTE: GUI must be created prior to loading settings from GtkSettings object.
     * Some properties of GtkSettings class are installed by some other gtk classes.
     * For example, "gtk-menu-images" property is actually installed by initialization
     * of GtkImageMenuItem class. If we load the GUI first, then when the menu items
     * are created, this property gets correctly registered. So later it can be read. */

    /* load config values */
    settings_init();

    app.dlg = GTK_WIDGET(gtk_builder_get_object(b, "dlg"));

    widget_theme_init(b);
    color_scheme_init(b);
    icon_theme_init(b);
    cursor_theme_init(b);
    font_init(b);
    other_init(b);
    /* the page for window manager plugins */
    app.wm_page = GTK_WIDGET(gtk_builder_get_object(b, "wm_page"));

    plugins_init(b);

    g_signal_connect(app.dlg, "response", G_CALLBACK(on_dlg_response), NULL);

    gtk_window_present(GTK_WINDOW(app.dlg));
    g_object_unref(b);

    gtk_main();

    plugins_finalize();

    return 0;
}

void lxappearance_changed()
{
    if(!app.changed)
    {
        app.changed = TRUE;
        gtk_dialog_set_response_sensitive(GTK_DIALOG(app.dlg), GTK_RESPONSE_APPLY, TRUE);
    }
}

void on_check_button_toggled(GtkToggleButton* btn, gpointer user_data)
{
    gboolean* val = (gboolean*)user_data;
    gboolean new_val = gtk_toggle_button_get_active(btn);
    if(new_val != *val)
    {
        *val = new_val;
        lxappearance_changed();
    }
}
