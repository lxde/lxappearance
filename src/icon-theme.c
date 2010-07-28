/*
 *      icon-theme.c
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

#include "icon-theme.h"
#include "lxappearance2.h"
#include <string.h>

gint icon_theme_cmp_name(IconTheme* t, const char* name)
{
    return strcmp(t->name, name);
}

gint icon_theme_cmp_disp_name(IconTheme* t1, IconTheme* t2)
{
    return g_utf8_collate(t1->disp_name, t2->disp_name);
}

static void icon_theme_free(IconTheme* theme)
{
    g_free(theme->comment);
    g_free(theme->name);
    if(theme->disp_name != theme->name)
        g_free(theme->disp_name);
    g_slice_free(IconTheme, theme);
}

static void load_icon_themes_from_dir(const char* theme_dir, GKeyFile* kf)
{
    GDir* dir = g_dir_open(theme_dir, 0, NULL);
    if(dir)
    {
        const char* name;
        while(name = g_dir_read_name(dir))
        {
            /* skip "default" */
            if(G_UNLIKELY(strcmp(name, "default") == 0))
                continue;
            /* test if we already have this in list */
            if(!g_slist_find_custom(app.icon_themes, name, (GCompareFunc)icon_theme_cmp_name))
            {
                IconTheme* theme = g_slice_new0(IconTheme);
                char* index_theme;
                char* cursor_subdir;

                theme->name = g_strdup(name);
                index_theme = g_build_filename(theme_dir, name, "index.theme", NULL);
                if(g_key_file_load_from_file(kf, index_theme, 0, NULL))
                {
                    /* skip hidden ones */
                    if(!g_key_file_get_boolean(kf, "Icon Theme", "Hidden", NULL))
                    {
                        theme->disp_name = g_key_file_get_locale_string(kf, "Icon Theme", "Name", NULL, NULL);
                        /* test if this is a icon theme or it's a cursor theme */
                        theme->comment = g_key_file_get_locale_string(kf, "Icon Theme", "Comment", NULL, NULL);

                        /* icon theme must have this key, so it has icons if it has this key */
                        theme->has_icon = g_key_file_has_key(kf, "Icon Theme", "Directories", NULL);
                    }
                }
                else
                    theme->disp_name = theme->name;
                g_free(index_theme);

                cursor_subdir = g_build_filename(theme_dir, name, "cursors", NULL);
                /* it contains a cursor theme */
                if(g_file_test(cursor_subdir, G_FILE_TEST_IS_DIR))
                    theme->has_cursor = TRUE;
                g_free(cursor_subdir);

                if(theme->has_icon || theme->has_cursor)
                    app.icon_themes = g_slist_prepend(app.icon_themes, theme);
                else /* this dir contains no icon or cursor theme, drop it. */
                    icon_theme_free(theme);
            }
        }
        g_dir_close(dir);
    }
}

static void load_icon_themes()
{
    const char* const *dirs = g_get_system_data_dirs();
    const char* const *pdir;
    char* dir_path;
    GKeyFile* kf = g_key_file_new();

    dir_path = g_build_filename(g_get_home_dir(), ".icons", NULL);
    load_icon_themes_from_dir(dir_path, kf);
    g_free(dir_path);

    dir_path = g_build_filename(g_get_user_data_dir(), "icons", NULL);
    load_icon_themes_from_dir(dir_path, kf);
    g_free(dir_path);

    for(pdir = dirs; *pdir; ++pdir)
    {
        dir_path = g_build_filename(*pdir, "icons", NULL);
        load_icon_themes_from_dir(dir_path, kf);
        g_free(dir_path);
    }
    g_key_file_free(kf);

    app.icon_themes = g_slist_sort(app.icon_themes, (GCompareFunc)icon_theme_cmp_disp_name);
}

static void icon_sizes_init(GtkBuilder* b)
{
    const char* names[] = {
        "gtk-menu",
        "gtk-button",
        "gtk-small-toolbar",
        "gtk-large-toolbar",
        "gtk-dnd",
        "gtk-dialog"
    };
    char* sizes_str;
    g_object_get(gtk_settings_get_default(), "gtk-icon-sizes", &sizes_str);
    g_debug("%s", sizes_str);
}

void icon_theme_init(GtkBuilder* b)
{
    GSList* l;
    GtkTreeIter it;
    GtkTreeIter icon_theme_sel_it = {0};
    GtkTreeIter cursor_theme_sel_it = {0};
    GtkTreeSelection* sel;

    app.icon_theme_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
    app.cursor_theme_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
    app.icon_theme_view = GTK_WIDGET(gtk_builder_get_object(b, "icon_theme_view"));
    app.cursor_theme_view = GTK_WIDGET(gtk_builder_get_object(b, "cursor_theme_view"));

    /* load icon themes */
    load_icon_themes();

    for(l = app.icon_themes; l; l=l->next)
    {
        IconTheme* theme = (IconTheme*)l->data;

        if(theme->has_icon)
        {
            gtk_list_store_insert_with_values(app.icon_theme_store, &it, -1, 0, theme->disp_name, 1, theme, -1);
            if(!icon_theme_sel_it.user_data)
            {
                if(strcmp(theme->name, app.icon_theme) == 0)
                    icon_theme_sel_it = it;
            }
        }

        if(theme->has_cursor)
        {
            gtk_list_store_insert_with_values(app.cursor_theme_store, &it, -1, 0, theme->disp_name, 1, theme, -1);
            if(!cursor_theme_sel_it.user_data)
            {
                if(g_strcmp0(theme->name, app.cursor_theme) == 0)
                    cursor_theme_sel_it = it;
            }
        }
    }

    /* select the currently used theme from the list */
    gtk_tree_view_set_model(app.icon_theme_view, GTK_TREE_MODEL(app.icon_theme_store));
    sel = gtk_tree_view_get_selection(app.icon_theme_view);
    if(icon_theme_sel_it.user_data)
    {
        GtkTreePath* tp = gtk_tree_model_get_path(GTK_TREE_MODEL(app.icon_theme_store), &icon_theme_sel_it);
        gtk_tree_selection_select_iter(sel, &icon_theme_sel_it);
        gtk_tree_view_scroll_to_cell(app.icon_theme_view, tp, NULL, FALSE, 0, 0);
        gtk_tree_path_free(tp);
    }

    gtk_tree_view_set_model(app.cursor_theme_view, GTK_TREE_MODEL(app.cursor_theme_store));
    sel = gtk_tree_view_get_selection(app.cursor_theme_view);
    if(cursor_theme_sel_it.user_data)
    {
        GtkTreePath* tp = gtk_tree_model_get_path(GTK_TREE_MODEL(app.cursor_theme_store), &cursor_theme_sel_it);
        gtk_tree_selection_select_iter(sel, &cursor_theme_sel_it);
        gtk_tree_view_scroll_to_cell(app.cursor_theme_view, tp, NULL, FALSE, 0, 0);
        gtk_tree_path_free(tp);
    }

    /* load "gtk-icon-sizes" */
    icon_sizes_init(b);
}
