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

#include "lxappearance2.h"
#include "color-scheme.h"
#include <string.h>

void color_scheme_init()
{
    color_scheme_update();
}

static gboolean gtkrc_supports_colors(const char* gtkrc_file, gboolean* support)
{
    char* content;
    gboolean support_colors = FALSE;
    if(g_file_get_contents(gtkrc_file, &content, NULL, NULL))
    {
        /* FIXME: check included gtkrc files, too. */
        if(strstr(content, "gtk-color-scheme"))
            support_colors = TRUE;
        g_free(content);
    }
    *support = support_colors;

    return TRUE;
}

/* check if current gtk+ widget theme supports color schemes */
void color_scheme_update()
{
    gboolean supported = FALSE;
    if(app.widget_theme)
    {
        gboolean file_found;
        char* gtkrc = g_build_filename(g_get_home_dir(), ".themes", app.widget_theme, "gtk-2.0/gtkrc", NULL);
        file_found = gtkrc_supports_colors(gtkrc, &supported);
        g_free(gtkrc);

        if(!file_found)
        {
            gtkrc = g_build_filename(gtk_rc_get_theme_dir(), app.widget_theme, "gtk-2.0/gtkrc", NULL);
            gtkrc_supports_colors(gtkrc, &supported);
            g_free(gtkrc);
        }
    }

    g_free(app.color_scheme);
    if(supported)
    {
        g_object_get(gtk_settings_get_default(), "gtk-color-scheme", &app.color_scheme, NULL);
        g_debug("color-scheme: %s", app.color_scheme);
        //gtk_widget_set_sensitive(, TRUE);
    }
    else
    {
        app.color_scheme = NULL;
        //gtk_widget_set_sensitive(, FALSE);
        g_debug("color-scheme is not supported by this theme");
    }
}

