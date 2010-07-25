/*
 *      widget-theme.c
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

#include "widget-theme.h"
#include <string.h>

static GtkWidget* widget_theme_view;
static GtkListStore* store;

static GSList* load_themes_in_dir(const char* theme_dir, GSList* themes)
{
    GDir* dir = g_dir_open(theme_dir, 0, NULL);
    if(dir)
    {
        const char* name;
        while(name = g_dir_read_name(dir))
        {
            /* test if we already have this in list */
            if(!g_slist_find_custom(themes, name, (GCompareFunc)strcmp))
            {
                /* test if this is a gtk theme */
                char* gtkrc = g_build_filename(theme_dir, name, "gtk-2.0/gtkrc", NULL);
                if(g_file_test(gtkrc, G_FILE_TEST_EXISTS))
                    themes = g_list_prepend(themes, g_strdup(name));
                g_free(gtkrc);
            }
        }
        g_dir_close(dir);
    }
    return themes;
}

static void on_sel_changed(GtkTreeSelection* sel, gpointer user_data)
{
    GtkTreeIter it;
    GtkTreeModel* model;
    if(gtk_tree_selection_get_selected(sel, &model, &it))
    {
        char* theme_name;
        gtk_tree_model_get(model, &it, 0, &theme_name, -1);
        g_object_set(gtk_settings_get_default(), "gtk-theme-name", theme_name, NULL);
        g_free(theme_name);

        /*
        FIXME: check if current theme support color schemes.

        char* color_scheme;
        g_object_get(gtk_settings_get_default(), "gtk-color-scheme", &color_scheme, NULL);
        g_debug("gtk-color-scheme:%s", color_scheme);
        g_free(color_scheme);
        */
    }
}

static void load_themes()
{
    char* dir;
    GSList* themes = NULL, *l;
    GtkTreeViewColumn* col;
    char* current;
    GtkTreeIter sel_it = {0};
    GtkTreeSelection* tree_sel;

    g_object_get(gtk_settings_get_default(), "gtk-theme-name", &current, NULL);

    /* load user dir */
    dir = g_build_filename(g_get_home_dir(), ".themes", NULL);
    themes = load_themes_in_dir(dir, themes);
    g_free(dir);

    /* load system default */
    dir = gtk_rc_get_theme_dir();
    themes = load_themes_in_dir(dir, themes);
    g_free(dir);

    col = gtk_tree_view_column_new_with_attributes("", gtk_cell_renderer_text_new(), "text", 0, NULL);
    gtk_tree_view_append_column(widget_theme_view, col);

    themes = g_slist_sort(themes, (GCompareFunc)strcmp);
    for(l = themes; l; l=l->next)
    {
        GtkTreeIter it;
        char* name = (char*)l->data;
        gtk_list_store_insert_with_values(store, &it, -1, 0, name, -1);
        /* if this theme is the one currently in use */
        if(!sel_it.user_data)
        {
            if(strcmp(name, current) == 0)
                sel_it = it;
        }
        g_free(name);
    }
    g_free(current);

    gtk_tree_view_set_model(widget_theme_view, GTK_TREE_MODEL(store));
    tree_sel = gtk_tree_view_get_selection(widget_theme_view);
    if(sel_it.user_data)
        gtk_tree_selection_select_iter(tree_sel, &sel_it);

    g_list_free(themes);

    g_signal_connect(tree_sel, "changed", G_CALLBACK(on_sel_changed), NULL);

    /* FIXME: we need to handle this, too. */
    // g_signal_connect(gtk_settings_get_default(), "notify::gtk-theme-name", G_CALLBACK(on_sel_changed), NULL);
}

void widget_theme_init(GtkBuilder* b)
{
    GtkWidget* demo;
    GdkColor black = {0, 0, 0, 0};

    demo = GTK_WIDGET(gtk_builder_get_object(b, "demo"));
    widget_theme_view = GTK_WIDGET(gtk_builder_get_object(b, "widget_theme_view"));

    gtk_widget_modify_bg(demo, GTK_STATE_NORMAL, &black);

    /* load available themes */
    store = gtk_list_store_new(1, G_TYPE_STRING);
    load_themes();
}
