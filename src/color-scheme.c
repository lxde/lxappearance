//      color-scheme.c
//
//      Copyright 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.

#include "lxappearance2.h"
#include "color-scheme.h"
#include <string.h>

/* http://live.gnome.org/GnomeArt/Tutorials/GtkThemes/SymbolicColors/#Default_colors_in_GNOME */
static const char* gnome_color_names[] = {
    "fg_color", /* The base for the foreground colors. */
    "bg_color", /* Color to generate the background colors from. */
    "base_color", /* The base color. */
    "text_color", /* The text color in input widgets. */
    "selected_bg_color", /* Color for the background of selected text. */
    "selected_fg_color", /* Color of selected text. */
    "tooltip_bg_color", /* Background color of tooltips. */
    "tooltip_fg_color", /* Text color for text in tooltips. */
};

static char* hash_to_color_scheme_str(GHashTable* hash)
{
    GHashTableIter it;
    char* key, *val;
    GString* ret = g_string_sized_new(100);
    g_hash_table_iter_init (&it, hash);
    while(g_hash_table_iter_next(&it, (gpointer*)&key, (gpointer*)&val))
        g_string_append_printf(ret, "%s:%s\n", key, val);
    return g_string_free(ret, FALSE);
}

static void on_color_set(GtkColorButton* btn, const char* color_name)
{
    GdkColor clr;
    char* color_str;
    gtk_color_button_get_color(btn, &clr);
    color_str = gdk_color_to_string(&clr);

    g_hash_table_replace(app.color_scheme_hash, g_strdup(color_name), color_str);
    g_free(app.color_scheme);
    app.color_scheme = hash_to_color_scheme_str(app.color_scheme_hash);

    g_object_set(gtk_settings_get_default(), "gtk-color-scheme", app.color_scheme, NULL);

    lxappearance_changed();
}

static void on_reset_color_schemes()
{
    /* FIXME: How to correctly do this? */

    /* this resets gtk color schemes to default. */
    g_free(app.color_scheme);
    app.color_scheme = NULL;

    g_object_set(gtk_settings_get_default(), "gtk-color-scheme", "", NULL);
    color_scheme_update();

    lxappearance_changed();
}

void color_scheme_init(GtkBuilder* b)
{
    int i;
    GtkWidget* btn = GTK_WIDGET(gtk_builder_get_object(b, "default_color_scheme"));
    g_signal_connect(btn, "clicked", G_CALLBACK(on_reset_color_schemes), NULL);
    /* FIXME: there is no good way to restore default colors yet. */
    gtk_widget_hide(btn);

    app.color_page = GTK_WIDGET(gtk_builder_get_object(b, "color_page"));

    app.color_scheme_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    for(i = 0; i < 8; ++i)
        app.color_btns[i] = GTK_WIDGET(gtk_builder_get_object(b, gnome_color_names[i]));;

    color_scheme_update();

    for(i = 0; i < 8; ++i)
        g_signal_connect(app.color_btns[i], "color-set", G_CALLBACK(on_color_set), gnome_color_names[i]);
}

static gboolean gtkrc_supports_colors(const char* gtkrc_file, gboolean* support)
{
    char* content;
    gboolean support_colors = FALSE;
    g_debug("check: %s", gtkrc_file);
    if(g_file_get_contents(gtkrc_file, &content, NULL, NULL))
    {
        /* FIXME: check included gtkrc files, too. */
        if(strstr(content, "gtk-color-scheme"))
            support_colors = TRUE;
        g_free(content);
    }
    else
        return FALSE;
    *support = support_colors;
    return TRUE;
}

/* check if current gtk+ widget theme supports color schemes */
void color_scheme_update()
{
    gboolean supported = FALSE;
    if(app.widget_theme)
    {
        gboolean file_found;
        char* gtkrc = g_build_filename(g_get_home_dir(), ".themes", app.widget_theme, "gtk-2.0/gtkrc", NULL);
        file_found = gtkrc_supports_colors(gtkrc, &supported);
        g_free(gtkrc);

        if(!file_found)
        {
            gtkrc = g_build_filename(gtk_rc_get_theme_dir(), app.widget_theme, "gtk-2.0/gtkrc", NULL);
            gtkrc_supports_colors(gtkrc, &supported);
            g_free(gtkrc);
        }
    }

    g_free(app.color_scheme);
    if(supported)
    {
        char** pairs, **pair;
        int i;

        g_object_get(gtk_settings_get_default(), "gtk-color-scheme", &app.color_scheme, NULL);
        g_debug("color-scheme: %s", app.color_scheme);
        gtk_widget_set_sensitive(app.color_page, TRUE);

        g_hash_table_remove_all(app.color_scheme_hash);

        /* split color scheme string into key/value pairs */
        pairs = g_strsplit_set(app.color_scheme, "\n;", -1);
        for(pair = pairs; *pair; ++pair)
        {
            char* name = strtok(*pair, ": \t");
            if(name)
            {
                char* val = strtok(NULL, " \t");
                if(val)
                    g_hash_table_replace(app.color_scheme_hash, g_strdup(name), g_strdup(val));
            }
        }
        g_strfreev(pairs);

        /* set the color to buttons */
        for(i = 0; i < 8; ++i)
        {
            GtkWidget* btn = app.color_btns[i];
            const char* color_str = (const char*)g_hash_table_lookup(app.color_scheme_hash, gnome_color_names[i]);
            g_debug("%s ='%s'", gnome_color_names[i], color_str);
            if(color_str)
            {
                GdkColor clr;
                if(gdk_color_parse(color_str, &clr))
                    gtk_color_button_set_color(GTK_COLOR_BUTTON(btn), &clr);
                gtk_widget_set_sensitive(btn, TRUE);
            }
            else
                gtk_widget_set_sensitive(btn, FALSE);
        }
    }
    else
    {
        app.color_scheme = NULL;
        gtk_widget_set_sensitive(app.color_page, FALSE);
        g_debug("color-scheme is not supported by this theme");
    }
}

