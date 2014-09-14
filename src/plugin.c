//      plugin.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "plugin.h"
#include "lxappearance.h"
#include <gmodule.h>

#define PLUGIN_DIR  PACKAGE_LIB_DIR"/lxappearance/plugins"

typedef gboolean (*PluginLoadFunc)(LXAppearance*, GtkBuilder*);
typedef void (*PluginUnloadFunc)(LXAppearance*);

typedef struct _Plugin Plugin;
struct _Plugin
{
    GModule* module;
    PluginLoadFunc load;
    PluginUnloadFunc unload;
};

static GSList* plugins = NULL;

void plugins_init(GtkBuilder* builder)
{
    GDir* dir = g_dir_open(PLUGIN_DIR, 0, NULL);
    const char* name = NULL;
    if(!dir)
        return;
    while ((name = g_dir_read_name(dir)))
    {
        if(g_str_has_suffix(name, ".so"))
        {
            char* file = g_build_filename(PLUGIN_DIR, name, NULL);
            GModule* mod = g_module_open(file, 0);
            g_free(file);
            if(mod)
            {
                PluginLoadFunc load;
                gboolean loaded = FALSE;
                g_debug("module: %s", g_module_name(mod));
                if(g_module_symbol(mod, "plugin_load", (gpointer*)&load))
                    loaded = load(&app, builder);
                if(loaded)
                {
                    Plugin* plugin = g_slice_new0(Plugin);
                    plugin->module = mod;
                    plugin->load = load;
                    g_module_symbol(mod, "plugin_unload", (gpointer*)&plugin->unload);
                    plugins = g_slist_prepend(plugins, plugin);
                }
                else
                    g_module_close(mod);
            }
            else
                g_debug("open failed: %s\n%s", name, g_module_error());
        }
    }
    g_dir_close(dir);
}

void plugins_finalize()
{
    GSList* l;
    for(l = plugins; l; l=l->next)
    {
        Plugin* plugin = (Plugin*)l->data;
        if(plugin->unload)
            plugin->unload(&app);
        g_module_close(plugin->module);
        g_slice_free(Plugin, plugin);
    }
    g_slist_free(plugins);
}
