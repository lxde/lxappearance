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

    /* gtk theme */
    GtkWidget* widget_theme_view;
    GtkListStore* widget_theme_store;
    GtkWidget* default_font_btn;

    /* color scheme */
    GtkWidget* color_table;
    GtkWidget* custom_colors;
    GtkWidget* no_custom_colors;
    GHashTable* color_scheme_hash; /* the custom color scheme set by the user */
    GHashTable* default_color_scheme_hash; /* default colors of current gtk theme */
    gboolean color_scheme_supported; /* if color scheme is supported by current gtk theme */
    /* color buttons in color scheme page. */
    GtkWidget* color_btns[8]; /* FIXME: this value might be changed in the future */

    /* icon theme */
    GtkWidget* icon_theme_view;
    GtkListStore* icon_theme_store;
    GtkWidget* icon_theme_remove_btn;

    /* cursor theme */
    GtkWidget* cursor_theme_view;
    GtkWidget* cursor_demo_view;
    GtkListStore* cursor_theme_store;
    GtkWidget* cursor_size_range;
    GtkWidget* cursor_theme_remove_btn;

    GSList* icon_themes; /* a list of IconTheme struct representing all icon and cursor themes */

    /* toolbar style and icon size */
    GtkWidget* tb_style_combo;
    GtkWidget* tb_icon_size_combo;

    char* widget_theme;
    char* default_font;
    char* icon_theme;
    char* cursor_theme;
    int cursor_theme_size;
    char* color_scheme;
    int toolbar_style;
    int toolbar_icon_size;

#if GTK_CHECK_VERSION(2, 14, 0)
    GtkWidget* event_sound_check;
    GtkWidget* input_feedback_check;
    gboolean enable_event_sound;
    gboolean enable_input_feedback;
#endif

    gboolean changed;
    gboolean use_lxsession;
};

extern LXAppearance app;

void lxappearance_changed();

#endif
