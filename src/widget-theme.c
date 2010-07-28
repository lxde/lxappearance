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

#include "lxappearance2.h"
#include "widget-theme.h"
#include "color-scheme.h"
#include <string.h>

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
                    themes = g_slist_prepend(themes, g_strdup(name));
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
        g_free(app.widget_theme);
        gtk_tree_model_get(model, &it, 0, &app.widget_theme, -1);
        g_object_set(gtk_settings_get_default(), "gtk-theme-name", app.widget_theme, NULL);
        lxappearance_changed();

        /* check if current theme support color schemes. */
        color_scheme_update();
    }
}

static void load_themes()
{
    char* dir;
    GSList* themes = NULL, *l;
    GtkTreeIter sel_it = {0};
    GtkTreeSelection* tree_sel;

    /* load user dir */
    dir = g_build_filename(g_get_home_dir(), ".themes", NULL);
    themes = load_themes_in_dir(dir, themes);
    g_free(dir);

    /* load system default */
    dir = gtk_rc_get_theme_dir();
    themes = load_themes_in_dir(dir, themes);
    g_free(dir);

    themes = g_slist_sort(themes, (GCompareFunc)strcmp);
    for(l = themes; l; l=l->next)
    {
        GtkTreeIter it;
        char* name = (char*)l->data;
        gtk_list_store_insert_with_values(app.widget_theme_store, &it, -1, 0, name, -1);
        /* if this theme is the one currently in use */
        if(!sel_it.user_data)
        {
            if(strcmp(name, app.widget_theme) == 0)
                sel_it = it;
        }
        g_free(name);
    }

    gtk_tree_view_set_model(app.widget_theme_view, GTK_TREE_MODEL(app.widget_theme_store));
    tree_sel = gtk_tree_view_get_selection(app.widget_theme_view);
    if(sel_it.user_data)
    {
        GtkTreePath* tp = gtk_tree_model_get_path(GTK_TREE_MODEL(app.widget_theme_store), &sel_it);
        gtk_tree_selection_select_iter(tree_sel, &sel_it);
        gtk_tree_view_scroll_to_cell(app.widget_theme_view, tp, NULL, FALSE, 0, 0);
        gtk_tree_path_free(tp);
    }

    g_slist_free(themes);

    g_signal_connect(tree_sel, "changed", G_CALLBACK(on_sel_changed), NULL);

    /* FIXME: we need to handle this, too. */
    // g_signal_connect(gtk_settings_get_default(), "notify::gtk-theme-name", G_CALLBACK(on_sel_changed), NULL);
}

void widget_theme_init(GtkBuilder* b)
{
    GtkWidget* demo;
    GdkColor black = {0, 0, 0, 0};

    demo = GTK_WIDGET(gtk_builder_get_object(b, "demo"));
    app.widget_theme_view = GTK_WIDGET(gtk_builder_get_object(b, "widget_theme_view"));

    gtk_widget_modify_bg(demo, GTK_STATE_NORMAL, &black);

    app.widget_theme_store = gtk_list_store_new(1, G_TYPE_STRING);

    /* load available themes */
    load_themes();
}
