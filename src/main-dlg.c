#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "main-dlg.h"
#include "main-dlg-ui.h"
#include "glade-support.h"

#define GET_WIDGET( name )  name = lookup_widget( dlg, #name )
#define INIT_LIST(name, prop) \
    GET_WIDGET( name##_view ); \
    name##_list = init_tree_view( name##_view, G_CALLBACK(on_list_sel_changed), prop ); \
    load_##name##s( name##_list );

static GtkTreeView* gtk_theme_view = NULL;
static GtkListStore* gtk_theme_list = NULL;

static GtkTreeView* icon_theme_view = NULL;
static GtkListStore* icon_theme_list = NULL;

static GtkTreeView* font_view = NULL;
static GtkListStore* font_list = NULL;

static GtkIconView* icon_view = NULL;

static void reload_demo_icons()
{
    static const char** icon_names[]={
        "gnome-fs-home",
        "gnome-fs-desktop",
        "gnome-fs-directory",
        "gnome-fs-trash-empty",
        "gnome-fs-regular",
        "gnome-fs-executable",
        "gnome-mime-image",
        "gnome-mime-text"
    };

    int i;
    GtkIconTheme* theme = gtk_icon_theme_get_default();
    GtkListStore* demo_icon_list;

    demo_icon_list = gtk_list_store_new( 2, GDK_TYPE_PIXBUF, G_TYPE_STRING );

    for( i = 0; i < G_N_ELEMENTS(icon_names); ++i )
    {
        GtkTreeIter it;
        GdkPixbuf* icon = gtk_icon_theme_load_icon( theme, icon_names[i], 32, 0, NULL );
        gtk_list_store_append( demo_icon_list, &it );
        gtk_list_store_set( demo_icon_list, &it, 0, icon, 1, icon_names[i], -1 );
        if( icon )
            g_object_unref( icon );
    }
    gtk_icon_view_set_model( icon_view, demo_icon_list );
    g_object_unref( demo_icon_list );
}

static void reload_theme()
{

    gtk_rc_reparse_all();
}

static void on_list_sel_changed( GtkTreeSelection* sel, const char* prop )
{
    GtkTreeIter it;
    GtkTreeModel* model;
    if( gtk_tree_selection_get_selected( sel, &model, &it ) )
    {
        char* name;
        gtk_tree_model_get( model, &it, 0, &name, -1 );
        if( model == font_list )    /* this is a font */
        {

        }
        g_object_set( gtk_settings_get_default(), prop, name, NULL );
        g_free( name );

        if( model == icon_theme_list ) /* icon theme is changed */
            reload_demo_icons();
    }
}

static GtkListStore* init_tree_view( GtkTreeView* view, GCallback on_sel_changed, const char* prop )
{
    GtkTreeViewColumn* col;
    GtkListStore* list;
    col = gtk_tree_view_column_new_with_attributes( NULL, gtk_cell_renderer_text_new(), "text", 0, NULL );
    gtk_tree_view_append_column( view, col );
    g_signal_connect( gtk_tree_view_get_selection(view), "changed", on_sel_changed, prop );

    list = gtk_list_store_new( 1, G_TYPE_STRING );
    gtk_tree_view_set_model( view, (GtkTreeModel*)list );
    g_object_unref( list );
    return list;
}

static void load_themes_from_dir( GtkListStore* list,
                                                    const char* dir_path,
                                                    const char* lookup )
{
    GDir* dir;
    if( dir = g_dir_open( dir_path, 0, NULL ) )
    {
        char* name;
        while( name = g_dir_read_name( dir ) )
        {
            char* file = g_build_filename( dir_path, name, lookup, NULL );
            if( g_file_test( file, G_FILE_TEST_EXISTS ) )
            {
                GtkTreeIter it;
                gtk_list_store_append( list, &it );
                gtk_list_store_set( list, &it, 0, name, -1 );
            }
            g_free( file );
        }
        g_dir_close( dir );
    }
}

static void load_from_data_dirs( GtkListStore* list,
                                                const char* relative_path,
                                                const char* lookup )
{
    const char* const *dirs = g_get_system_data_dirs();
    const char* const *dir;
    char* dir_path;
    for( dir = dirs; *dir; ++dir )
    {
        dir_path = g_build_filename( *dir, relative_path, NULL );
        load_themes_from_dir( list, dir_path, lookup );
        g_free( dir_path );
    }
    dir_path = g_build_filename( g_get_user_data_dir(), relative_path, NULL );
    load_themes_from_dir( list, dir_path, lookup );
    g_free( dir_path );
}

static void load_gtk_themes( GtkListStore* list )
{
    load_from_data_dirs( list, "themes", "gtk-2.0" );
}

static void load_icon_themes( GtkListStore* list )
{
    char* path;
    load_from_data_dirs( list, "icons", "index.theme" );
    path = g_build_filename( g_get_home_dir(), ".icons", NULL );
    load_themes_from_dir( list, path, "index.theme" );
    g_free( path );
}

static void load_fonts( GtkListStore* list )
{

}

void main_dlg_init( GtkWidget* dlg )
{
    INIT_LIST( gtk_theme, "gtk-theme-name" )
    INIT_LIST( icon_theme, "gtk-icon-theme-name" )
    INIT_LIST( font, "gtk-font-name" )

    GET_WIDGET( icon_view );
    gtk_icon_view_set_pixbuf_column( icon_view, 0 );
    gtk_icon_view_set_text_column( icon_view, 1 );
    gtk_icon_view_set_item_width( icon_view, 64 );
    gtk_icon_view_set_column_spacing( icon_view, 8 );
    gtk_icon_view_set_row_spacing( icon_view, 8 );
    reload_demo_icons();
}

void
on_apply_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{

}

