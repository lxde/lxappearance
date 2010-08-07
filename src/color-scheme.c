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

#include "lxappearance.h"
#include "color-scheme.h"
#include <string.h>

static GRegex* gtkrc_include_reg = NULL;
static GRegex* gtkrc_color_scheme_reg = NULL;

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

char* color_scheme_hash_to_str(GHashTable* hash)
{
    GHashTableIter it;
    char* key, *val;
    GString* ret = g_string_sized_new(100);
    g_hash_table_iter_init (&it, hash);
    while(g_hash_table_iter_next(&it, (gpointer*)&key, (gpointer*)&val))
        g_string_append_printf(ret, "%s:%s\n", key, val);
    return g_string_free(ret, FALSE);
}

void color_scheme_str_to_hash(GHashTable* hash, const char* color_str)
{
    /* g_debug("color_str: %s", color_str); */
    /* split color scheme string into key/value pairs */
    char** pairs = g_strsplit_set(color_str, "\n;", -1);
    char** pair;
    for(pair = pairs; *pair; ++pair)
    {
        char* name = strtok(*pair, ": \t");
        /* g_debug("color_name = %s", name); */
        if(name)
        {
            char* val = strtok(NULL, " \t");
            if(val)
                g_hash_table_replace(hash, g_strdup(name), g_strdup(val));
        }
    }
    g_strfreev(pairs);
}

static void on_color_set(GtkColorButton* btn, const char* color_name)
{
    GdkColor clr;
    char* color_str;
    gtk_color_button_get_color(btn, &clr);
    color_str = gdk_color_to_string(&clr);

    g_hash_table_replace(app.color_scheme_hash, g_strdup(color_name), color_str);
    g_free(app.color_scheme);
    app.color_scheme = color_scheme_hash_to_str(app.color_scheme_hash);

    g_object_set(gtk_settings_get_default(), "gtk-color-scheme", app.color_scheme, NULL);

    lxappearance_changed();
}

static void update_color_buttons()
{
    int i;
    /* set the color to buttons */
    GHashTable* hash;

    /* if custom color scheme is not used, use the default one. */
    if(app.color_scheme)
        hash = app.color_scheme_hash;
    else
        hash = app.default_color_scheme_hash;

    for(i = 0; i < 8; ++i)
    {
        GtkWidget* btn = app.color_btns[i];
        const char* color_name = gnome_color_names[i];
        const char* color_str = (const char*)g_hash_table_lookup(hash, color_name);
        /* g_debug("%s ='%s'", gnome_color_names[i], color_str); */
        if(color_str)
        {
            GdkColor clr;
            if(gdk_color_parse(color_str, &clr))
            {
                /* prevent invoking color-set handlers here. */
                g_signal_handlers_block_by_func(btn, on_color_set, (gpointer)color_name);
                gtk_color_button_set_color(GTK_COLOR_BUTTON(btn), &clr);
                g_signal_handlers_unblock_by_func(btn, on_color_set, (gpointer)color_name);
            }
            gtk_widget_set_sensitive(btn, TRUE);
        }
        else
            gtk_widget_set_sensitive(btn, FALSE);
    }
}

static void hash_table_copy(GHashTable* dest, GHashTable* src)
{
    GHashTableIter it;
    char* key, *val;
    g_hash_table_remove_all(dest);
    g_hash_table_iter_init(&it, src);
    while(g_hash_table_iter_next(&it, (gpointer*)&key, (gpointer*)&val))
        g_hash_table_insert(dest, g_strdup(key), g_strdup(val));
}

static void on_custom_color_toggled(GtkToggleButton* btn, gpointer user_data)
{
    g_free(app.color_scheme);
    if(gtk_toggle_button_get_active(btn)) /* use customized color scheme. */
    {
        gtk_widget_set_sensitive(app.color_table, TRUE);
        /* copy default colors to custom color hash table */
        hash_table_copy(app.color_scheme_hash, app.default_color_scheme_hash);
        app.color_scheme = color_scheme_hash_to_str(app.color_scheme_hash);
        g_object_set(gtk_settings_get_default(), "gtk-color-scheme", app.color_scheme, NULL);
    }
    else /* use default colors provided by the theme. */
    {
        char* color_scheme_str;
        gtk_widget_set_sensitive(app.color_table, FALSE);
        /* restore default colors */
        app.color_scheme = NULL;
        g_hash_table_remove_all(app.color_scheme_hash);
        if(g_hash_table_size(app.default_color_scheme_hash) > 0)
            color_scheme_str = color_scheme_hash_to_str(app.default_color_scheme_hash);
        else
            color_scheme_str = g_strdup("");
        g_object_set(gtk_settings_get_default(), "gtk-color-scheme", color_scheme_str, NULL);
        g_free(color_scheme_str);
    }
    update_color_buttons();

    lxappearance_changed();
}

void color_scheme_init(GtkBuilder* b)
{
    int i;
    /* regular expressions used to parse gtkrc files */
    gtkrc_include_reg = g_regex_new(
        "[\\s]*include[\\s]+(\"([^\"]+)\"|'([^']+)')",
        G_REGEX_MULTILINE|G_REGEX_OPTIMIZE, 0, NULL);

    gtkrc_color_scheme_reg = g_regex_new(
        "[\\s]*(gtk-color-scheme|gtk_color_scheme)[\\s]*=[\\s]*(\"([^\"]+)\"|'([^']+)')",
        G_REGEX_MULTILINE|G_REGEX_OPTIMIZE, 0, NULL);

    app.color_table = GTK_WIDGET(gtk_builder_get_object(b, "color_table"));
    app.custom_colors = GTK_WIDGET(gtk_builder_get_object(b, "custom_colors"));
    app.no_custom_colors = GTK_WIDGET(gtk_builder_get_object(b, "no_custom_colors"));

    /* toggle the check box if we have custom color scheme */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app.custom_colors), app.color_scheme != NULL);
    g_signal_connect(app.custom_colors, "toggled", G_CALLBACK(on_custom_color_toggled), NULL);

    /* hash table of the default color scheme of currently selected theme. */
    app.default_color_scheme_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    for(i = 0; i < 8; ++i)
        app.color_btns[i] = GTK_WIDGET(gtk_builder_get_object(b, gnome_color_names[i]));;

    /* update color scheme page for currently selected gtk theme. */
    color_scheme_update();

    for(i = 0; i < 8; ++i)
        g_signal_connect(app.color_btns[i], "color-set", G_CALLBACK(on_color_set), (gpointer)gnome_color_names[i]);
}

/* return FALSE when the gtkrc file does not exists. */
gboolean gtkrc_file_get_color_scheme(const char* gtkrc_file, GHashTable* hash)
{
    char* content;
    /* g_debug("check: %s", gtkrc_file); */
    if(g_file_get_contents(gtkrc_file, &content, NULL, NULL))
    {
        GMatchInfo* match_info;

        /* find gtkrc files included in this file. */
        g_regex_match(gtkrc_include_reg, content, 0, &match_info);
        while(g_match_info_matches (match_info))
        {
            gchar* include = g_match_info_fetch(match_info, 2);
            /* try to load color schemes in every included gtkrc file. */
            if(!g_path_is_absolute(include)) /* make a full path when needed. */
            {
                char* dirname = g_path_get_dirname(gtkrc_file);
                char* file = g_build_filename(dirname, include, NULL);
                g_free(dirname);
                g_free(include);
                include = file;
            }
            gtkrc_file_get_color_scheme(include, hash);
            g_free(include);
            g_match_info_next(match_info, NULL);
        }

        /* try to extract gtk-color-scheme from the gtkrc file. */
        g_regex_match(gtkrc_color_scheme_reg, content, 0, &match_info);
        while(g_match_info_matches (match_info))
        {
            char *color_scheme_str = g_match_info_fetch(match_info, 3);
            /* need to unescape the string to replace "\\n" with "\n" */
            char* unescaped = g_strcompress(color_scheme_str);
            g_free (color_scheme_str);
            color_scheme_str_to_hash(hash, unescaped);
            g_free(unescaped);
            g_match_info_next(match_info, NULL);
        }
        g_match_info_free(match_info);
        g_free(content);
    }
    else
        return FALSE;
    return TRUE;
}

/* update the color scheme page for currently selected gtk theme.
 * called when currently selected gtk theme gets changed. */
void color_scheme_update()
{
    /* the current gtk theme gets changed.
     * reload the default color scheme of current theme. */
    g_hash_table_remove_all(app.default_color_scheme_hash);

    if(app.widget_theme)
    {
        gboolean file_found;
        char* gtkrc = g_build_filename(g_get_home_dir(), ".themes", app.widget_theme, "gtk-2.0/gtkrc", NULL);
        /* if the theme is found in user-custom theme dir */
        file_found = gtkrc_file_get_color_scheme(gtkrc, app.default_color_scheme_hash);
        g_free(gtkrc);

        if(!file_found)
        {
            /* if the theme is found in system-wide theme dir */
            gtkrc = g_build_filename(gtk_rc_get_theme_dir(), app.widget_theme, "gtk-2.0/gtkrc", NULL);
            gtkrc_file_get_color_scheme(gtkrc, app.default_color_scheme_hash);
            g_free(gtkrc);
        }
        app.color_scheme_supported = (g_hash_table_size(app.default_color_scheme_hash) > 0);
    }
    else
        app.color_scheme_supported = FALSE;

    if(app.color_scheme_supported)
    {
        gtk_widget_set_sensitive(app.custom_colors, TRUE);
        gtk_widget_set_sensitive(app.color_table, app.color_scheme != NULL);
        gtk_widget_hide(app.no_custom_colors);

        /* if customized color scheme is not used,
         * use default colors of the theme. */
        if(!app.color_scheme)
        {
            char* color_scheme_str = color_scheme_hash_to_str(app.default_color_scheme_hash);
            g_object_set(gtk_settings_get_default(), "gtk-color-scheme", color_scheme_str, NULL);
            g_free(color_scheme_str);
        }
    }
    else
    {
        gtk_widget_set_sensitive(app.color_table, FALSE);
        gtk_widget_set_sensitive(app.custom_colors, FALSE);
        gtk_widget_show(app.no_custom_colors);
    }
    /* set the color to buttons */
    update_color_buttons();
}

