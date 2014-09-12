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
#include "lxappearance.h"
#include <gdk/gdkx.h>

static void update_cursor_demo()
{
    GtkListStore* store = gtk_list_store_new(1, GDK_TYPE_PIXBUF);
    GdkCursor* cursor;
    GdkCursorType types[] = {
        GDK_LEFT_PTR,
        GDK_HAND2,
        GDK_WATCH,
        GDK_FLEUR,
        GDK_XTERM,
        GDK_LEFT_SIDE,
        GDK_TOP_LEFT_CORNER,
        GDK_SB_H_DOUBLE_ARROW};
    int i, n;
    for(i = 0; i < G_N_ELEMENTS(types); ++i)
    {
        GtkTreeIter it;
        cursor = gdk_cursor_new(types[i]);
        GdkPixbuf* pix = gdk_cursor_get_image(cursor);
        gdk_cursor_unref(cursor);
        if (pix != NULL)
        {
            gtk_list_store_insert_with_values(store, &it, -1, 0, pix, -1);
            g_object_unref(pix);
        }
    }
    gtk_icon_view_set_model(GTK_ICON_VIEW(app.cursor_demo_view), GTK_TREE_MODEL(store));
    g_object_unref(store);

    /* gtk+ programs should reload named cursors correctly.
     * However, if the cursor is inherited from the root window,
     * gtk+ won't change it. So we need to update the cursor of root window.
     * Unfortunately, this doesn't work for non-gtk+ programs.
     * KDE programs seem to require special handling with XFixes */
    cursor = gdk_cursor_new(GDK_LEFT_PTR);
    i = gdk_display_get_n_screens(gdk_display_get_default());
    while(--i >= 0)
    {
        GdkScreen* screen = gdk_display_get_screen(gdk_display_get_default(), i);
        gdk_window_set_cursor(gdk_screen_get_root_window(screen), cursor);
    }
    gdk_cursor_unref(cursor);
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

        gtk_widget_set_sensitive(app.cursor_theme_remove_btn, theme->is_removable);
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
    int max_cursor_w, max_cursor_h, max_size;
    GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.cursor_theme_view));
    /* treeview and model are already set up in icon_theme_init() */
    g_signal_connect(sel, "changed", G_CALLBACK(on_cursor_theme_sel_changed), NULL);

    gdk_display_get_maximal_cursor_size(gdk_display_get_default(), &max_cursor_w, &max_cursor_h);
    max_size = MAX(max_cursor_w, max_cursor_h);

    /* FIXME: this isn't fully working... */
    app.cursor_size_range = GTK_WIDGET(gtk_builder_get_object(b, "cursor_size"));
    if(max_size < 128)
        gtk_range_set_range(GTK_RANGE(app.cursor_size_range), 1, max_size + 10); /* 10 is page size */
    gtk_range_set_value(GTK_RANGE(app.cursor_size_range), app.cursor_theme_size);
    g_signal_connect(app.cursor_size_range, "value-changed", G_CALLBACK(on_cursor_theme_size_changed), NULL);

    /* set up demo for cursors */
    app.cursor_demo_view = GTK_WIDGET(gtk_builder_get_object(b, "cursor_demo_view"));
    gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(app.cursor_demo_view), 0);
    update_cursor_demo();

    /* install and remove */
    /* this part is already done in icon-theme.c */
}
