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

#include "lxappearance.h"
#include "widget-theme.h"
#include "color-scheme.h"
#include <string.h>

static GSList* load_themes_in_dir(const char* theme_dir, GSList* themes)
{
    GDir* dir = g_dir_open(theme_dir, 0, NULL);
    if(dir)
    {
        const char* name;
        while ((name = g_dir_read_name(dir)))
        {
            /* test if we already have this in list */
            if(!g_slist_find_custom(themes, name, (GCompareFunc)strcmp))
            {
                /* test if this is a gtk theme */
#if GTK_CHECK_VERSION(3, 0, 0)
                char* gtkrc = g_build_filename(theme_dir, name, "gtk-3.0/gtk.css", NULL);
#else
                char* gtkrc = g_build_filename(theme_dir, name, "gtk-2.0/gtkrc", NULL);
#endif
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

#if GTK_CHECK_VERSION(3, 0, 0)
    /* add built-in themes */
    themes = g_slist_append(themes, g_strdup("Adwaita"));
    themes = g_slist_append(themes, g_strdup("HighContrast"));
    themes = g_slist_append(themes, g_strdup("HighContrastInverse"));
#endif

    /* load from userdata theme dir first */
    dir = g_build_filename(g_get_user_data_dir(), "themes", NULL);
    themes = load_themes_in_dir(dir, themes);
    g_free(dir);

    /* load from ~/.themes dir as old style */
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

    gtk_tree_view_set_model(GTK_TREE_VIEW(app.widget_theme_view), GTK_TREE_MODEL(app.widget_theme_store));
    tree_sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.widget_theme_view));
    if(sel_it.user_data)
    {
        GtkTreePath* tp = gtk_tree_model_get_path(GTK_TREE_MODEL(app.widget_theme_store), &sel_it);
        gtk_tree_selection_select_iter(tree_sel, &sel_it);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(app.widget_theme_view), tp, NULL, FALSE, 0, 0);
        gtk_tree_path_free(tp);
    }

    g_slist_free(themes);

    g_signal_connect(tree_sel, "changed", G_CALLBACK(on_sel_changed), NULL);

    /* FIXME: we need to handle this, too. */
    // g_signal_connect(gtk_settings_get_default(), "notify::gtk-theme-name", G_CALLBACK(on_sel_changed), NULL);
}

static void on_font_set(GtkFontButton* btn, gpointer user_data)
{
    const char* font_name = gtk_font_button_get_font_name(btn);
    if(g_strcmp0(font_name, app.default_font))
    {
        g_free(app.default_font);
        app.default_font = g_strdup(font_name);
        g_object_set(gtk_settings_get_default(), "gtk-font-name", font_name, NULL);

        lxappearance_changed();
    }
}

void widget_theme_init(GtkBuilder* b)
{
    GtkWidget* demo;
    GtkWidget* demo_vbox;
    GdkColor black = {0, 0, 0, 0};

    demo = GTK_WIDGET(gtk_builder_get_object(b, "demo"));
    demo_vbox = GTK_WIDGET(gtk_builder_get_object(b, "demo_vbox"));
    app.widget_theme_view = GTK_WIDGET(gtk_builder_get_object(b, "widget_theme_view"));

    gtk_widget_modify_bg(demo, GTK_STATE_NORMAL, &black);
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_style_context_add_class (gtk_widget_get_style_context (demo_vbox), GTK_STYLE_CLASS_BACKGROUND);
#endif

    app.widget_theme_store = gtk_list_store_new(1, G_TYPE_STRING);

    /* load available themes */
    load_themes();

    app.default_font_btn = GTK_WIDGET(gtk_builder_get_object(b, "default_font"));
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(app.default_font_btn), app.default_font);
    g_signal_connect(app.default_font_btn, "font-set", G_CALLBACK(on_font_set), NULL);
}
