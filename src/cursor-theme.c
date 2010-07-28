/*
 *      cursor-theme.c
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

#include "cursor-theme.h"
#include "icon-theme.h"
#include "lxappearance2.h"
#include <gdk/gdkx.h>

static void update_cursor_demo()
{
    GtkListStore* store = gtk_list_store_new(1, GDK_TYPE_PIXBUF);
    GdkCursorType types[] = {
        GDK_LEFT_PTR,
        GDK_HAND2,
        GDK_WATCH,
        GDK_FLEUR,
        GDK_XTERM,
        GDK_LEFT_SIDE,
        GDK_TOP_LEFT_CORNER,
        GDK_SB_H_DOUBLE_ARROW};
    int i;
    for(i = 0; i < G_N_ELEMENTS(types); ++i)
    {
        GtkTreeIter it;
        GdkCursor* cursor = gdk_cursor_new(types[i]);
        GdkPixbuf* pix = gdk_cursor_get_image(cursor);
        gdk_cursor_unref(cursor);
        gtk_list_store_insert_with_values(store, &it, -1, 0, pix, -1);
        g_object_unref(pix);
    }
    gtk_icon_view_set_model(app.cursor_demo_view, GTK_TREE_MODEL(store));
    g_object_unref(store);
}

static void on_cursor_theme_sel_changed(GtkTreeSelection* tree_sel, gpointer user_data)
{
    GtkTreeModel* model;
    GtkTreeIter it;
    if(gtk_tree_selection_get_selected(tree_sel, &model, &it))
    {
        IconTheme* theme;
        gtk_tree_model_get(model, &it, 1, &theme, -1);
        if(g_strcmp0(theme->name, app.cursor_theme))
        {
            g_free(app.cursor_theme);
            app.cursor_theme = g_strdup(theme->name);
            g_object_set(gtk_settings_get_default(), "gtk-cursor-theme-name", app.cursor_theme, NULL);

            update_cursor_demo();
            lxappearance_changed();
        }
    }
}

static void on_cursor_theme_size_changed(GtkRange* range, gpointer user_data)
{
    int size = (int)gtk_range_get_value(range);
    if(size != app.cursor_theme_size)
    {
        app.cursor_theme_size = size;
        g_object_set(gtk_settings_get_default(), "gtk-cursor-theme-size", size, NULL);

        update_cursor_demo();
        lxappearance_changed();
    }
}

void cursor_theme_init(GtkBuilder* b)
{
    GtkTreeSelection* sel = gtk_tree_view_get_selection(app.cursor_theme_view);
    /* treeview and model are already set up in icon_theme_init() */
    g_signal_connect(sel, "changed", G_CALLBACK(on_cursor_theme_sel_changed), NULL);

    app.cursor_size_range = GTK_RANGE(gtk_builder_get_object(b, "cursor_size"));
    gtk_range_set_value(app.cursor_size_range, app.cursor_theme_size);
    g_signal_connect(app.cursor_size_range, "value-changed", G_CALLBACK(on_cursor_theme_size_changed), NULL);

    app.cursor_demo_view = GTK_WIDGET(gtk_builder_get_object(b, "cursor_demo_view"));
    gtk_icon_view_set_pixbuf_column(app.cursor_demo_view, 0);
    update_cursor_demo();
}
