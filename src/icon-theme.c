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
#include <unistd.h>
#include "utils.h"

char** icon_theme_dirs = NULL;

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

void load_icon_themes_from_dir(const char* base_dir, const char* theme_dir, GKeyFile* kf)
{
    /* NOTE:
     * 1. theoratically, base_dir is identical to theme_dir
     * the only case that they are different is when we try to install
     * a new theme. icon themes in a temporary theme dir may later be
     * installed to base_dir, and we load it from temp. theme dir
     * before installation. So theme_dir is the temporary dir containing
     * the themes to install. base_dir is the destination directory to be
     * installed to.
     * 2. base_dir is actually a component of global variable icon_theme_dirs.
     * so it's safe to use this string without strdup() since its life
     * span is the same as the whole program and won't be freed. */

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
                theme->base_dir = base_dir;
                theme->is_removable = (0 == access(base_dir, W_OK));

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
                    app.icon_themes = g_slist_insert_sorted(app.icon_themes, theme,
                                            (GCompareFunc)icon_theme_cmp_disp_name);
                else /* this dir contains no icon or cursor theme, drop it. */
                    icon_theme_free(theme);
            }
        }
        g_dir_close(dir);
    }
}

static void load_icon_themes()
{
    int n, i;
    gtk_icon_theme_get_search_path(gtk_icon_theme_get_default(), &icon_theme_dirs, &n);
    GKeyFile* kf = g_key_file_new();
    for(i = 0; i < n; ++i)
    {
        load_icon_themes_from_dir(icon_theme_dirs[i], icon_theme_dirs[i], kf);
        g_debug("icon_theme_dirs[%d] = %s", i, icon_theme_dirs[i]);
    }
    g_key_file_free(kf);
}


static void on_icon_theme_sel_changed(GtkTreeSelection* tree_sel, gpointer user_data)
{
    GtkTreeModel* model;
    GtkTreeIter it;
    if(gtk_tree_selection_get_selected(tree_sel, &model, &it))
    {
        IconTheme* theme;
        gtk_tree_model_get(model, &it, 1, &theme, -1);
        if(g_strcmp0(theme->name, app.icon_theme))
        {
            g_free(app.icon_theme);
            app.icon_theme = g_strdup(theme->name);
            g_object_set(gtk_settings_get_default(), "gtk-icon-theme-name", app.icon_theme, NULL);

            lxappearance_changed();
        }

        gtk_widget_set_sensitive(app.icon_theme_remove_btn, theme->is_removable);
    }
}

static void on_install_theme_clicked(GtkButton* btn, gpointer user_data)
{
    install_icon_theme(gtk_widget_get_toplevel(btn));
}

static void on_remove_theme_clicked(GtkButton* btn, gpointer user_data)
{
    char* theme_name;
    GtkTreeSelection* sel;
    GtkTreeModel* model;
    GtkTreeIter it;

    if(btn == app.icon_theme_remove_btn) /* remove icon theme */
        sel = gtk_tree_view_get_selection(app.icon_theme_view);
    else if(btn == app.cursor_theme_remove_btn) /* remove cursor theme */
        sel = gtk_tree_view_get_selection(app.cursor_theme_view);
    else
        return;

    if(gtk_tree_selection_get_selected(sel, &model, &it))
    {
        IconTheme* theme;
        gboolean both = theme->has_icon && theme->has_cursor;

        if(gtk_tree_model_iter_n_children(model, NULL) < 2)
        {
            /* FIXME: the user shouldn't remove the last available theme.
             * another list needs to be checked, too. */
            return;
        }

        gtk_tree_model_get(model, &it, 1, &theme, -1);
        if(remove_icon_theme(gtk_widget_get_toplevel(btn), theme))
        {
            gtk_list_store_remove(GTK_LIST_STORE(model), &it);

            /* select the first theme */
            gtk_tree_model_get_iter_first(model, &it);
            gtk_tree_selection_select_iter(sel, &it);

            /* FIXME: in rare case, a theme can contain both icons and cursors */
            if(both) /* we need to remove item in another list store, too */
            {
                model = (model == app.icon_theme_store ? app.cursor_theme_store : app.icon_theme_store);
                /* find the item in another store */
                if(gtk_tree_model_get_iter_first(model, &it))
                {
                    IconTheme* theme2;
                    do
                    {
                        gtk_tree_model_get(model, &it, 1, &theme2, -1);
                        if(theme2 == theme)
                        {
                            gtk_list_store_remove(GTK_LIST_STORE(model), &it);
                            /* select the first theme */
                            gtk_tree_model_get_iter_first(model, &it);
                            gtk_tree_selection_select_iter(sel, &it);
                            break;
                        }
                    }while(gtk_tree_model_iter_next(model, &it));
                }
            }
        }
    }
}

void icon_theme_init(GtkBuilder* b)
{
    GSList* l;
    GtkTreeIter it;
    GtkTreeIter icon_theme_sel_it = {0};
    GtkTreeIter cursor_theme_sel_it = {0};
    GtkTreeSelection* sel;
    GtkWidget* btn;

    app.icon_theme_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
    app.cursor_theme_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
    app.icon_theme_view = GTK_WIDGET(gtk_builder_get_object(b, "icon_theme_view"));
    app.cursor_theme_view = GTK_WIDGET(gtk_builder_get_object(b, "cursor_theme_view"));

    /* install icon and cursor theme */
    btn = GTK_WIDGET(gtk_builder_get_object(b, "install_icon_theme"));
    g_signal_connect(btn, "clicked", G_CALLBACK(on_install_theme_clicked), NULL);

    btn = GTK_WIDGET(gtk_builder_get_object(b, "install_cursor_theme"));
    g_signal_connect(btn, "clicked", G_CALLBACK(on_install_theme_clicked), NULL);

    /* remove icon theme */
    app.icon_theme_remove_btn = GTK_WIDGET(gtk_builder_get_object(b, "remove_icon_theme"));
    g_signal_connect(app.icon_theme_remove_btn, "clicked", G_CALLBACK(on_remove_theme_clicked), NULL);

    app.cursor_theme_remove_btn = GTK_WIDGET(gtk_builder_get_object(b, "remove_cursor_theme"));
    g_signal_connect(app.cursor_theme_remove_btn, "clicked", G_CALLBACK(on_remove_theme_clicked), NULL);

    /* load icon and cursor themes */
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
        IconTheme* theme;
        GtkTreePath* tp = gtk_tree_model_get_path(GTK_TREE_MODEL(app.icon_theme_store), &icon_theme_sel_it);
        gtk_tree_selection_select_iter(sel, &icon_theme_sel_it);
        gtk_tree_view_scroll_to_cell(app.icon_theme_view, tp, NULL, FALSE, 0, 0);
        gtk_tree_path_free(tp);

        gtk_tree_model_get(app.icon_theme_store, &icon_theme_sel_it, 1, &theme, -1);
        gtk_widget_set_sensitive(app.icon_theme_remove_btn, theme->is_removable);
    }
    g_signal_connect(sel, "changed", G_CALLBACK(on_icon_theme_sel_changed), NULL);

    gtk_tree_view_set_model(app.cursor_theme_view, GTK_TREE_MODEL(app.cursor_theme_store));
    sel = gtk_tree_view_get_selection(app.cursor_theme_view);
    if(cursor_theme_sel_it.user_data)
    {
        IconTheme* theme;
        GtkTreePath* tp = gtk_tree_model_get_path(GTK_TREE_MODEL(app.cursor_theme_store), &cursor_theme_sel_it);
        gtk_tree_selection_select_iter(sel, &cursor_theme_sel_it);
        gtk_tree_view_scroll_to_cell(app.cursor_theme_view, tp, NULL, FALSE, 0, 0);
        gtk_tree_path_free(tp);

        gtk_tree_model_get(app.cursor_theme_store, &cursor_theme_sel_it, 1, &theme, -1);
        gtk_widget_set_sensitive(app.cursor_theme_remove_btn, theme->is_removable);
    }
}
