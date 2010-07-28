/*
 *      lxappearance2.h
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

#ifndef _LXAPPEARANCE2_H_
#define _LXAPPEARANCE2_H_

#include <gtk/gtk.h>

typedef struct _LXAppearance    LXAppearance;
struct _LXAppearance
{
    GtkWidget* dlg;
    GtkWidget* notebook;

    GtkWidget* widget_theme_view;
    GtkListStore* widget_theme_store;
    GtkWidget* default_font_btn;

    GtkWidget* icon_theme_view;
    GtkListStore* icon_theme_store;

    GtkWidget* cursor_theme_view;
    GtkWidget* cursor_demo_view;
    GtkListStore* cursor_theme_store;
    GtkWidget* cursor_size_range;

    GSList* icon_themes; /* a list of IconTheme struct */

    GtkWidget* tb_style_combo;
    GtkWidget* tb_icon_size_combo;

    char* widget_theme;
    char* default_font;
    char* icon_theme;
    char* cursor_theme;
    int cursor_theme_size;
    char* color_scheme;
    char* icon_sizes;
    int toolbar_style;
    int toolbar_icon_size;
    gboolean changed;
    gboolean use_lxsession;
};

extern LXAppearance app;

void lxappearance_changed();

#endif
