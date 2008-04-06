#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* for kill & waitpid */
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#include "main-dlg.h"
#include "main-dlg-ui.h"
#include "glade-support.h"

#define GET_WIDGET( name )  name = lookup_widget( dlg, #name )

#define INIT_LIST(name, prop) \
    GET_WIDGET( name##_view ); \
    name##_list = init_tree_view( name##_view, G_CALLBACK(on_list_sel_changed), prop ); \
    load_##name##s( name##_list, name##_name );

#define enable_apply()      gtk_dialog_set_response_sensitive( main_dlg, GTK_RESPONSE_APPLY, TRUE )
#define disable_apply()      gtk_dialog_set_response_sensitive( main_dlg, GTK_RESPONSE_APPLY, FALSE )

extern gboolean under_lxde;	/* wether lxde-xsettings daemon is active */

extern GtkWidget* main_dlg; /* defined in main.c */

static GtkTreeView* gtk_theme_view = NULL;
static GtkListStore* gtk_theme_list = NULL;

static GtkTreeView* icon_theme_view = NULL;
static GtkListStore* icon_theme_list = NULL;

static char* gtk_theme_name = NULL;
static char* icon_theme_name = NULL;
static char* font_name = NULL;
static GtkToolbarStyle tb_style = GTK_TOOLBAR_BOTH;

extern char tmp_rc_file[];
static char* rc_file = NULL;

/*
static GtkTreeView* font_view = NULL;
static GtkListStore* font_list = NULL;
*/
static GtkWidget* demo_box = NULL;
static GtkWidget* demo_socket = NULL;
static GPid demo_pid = 0;

static void reload_demo_process()
{
    char* argv[5];
    char wid[16];

    if( demo_pid > 0 ) /* kill old demo */
    {
        int stat;
        kill( demo_pid, SIGTERM );
        waitpid( demo_pid, &stat, 0 );
        demo_pid = 0;
    }

    g_snprintf( wid, 16, "%ld", gtk_socket_get_id(demo_socket) );

    argv[0] = g_get_prgname();
    argv[1] = "demo";
    argv[2] = wid;
    argv[3] = tmp_rc_file;
    argv[4] = NULL;
    g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &demo_pid, NULL );

    /* reloading demo means the current selected theme is changed */
    enable_apply();
}

static void write_rc_file( const char* path )
{
    FILE* f;
    static char* tb_styles[] = {
        "GTK_TOOLBAR_ICONS",
        "GTK_TOOLBAR_TEXT",
        "GTK_TOOLBAR_BOTH",
        "GTK_TOOLBAR_BOTH_HORIZ"
    };

    if( f = fopen( path, "w" ) )
    {
        fputs( "# DO NOT EDIT!  This file will be overwritten by LXAppearance.\n"
                 "# Any customization should be done in ~/.gtkrc-2.0.mine\n\n", f );

        fprintf( f, "gtk-theme-name=\"%s\"\n", gtk_theme_name );
        fprintf( f, "gtk-icon-theme-name=\"%s\"\n", icon_theme_name );
        fprintf( f, "gtk-font-name=\"%s\"\n", font_name );
        fprintf( f, "gtk-toolbar-style=%d\n", tb_style );

        fprintf( f, "include \"%s/.gtkrc-2.0.mine\"\n", g_get_home_dir() );

        fclose( f );
    }
}

static void create_lxde_config_dir()
{
	char* dir = g_build_filename( g_get_user_config_dir(), "lxde", NULL );
#if GTK_CHECK_VERSION( 2, 8, 0 )
	g_mkdir_with_parents( dir, 0755 );
#else
	/* FIXME: implement mkdir -p for gtk+ < 2.8 */
#endif
	g_free( dir );
}

static void write_lxde_config()
{
    FILE* f;
	char* file, *data;
	gsize len;
	GKeyFile* kf = g_key_file_new();
	gboolean ret;

	file = g_build_filename( g_get_user_config_dir(), "lxde/config", NULL );
	ret = g_key_file_load_from_file( kf, file, G_KEY_FILE_KEEP_COMMENTS, NULL );

	if( ! ret )
	{
		const char** dir, **dirs = g_get_system_data_dirs();
		create_lxde_config_dir();
		/* load system-wide config file */
		for( dir = dirs; *dir; ++dir )
		{
			char* path = g_build_filename( *dir, "lxde/config", NULL );
			ret = g_key_file_load_from_file( kf, path, 0, NULL );
			g_free( path );
			if( ret )
				break;
		}
	}

	g_key_file_set_string( kf, "GTK", "sNet/ThemeName", gtk_theme_name );
	g_key_file_set_string( kf, "GTK", "sNet/IconThemeName", icon_theme_name );
	g_key_file_set_string( kf, "GTK", "sGtk/FontName", font_name );
	g_key_file_set_integer( kf, "GTK", "iGtk/ToolbarStyle", tb_style );

	data = g_key_file_to_data( kf, &len, NULL );
	g_key_file_free( kf );

    if( f = fopen( file, "w" ) )
    {
		fwrite( data, sizeof(char), len, f );
        fclose( f );
    }
    g_free( data );
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

            if( under_lxde )
				g_object_set( gtk_settings_get_default(), "gtk-theme-name", name, NULL );
        }
        else if( model == icon_theme_list )   /* icon theme */
        {
            if( name && icon_theme_name && 0 == strcmp( name, icon_theme_name ) )
                goto out;
            g_free( icon_theme_name );
            icon_theme_name = name;

            if( under_lxde )
				g_object_set( gtk_settings_get_default(), "gtk-icon-theme-name", name, NULL );
        }

		if( under_lxde )
		{
			enable_apply();
		}
		else
		{
			write_rc_file( tmp_rc_file );
			//gtk_rc_reparse_all_for_settings(gtk_settings_get_default(), TRUE);
			reload_demo_process();
		}

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

static void load_gtk_themes( GtkListStore* list, const char* cur_sel )
{
    char* path;
    GtkTreeSelection* sel = gtk_tree_view_get_selection( gtk_theme_view );
    load_from_data_dirs( list, "themes", "gtk-2.0", sel, cur_sel );
    path = g_build_filename( g_get_home_dir(), ".themes", NULL );
    load_themes_from_dir( list, path, "gtk-2.0", sel, cur_sel );
    g_free( path );
}

static void load_icon_themes( GtkListStore* list, const char* cur_sel )
{
    char* path;
    GtkTreeSelection* sel = gtk_tree_view_get_selection( icon_theme_view );
    load_from_data_dirs( list, "icons", "index.theme", sel, cur_sel );
    path = g_build_filename( g_get_home_dir(), ".icons", NULL );
    load_themes_from_dir( list, path, "index.theme", sel, cur_sel );
    g_free( path );
}

/*
static void load_fonts( GtkListStore* list )
{

}
*/

gboolean center_win( GtkWindow* dlg )
{
    gtk_widget_show( dlg );
    return FALSE;
}

static void on_demo_loaded( GtkSocket* socket, GtkWidget* dlg )
{
    /* sleep for 0.8 sec for loading of the demo window */
    /* FIXME: we need a better way to do this, such as IPC */
    g_timeout_add_full( G_PRIORITY_LOW, 800, (GSourceFunc)center_win, dlg, NULL );
    g_signal_handlers_disconnect_by_func( socket, on_demo_loaded, dlg );
}

void main_dlg_init( GtkWidget* dlg )
{
    char* files[] = { tmp_rc_file, NULL };
    char** def_files = gtk_rc_get_default_files();
    char** file;

	/* no lxde-settings daemon, use gtkrc-2.0 */
	if( ! under_lxde )
	{
		for( file = def_files; *file; ++file )
		{
			if( 0 == access( *file, W_OK ) )
				rc_file = *file;
		}
		if( rc_file )
			rc_file = g_strdup( rc_file );
		else
			rc_file = g_build_filename( g_get_home_dir(), ".gtkrc-2.0", NULL );
	}

    g_object_get( gtk_settings_get_default(),
                        "gtk-theme-name", &gtk_theme_name,
                        "gtk-icon-theme-name", &icon_theme_name,
                        "gtk-font-name", &font_name,
                        "gtk-toolbar-style", &tb_style,
                        NULL );

    if(  ! gtk_theme_name )
        gtk_theme_name = g_strdup( "Raleigh" );
    if(  ! icon_theme_name )
        gtk_theme_name = g_strdup( "hicolor" );
    if( ! font_name )
        font_name = g_strdup( "Sans 10" );

	/* no lxde-settings daemon, use gtkrc-2.0 */
	if( ! under_lxde )
		write_rc_file( tmp_rc_file );

    INIT_LIST( gtk_theme, "gtk-theme-name" )
    INIT_LIST( icon_theme, "gtk-icon-theme-name" )
    gtk_font_button_set_font_name( (GtkFontButton*)lookup_widget(dlg, "font"), font_name );

    gtk_combo_box_set_active( (GtkComboBox*)lookup_widget(dlg, "tb_style"), tb_style < 4 ? tb_style : 3 );

    GET_WIDGET( demo_box );
    gtk_widget_show( demo_box );

	if( under_lxde )
	{
		/* XSettings daemon of LXDE is running, gtkrc is useless.
		 * 	We should set properties of GtkSettings object on the fly.
		 * 	This will cause problems with some themes, but we have no choice.
		 */
		show_demo( (GdkNativeWindow)demo_box );
		gtk_widget_show_all( dlg );
	}
	else
	{
		/* no lxde-settings daemon, use gtkrc-2.0 and load preview in another process */
		demo_socket = gtk_socket_new();
		g_signal_connect( demo_socket, "plug-added", G_CALLBACK(on_demo_loaded), dlg );
		g_signal_connect( demo_socket, "plug-removed", G_CALLBACK(gtk_true), NULL );
		gtk_widget_show( demo_socket );
		gtk_container_add( (GtkContainer*)demo_box, demo_socket );

		gtk_widget_realize( dlg );
		reload_demo_process();
	}

    disable_apply();
}

static void reload_all_programs( gboolean icon_only )
{
    GdkEventClient event;
    event.type = GDK_CLIENT_EVENT;
    event.send_event = TRUE;
    event.window = NULL;

	if( under_lxde )
	{
		event.message_type = gdk_atom_intern("LXDE_SETTINGS", FALSE);
		event.data.b[0] = 0;	/* LXS_RELOAD */
	}
	else
	{
		if( icon_only )
			event.message_type = gdk_atom_intern("_GTK_LOAD_ICONTHEMES", FALSE);
		else
			event.message_type = gdk_atom_intern("_GTK_READ_RCFILES", FALSE);
	}
	event.data_format = 8;
	gdk_event_send_clientmessage_toall((GdkEvent *)&event);
}

void
on_apply_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
	if( under_lxde )
		write_lxde_config();
	else
		write_rc_file( rc_file );

	reload_all_programs( FALSE );
    disable_apply();
}


void
on_font_changed                        (GtkFontButton   *fontbutton,
                                        gpointer         user_data)
{
    const char* name = gtk_font_button_get_font_name(fontbutton);
    if( name && font_name && 0 == strcmp( name, font_name ) )
        return;
    g_free( font_name );
    font_name = g_strdup( name );

	if( under_lxde )
	{
		g_object_set( gtk_settings_get_default(), "gtk-font-name", font_name, NULL );
		
		enable_apply();
	}
	else
	{
		write_rc_file( tmp_rc_file );
		reload_demo_process();
	}
}

void
on_install_theme_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkFileFilter* filter = gtk_file_filter_new();
    GtkWidget* fc = gtk_file_chooser_dialog_new( _("Select an icon theme"), NULL,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL );

    gtk_file_filter_add_pattern( filter, "*.tar.gz" );
    gtk_file_filter_add_pattern( filter, "*.tar.bz2" );
    gtk_file_filter_set_name( filter, _("*.tar.gz, *.tar.bz2 (Icon Theme)") );

    gtk_file_chooser_add_filter( fc, filter );
    gtk_file_chooser_set_filter( (GtkFileChooser*)fc, filter );

    if( gtk_dialog_run( (GtkDialog*)fc ) == GTK_RESPONSE_OK )
    {
        char* file = gtk_file_chooser_get_filename( fc );
        char* argv[]={
            PACKAGE_DATA_DIR"/lxappearance/install-icon-theme.sh",
            file, NULL };
        int status = 0;
        char* stdo = NULL;
        if( g_spawn_sync( NULL, argv, NULL, 0, NULL, NULL, &stdo, NULL, &status, NULL ) && 0 == status )
        {
            char* sep = stdo ? strchr( stdo, '\n' ) : NULL;
            if( sep )
                *sep = '\0';

            /* reload all icon themes */
            gtk_list_store_clear( icon_theme_list );
            load_icon_themes( icon_theme_list, stdo ? stdo : "" );
        }
        g_free( file );
    }

    gtk_widget_destroy( fc );
}


void
on_remove_theme_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_tb_style_changed                    (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int sel = gtk_combo_box_get_active( combobox );
    if( sel == tb_style || sel < 0 )
        return;
    tb_style = sel;

	if( under_lxde )
	{
		g_object_set( gtk_settings_get_default(), "gtk-toolbar-style", tb_style, NULL );
		enable_apply();
	}
	else
	{
		write_rc_file( tmp_rc_file );
		reload_demo_process();
	}
}

