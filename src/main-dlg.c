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

enum {
    COL_DISP_NAME,
    COL_NAME,
    N_COLS
};

#define GET_WIDGET_WITH_TYPE(name, type) name = type( lookup_widget( dlg, #name ))

#define INIT_LIST(name, prop) \
    GET_WIDGET_WITH_TYPE( name##_view, GTK_TREE_VIEW ); \
    name##_list = init_tree_view( name##_view, G_CALLBACK(on_list_sel_changed), prop); \
    load_##name##s( name##_list, name##_name );

#define enable_apply()      gtk_dialog_set_response_sensitive(GTK_DIALOG (main_dlg), GTK_RESPONSE_APPLY, TRUE )
#define disable_apply()      gtk_dialog_set_response_sensitive(GTK_DIALOG (main_dlg), GTK_RESPONSE_APPLY, FALSE )

extern gboolean under_lxsession;	/* wether we are under lxsession */

extern GtkWidget* main_dlg; /* defined in main.c */

static GtkTreeView* gtk_theme_view = NULL;
static GtkListStore* gtk_theme_list = NULL;

static GtkTreeView* icon_theme_view = NULL;
static GtkListStore* icon_theme_list = NULL;

static GtkTreeView* cursor_theme_view = NULL;
static GtkListStore* cursor_theme_list = NULL;

static char* gtk_theme_name = NULL;
static char* icon_theme_name = NULL;
static char* font_name = NULL;
static char* cursor_theme_name = NULL;
static gint cursor_theme_size = 0;
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

static const char* session_name = NULL;

extern Atom lxsession_atom; /* defined in main.c */

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

    g_snprintf( wid, 16, "%ld", gtk_socket_get_id(GTK_SOCKET (demo_socket)) );

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
        fprintf( f, "gtk-cursor-theme-name=\"%s\"\n", cursor_theme_name );
        fprintf( f, "gtk-cursor-theme-size=%d\n", cursor_theme_size );

        fprintf( f, "include \"%s/.gtkrc-2.0.mine\"\n", g_get_home_dir() );

        fclose( f );
    }
}

static void create_lxsession_config_dir()
{
	char* dir = g_build_filename( g_get_user_config_dir(), "lxsession", session_name, NULL );
	g_mkdir_with_parents( dir, 0755 );
	g_free( dir );
}

static void write_lxsession_config()
{
    FILE* f;
    char* file, *data;
    gsize len;
    GKeyFile* kf = g_key_file_new();
    gboolean ret;

    file = g_build_filename( g_get_user_config_dir(), "lxsession", session_name, "desktop.conf", NULL );
    ret = g_key_file_load_from_file( kf, file, G_KEY_FILE_KEEP_COMMENTS, NULL );

    if( ! ret )
    {
    	const gchar* const * dir;
    	const gchar* const * dirs = g_get_system_config_dirs();
    	create_lxsession_config_dir();

    	/* load system-wide config file */
    	for( dir = dirs; *dir; ++dir )
    	{
    		char* path = g_build_filename( *dir, "lxsession", session_name, "desktop.conf", NULL );
    		ret = g_key_file_load_from_file( kf, path, G_KEY_FILE_KEEP_COMMENTS, NULL );
    		g_free( path );
    		if( ret )
    			break;
    	}
    }

    g_key_file_set_string( kf, "GTK", "sNet/ThemeName", gtk_theme_name );
    g_key_file_set_string( kf, "GTK", "sNet/IconThemeName", icon_theme_name );
    g_key_file_set_string( kf, "GTK", "sGtk/FontName", font_name );
    g_key_file_set_integer( kf, "GTK", "iGtk/ToolbarStyle", tb_style );
    g_key_file_set_string( kf, "GTK", "sGtk/CursorThemeName", cursor_theme_name );
    g_key_file_set_integer( kf, "GTK", "iGtk/CursorThemeSize", cursor_theme_size );

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
        gtk_tree_model_get( model, &it, COL_NAME, &name, -1 );

        if( model == GTK_TREE_MODEL (gtk_theme_list) )   /* gtk+ theme */
        {
            if( name && gtk_theme_name && 0 == strcmp( name, gtk_theme_name ) )
                goto out;
            g_free( gtk_theme_name );
            gtk_theme_name = name;

            if( under_lxsession )
				g_object_set( gtk_settings_get_default(), "gtk-theme-name", name, NULL );
        }
        else if( model == GTK_TREE_MODEL (icon_theme_list) )   /* icon theme */
        {
            if( name && icon_theme_name && 0 == strcmp( name, icon_theme_name ) )
                goto out;
            g_free( icon_theme_name );
            icon_theme_name = name;

            if( under_lxsession )
				g_object_set( gtk_settings_get_default(), "gtk-icon-theme-name", name, NULL );
        }
        else if( model == GTK_TREE_MODEL (cursor_theme_list) )   /* cursor theme */
        {
            if( name && cursor_theme_name && 0 == strcmp( name, cursor_theme_name ) )
                goto out;
            g_free( cursor_theme_name );
            cursor_theme_name = g_strdup(name);
	    //gdk_x11_display_set_cursor_theme ( gdk_display_get_default (), name, cursor_theme_size );

            if( under_lxsession )
				g_object_set( gtk_settings_get_default(), "gtk-cursor-theme-name", name, NULL );
        }

	if( under_lxsession )
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

static gint sort_func( GtkTreeModel* model, GtkTreeIter* it1, GtkTreeIter* it2, gpointer data )
{
	char* str1, *str2;
	int ret;
	gtk_tree_model_get( model, it1, COL_DISP_NAME, &str1, -1 );
	gtk_tree_model_get( model, it2, COL_DISP_NAME, &str2, -1 );
	ret = g_utf8_collate( str1, str2 );
	g_free( str1 );
	g_free( str2 );
	return ret;
}

static GtkListStore* init_tree_view( GtkTreeView* view, GCallback on_sel_changed, const char* prop )
{
    GtkTreeViewColumn* col;
    GtkListStore* list;
    GtkTreeSelection* sel;
    int text_col = strcmp(prop, "gtk-theme-name") && strcmp(prop, "gtk-cursor-theme-name") ? COL_DISP_NAME : COL_NAME;

    col = gtk_tree_view_column_new_with_attributes( NULL, gtk_cell_renderer_text_new(),
                                                    "text", text_col, NULL );
    gtk_tree_view_append_column( view, col );

    sel = gtk_tree_view_get_selection(view);
    g_signal_connect( sel, "changed", on_sel_changed, GINT_TO_POINTER(prop) );

    list = gtk_list_store_new( N_COLS, G_TYPE_STRING, G_TYPE_STRING );
    gtk_tree_sortable_set_sort_func( (GtkTreeSortable*)list, text_col, sort_func, NULL, NULL );
    gtk_tree_view_set_model( view, (GtkTreeModel*)list );
    g_object_unref( list );
    return list;
}

typedef gboolean (*ThemeFunc)(const char* file, const char* dir, const char* name, GtkListStore* list, GtkTreeIter* it);

static void load_themes_from_dir( GtkListStore* list,
                                  const char* dir_path,
                                  const char* lookup,
                                  GtkTreeSelection* sel,
                                  const char* init_sel,
                                  ThemeFunc theme_func )
{
    GDir* dir;
    if( dir = g_dir_open( dir_path, 0, NULL ) )
    {
        const char* name;
        while( name = g_dir_read_name( dir ) )
        {
            char* file = g_build_filename( dir_path, name, lookup, NULL );
            if( g_file_test( file, G_FILE_TEST_EXISTS ) )
            {
                gboolean add = TRUE;
                GtkTreeIter it;

                /* prevent duplication */
                if( gtk_tree_model_get_iter_first(GTK_TREE_MODEL (list), &it ) )
                {
                    char* _name;
                    do {
                        _name = NULL;
                        gtk_tree_model_get(GTK_TREE_MODEL (list), &it, COL_NAME, &_name, -1);
                        if( _name && strcmp(_name, name) == 0 )
                        {
                            add = FALSE;
                            g_free(_name);
                            break;
                        }
                        g_free(_name);
                    }
                    while( gtk_tree_model_iter_next(GTK_TREE_MODEL (list), &it ) );
                }

                if( add )
                {
                    gtk_list_store_append( list, &it );
                    gtk_list_store_set( list, &it, COL_NAME, name, -1 );

                    if( theme_func )
                    {
                        if( ! theme_func(file, (char *) dir, name, list, &it) )
                            add = FALSE;
                    }

                    if( add )
                    {
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
                    else
                        gtk_list_store_remove( list, &it );
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
                                 const char* init_sel,
                                 ThemeFunc theme_func )
{
    const char* const *dirs = g_get_system_data_dirs();
    const char* const *dir;
    char* dir_path;
    for( dir = dirs; *dir; ++dir )
    {
        dir_path = g_build_filename( *dir, relative_path, NULL );
        load_themes_from_dir( list, dir_path, lookup, sel, init_sel, theme_func );
        g_free( dir_path );
    }
    dir_path = g_build_filename( g_get_user_data_dir(), relative_path, NULL );
    load_themes_from_dir( list, dir_path, lookup, sel, init_sel, theme_func );
    g_free( dir_path );
}

static gboolean icon_theme_func(const char* file, const char* dir, const char* name, GtkListStore* list, GtkTreeIter* it)
{
    GKeyFile* kf;
    char* disp_name = NULL;
    if( g_str_has_prefix( name, "default." ) )
        return FALSE;

    kf = g_key_file_new();
    if( g_key_file_load_from_file(kf, file, 0, NULL) )
    {
        if( g_key_file_has_key(kf, "Icon Theme", "Directories", NULL)
            && ! g_key_file_get_boolean(kf, "Icon Theme", "Hidden", NULL) )
        {
            disp_name = g_key_file_get_locale_string(kf, "Icon Theme", "Name", NULL, NULL);
            gtk_list_store_set( list, it, COL_DISP_NAME, disp_name ? disp_name : name, -1 );
        }
    }
    g_key_file_free(kf);
    return disp_name != NULL;
}

static gboolean cursor_theme_func(const char* dir, const char* name, const char* lookup)
{
    char* ret = NULL;
/*
    GKeyFile* kf = g_key_file_new();
    if( g_key_file_load_from_file(kf, lookup, 0, NULL) )
    {
        if( g_key_file_has_key(kf, "Icon Theme", "Directories", NULL) )
        {
            ret = g_key_file_get_locale_string(kf, "Icon Theme", "Name", NULL, NULL);
        }
    }
    g_key_file_free(kf);
*/
    return ret != NULL;
}

static void load_gtk_themes( GtkListStore* list, const char* cur_sel )
{
    char* path;
    GtkTreeSelection* sel = gtk_tree_view_get_selection( gtk_theme_view );
    load_from_data_dirs( list, "themes", "gtk-2.0", sel, cur_sel, NULL );
    path = g_build_filename( g_get_home_dir(), ".themes", NULL );
    load_themes_from_dir( list, path, "gtk-2.0", sel, cur_sel, NULL );
    g_free( path );
    gtk_tree_sortable_set_sort_column_id( (GtkTreeSortable*)list, 0, GTK_SORT_ASCENDING );
}

static void load_icon_themes( GtkListStore* list, const char* cur_sel )
{
    char* path;
    GtkTreeSelection* sel = gtk_tree_view_get_selection( icon_theme_view );
    load_from_data_dirs( list, "icons", "index.theme", sel, cur_sel, icon_theme_func );
    path = g_build_filename( g_get_home_dir(), ".icons", NULL );
    load_themes_from_dir( list, path, "index.theme", sel, cur_sel, icon_theme_func );
    g_free( path );
    gtk_tree_sortable_set_sort_column_id( (GtkTreeSortable*)list, 0, GTK_SORT_ASCENDING );
}

static void load_cursor_themes( GtkListStore* list, const char* cur_sel )
{
    char* path;
    GtkTreeSelection* sel = gtk_tree_view_get_selection( cursor_theme_view );
    load_from_data_dirs( list, "icons", "cursors", sel, cur_sel, NULL );
    path = g_build_filename( g_get_home_dir(), ".icons", NULL );
    load_themes_from_dir( list, path, "cursors", sel, cur_sel, NULL );
    g_free( path );
    gtk_tree_sortable_set_sort_column_id( (GtkTreeSortable*)list, 0, GTK_SORT_ASCENDING );
}
/*
static void load_fonts( GtkListStore* list )
{

}
*/

gboolean center_win( GtkWidget* dlg )
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

	if( under_lxsession )
	{
		session_name = g_getenv("DESKTOP_SESSION");
	}
	else
	{
        /* no lxsession-settings daemon, use gtkrc-2.0 */
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
                        "gtk-cursor-theme-name", &cursor_theme_name,
                        "gtk-cursor-theme-size", &cursor_theme_size,
                        NULL );

    if(  ! gtk_theme_name )
        gtk_theme_name = g_strdup( "Raleigh" );
    if(  ! icon_theme_name )
        icon_theme_name = g_strdup( "hicolor" );
    if(  ! cursor_theme_name )
        cursor_theme_name = g_strdup( "default" );
    if( ! font_name )
        font_name = g_strdup( "Sans 10" );
    if(  ! cursor_theme_size )
        cursor_theme_size = 16;

	/* no lxsession-settings daemon, use gtkrc-2.0 */
	if( ! under_lxsession )
		write_rc_file( tmp_rc_file );

    INIT_LIST( gtk_theme, "gtk-theme-name" )
    INIT_LIST( icon_theme, "gtk-icon-theme-name" )
    INIT_LIST( cursor_theme, "gtk-cursor-theme-name" )
    gtk_font_button_set_font_name( (GtkFontButton*)lookup_widget(dlg, "font"), font_name );
    gtk_range_set_value( GTK_RANGE(lookup_widget(dlg, "cursor_theme_size")), cursor_theme_size );

    gtk_combo_box_set_active( (GtkComboBox*)lookup_widget(dlg, "tb_style"), tb_style < 4 ? tb_style : 3 );

    GET_WIDGET_WITH_TYPE( demo_box, GTK_WIDGET );
    gtk_widget_show( demo_box );

	if( under_lxsession )
	{
		/* XSettings daemon of lxsession is running, gtkrc is useless.
		 * 	We should set properties of GtkSettings object on the fly.
		 * 	This will cause problems with some themes, but we have no choice.
		 */
		show_demo( (GdkNativeWindow)demo_box );
		gtk_widget_show_all( dlg );
	}
	else
	{
		/* no lxsession-settings daemon, use gtkrc-2.0 and load preview in another process */
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

	if( under_lxsession )
	{
/*
		char* argv[]={"lxsession", "-r", NULL};
		g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL, NULL, NULL);
		return;
*/
		event.message_type = gdk_atom_intern_static_string("_LXSESSION");
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
	if( under_lxsession )
		write_lxsession_config();
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

	if( under_lxsession )
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

    gtk_file_chooser_add_filter( GTK_FILE_CHOOSER(fc), filter );
    gtk_file_chooser_set_filter( GTK_FILE_CHOOSER(fc), filter );

    if( gtk_dialog_run( (GtkDialog*)fc ) == GTK_RESPONSE_OK )
    {
        char* file = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(fc) );
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

            /* reload all cursor themes */
            gtk_list_store_clear( cursor_theme_list );
            load_cursor_themes( cursor_theme_list, stdo ? stdo : "" );
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
on_cursor_size_changed                 (GtkHScale       *cursorsizescale,
                                        gpointer         user_data)
{
    cursor_theme_size = gtk_range_get_value( GTK_RANGE(cursorsizescale) );

	if( under_lxsession )
	{
		g_object_set( gtk_settings_get_default(), "gtk-cursor-theme-size", cursor_theme_size, NULL );
		enable_apply();
	}
	else
	{
		write_rc_file( tmp_rc_file );
		reload_demo_process();
	}
}

void
on_tb_style_changed                    (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int sel = gtk_combo_box_get_active( combobox );
    if( sel == tb_style || sel < 0 )
        return;
    tb_style = sel;

	if( under_lxsession )
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

