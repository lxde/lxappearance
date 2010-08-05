//      color-scheme.h
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


#ifndef __COLOR_SCHEME_H__
#define __COLOR_SCHEME_H__

G_BEGIN_DECLS

/* initialize gtk color scheme support. */
void color_scheme_init(GtkBuilder* b);

/* update the color scheme page for currently selected gtk theme.
 * should be called when currently selected gtk theme gets changed. */
void color_scheme_update();

/* load gtk-color-scheme from gtkrc file into hash table if it's available. */
gboolean gtkrc_file_get_color_scheme(const char* gtkrc_file, GHashTable* hash);

/* convert a color scheme hash table to string */
char* color_scheme_hash_to_str(GHashTable* hash);

/* merge a color scheme string to hash table. */
void color_scheme_str_to_hash(GHashTable* hash, const char* color_str);

G_END_DECLS

#endif /* __COLOR_SCHEME_H__ */
