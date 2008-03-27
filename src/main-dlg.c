#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "main-dlg.h"
#include "main-dlg-ui.h"
#include "glade-support.h"

/* for kill & waitpid */
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#define GET_WIDGET( name )  name = lookup_widget( dlg, #name )
#define INIT_LIST(name, prop) \
    GET_WIDGET( name##_view ); \
    name##_list = init_tree_view( name##_view, G_CALLBACK(on_list_sel_changed), prop ); \
    load_##name##s( name##_list );

static GtkTreeView* gtk_theme_view = NULL;
static GtkListStore* gtk_theme_list = NULL;

static GtkTreeView* icon_theme_view = NULL;
static GtkListStore* icon_theme_list = NULL;

static char* gtk_theme_name = NULL;
static char* icon_theme_name = NULL;
static char* font_name = NULL;

static char tmp_rc_file[] = "/tmp/gtkrc-2.0-XXXXXX";
static char* rc_file = NULL;

/*
static GtkTreeView* font_view = NULL;
static GtkListStore* font_list = NULL;
*/
static GtkWidget* demo_box = NULL;
static GtkWidget* demo_socket = NULL;
static GPid demo_pid = 0;

static void load_demo_process()
{
    char* argv[5];
    char wid[16];

    if( demo_pid > 0 ) /* kill old demo */
    {
        int stat;
//        if( demo_socket )
//            gtk_widget_destroy( demo_socket );
        kill( demo_pid, SIGTERM );
        waitpid( demo_pid, &stat, 0 );
        demo_pid = 0;
    }

    if( !demo_socket )
    {
        demo_socket = gtk_socket_new();
        g_signal_connect( demo_socket, "plug-removed", G_CALLBACK(gtk_true), NULL );
        gtk_widget_show( demo_socket );
        gtk_container_add( (GtkContainer*)demo_box, demo_socket );
    }

    g_snprintf( wid, 16, "%ld", gtk_socket_get_id(demo_socket) );

    argv[0] = g_get_prgname();
    argv[1] = "demo";
    argv[2] = wid;
    argv[3] = tmp_rc_file;
    argv[4] = NULL;
    g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &demo_pid, NULL );
}

static void write_rc_file( const char* path )
{
    FILE* f;
    if( f = fopen( path, "w" ) )
    {
        fputs( "# DO NOT EDIT!  This file will be overwritten by LXAppearance.\n"
                 "# Any customization should be done in ~/.gtkrc-2.0.mine\n\n", f );

        fprintf( f, "gtk-theme-name=\"%s\"\n", gtk_theme_name );
        fprintf( f, "gtk-icon-theme-name=\"%s\"\n", icon_theme_name );
        fprintf( f, "gtk-font-name=\"%s\"\n", font_name );
        fprintf( f, "include \"%s/.gtkrc-2.0.mine\"\n", g_get_home_dir() );

        fclose( f );
    }
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

        if( model == gtk_theme_list )   /* gtk+ theme */
        {
            if( name && gtk_theme_name && 0 == strcmp( name, gtk_theme_name ) )
                goto out;
            g_free( gtk_theme_name );
            gtk_theme_name = name;
        }
        else if( model == icon_theme_list )   /* icon theme */
        {
            if( name && icon_theme_name && 0 == strcmp( name, icon_theme_name ) )
                goto out;
            g_free( icon_theme_name );
            icon_theme_name = name;
        }
        write_rc_file( tmp_rc_file );
        //gtk_rc_reparse_all_for_settings(gtk_settings_get_default(), TRUE);
        load_demo_process();
        return;
    out:
        g_free( name );
    }
}

static GtkListStore* init_tree_view( GtkTreeView* view, GCallback on_sel_changed, const char* prop )
{
    GtkTreeViewColumn* col;
    GtkListStore* list;
    GtkTreeSelection* sel;
    col = gtk_tree_view_column_new_with_attributes( NULL, gtk_cell_renderer_text_new(), "text", 0, NULL );
    gtk_tree_view_append_column( view, col );
    sel = gtk_tree_view_get_selection(view);
    g_signal_connect( sel, "changed", on_sel_changed, prop );

    list = gtk_list_store_new( 1, G_TYPE_STRING );
    gtk_tree_view_set_model( view, (GtkTreeModel*)list );
    g_object_unref( list );
    return list;
}

static void load_themes_from_dir( GtkListStore* list,
                                                    const char* dir_path,
                                                    const char* lookup,
                                                    GtkTreeSelection* sel,
                                                    const char* init_sel )
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
                if( 0 == strcmp( name, init_sel ) )
                {
                    GtkTreeView* view;
                    GtkTreePath* tp;
                    gtk_tree_selection_select_iter( sel, &it );
                    view = gtk_tree_selection_get_tree_view( sel );
                    tp = gtk_tree_model_get_path( (GtkTreeModel*)list, &it );
                    gtk_tree_view_scroll_to_cell( view, tp, NULL, FALSE, 0, 0 );
                    gtk_tree_path_free( tp );
                }
            }
            g_free( file );
        }
        g_dir_close( dir );
    }
}

static void load_from_data_dirs( GtkListStore* list,
                                                const char* relative_path,
                                                const char* lookup,
                                                GtkTreeSelection* sel,
                                                const char* init_sel  )
{
    const char* const *dirs = g_get_system_data_dirs();
    const char* const *dir;
    char* dir_path;
    for( dir = dirs; *dir; ++dir )
    {
        dir_path = g_build_filename( *dir, relative_path, NULL );
        load_themes_from_dir( list, dir_path, lookup, sel, init_sel );
        g_free( dir_path );
    }
    dir_path = g_build_filename( g_get_user_data_dir(), relative_path, NULL );
    load_themes_from_dir( list, dir_path, lookup, sel, init_sel );
    g_free( dir_path );
}

static void load_gtk_themes( GtkListStore* list )
{
    GtkTreeSelection* sel = gtk_tree_view_get_selection( gtk_theme_view );
    load_from_data_dirs( list, "themes", "gtk-2.0", sel, gtk_theme_name );
}

static void load_icon_themes( GtkListStore* list )
{
    char* path;
    GtkTreeSelection* sel = gtk_tree_view_get_selection( icon_theme_view );
    load_from_data_dirs( list, "icons", "index.theme", sel, icon_theme_name );
    path = g_build_filename( g_get_home_dir(), ".icons", NULL );
    load_themes_from_dir( list, path, "index.theme", sel, icon_theme_name );
    g_free( path );
}

/*
static void load_fonts( GtkListStore* list )
{

}
*/

void main_dlg_init( GtkWidget* dlg )
{
    char* files[] = { tmp_rc_file, NULL };
    char** def_files = gtk_rc_get_default_files();
    char** file;

    for( file = def_files; *file; ++file )
    {
        if( 0 == access( *file, W_OK ) )
            rc_file = *file;
    }
    if( rc_file )
        rc_file = g_strdup( rc_file );
    else
        rc_file = g_build_filename( g_get_home_dir(), ".gtkrc-2.0", NULL );
    mkstemp( tmp_rc_file );

    g_object_get( gtk_settings_get_default(), "gtk-theme-name", &gtk_theme_name, NULL );
    if(  ! gtk_theme_name )
        gtk_theme_name = g_strdup( "Raleigh" );
    g_object_get( gtk_settings_get_default(), "gtk-icon-theme-name", &icon_theme_name, NULL );
    if(  ! icon_theme_name )
        gtk_theme_name = g_strdup( "hicolor" );
    g_object_get( gtk_settings_get_default(), "gtk-font-name", &font_name, NULL );
    if( ! font_name )
        font_name = g_strdup( "Sans 10" );

    write_rc_file( tmp_rc_file );

    INIT_LIST( gtk_theme, "gtk-theme-name" )
    INIT_LIST( icon_theme, "gtk-icon-theme-name" )
    gtk_font_button_set_font_name( (GtkFontButton*)lookup_widget(dlg, "font"), font_name );

    /* INIT_LIST( font, "gtk-font-name" ) */

    GET_WIDGET( demo_box );

    gtk_widget_show( demo_box );
//    gtk_widget_set_app_paintable( demo_socket, TRUE );
    gtk_widget_realize( dlg );
    load_demo_process();
}

static void reload_all_programs( gboolean icon_only )
{
    GdkEventClient event;
    event.type = GDK_CLIENT_EVENT;
    event.send_event = TRUE;
    event.window = NULL;

    if( icon_only )
        event.message_type = gdk_atom_intern("_GTK_LOAD_ICONTHEMES", FALSE);
    else
        event.message_type = gdk_atom_intern("_GTK_READ_RCFILES", FALSE);

    event.data_format = 8;
    gdk_event_send_clientmessage_toall((GdkEvent *)&event);
}

void
on_apply_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
    write_rc_file( rc_file );
    reload_all_programs( FALSE );
}


void
on_font_changed                        (GtkFontButton   *fontbutton,
                                        gpointer         user_data)
{
    const char* name = gtk_font_button_get_font_name(fontbutton);
    if( name && font_name && 0 == strcmp( font_name ) )
        return;
    g_free( font_name );
    font_name = g_strdup( name );
    write_rc_file( tmp_rc_file );
    load_demo_process();
}

