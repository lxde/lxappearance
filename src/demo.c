#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "demo.h"
#include "demo-ui.h"
#include "glade-support.h"

extern gboolean under_lxsession;	/* wether lxsession-xsettings daemon is active */

static GtkIconView* icon_view = NULL;

static void load_demo_icons()
{
    static const char* icon_names[]={
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
    gtk_icon_view_set_model( icon_view, GTK_TREE_MODEL(demo_icon_list) );
    g_object_unref( demo_icon_list );
}

static void load_demo_tree_view( GtkTreeView* view )
{
    GtkListStore* list;
    int i;
    char str[256];

    for( i = 0; i < 3; ++i )
    {
        GtkTreeViewColumn* col;
        g_snprintf( str, 256, "%s %d", _("Column"), i + 1 );
        col = gtk_tree_view_column_new_with_attributes( "Column 1", gtk_cell_renderer_text_new(), "text", i, NULL );
        gtk_tree_view_append_column( view, col );
    }

    list = gtk_list_store_new( 3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );
    for( i = 0; i < 3; ++i )
    {
        GtkTreeIter it;
        g_snprintf( str, 256, "%s %d", _("Item"), i + 1 );
        gtk_list_store_append( list, &it );
        gtk_list_store_set( list, &it, 0, str, 1, str, 2, str, -1 );
    }
    gtk_tree_view_set_model( view, (GtkTreeModel*)list );
    g_object_unref( list );
}

void show_demo( GdkNativeWindow wid )
{
    GtkWidget* demo = create_demo_window();
    GtkWidget* plug;
    GtkWidget* top_vbox;
    GtkToolbarStyle tb_style;
    GtkWidget* tree_view;

    g_object_get( gtk_settings_get_default(), "gtk-toolbar-style", &tb_style, NULL );
    gtk_toolbar_set_style (GTK_TOOLBAR (lookup_widget(demo, "toolbar")), tb_style );

    icon_view = GTK_ICON_VIEW( lookup_widget( demo, "icon_view" ) );
    tree_view = lookup_widget( demo, "demo_treeview" );

    gtk_icon_view_set_pixbuf_column( icon_view, 0 );
    gtk_icon_view_set_text_column( icon_view, 1 );
    gtk_icon_view_set_item_width( icon_view, 64 );
    gtk_icon_view_set_column_spacing( icon_view, 8 );
    gtk_icon_view_set_row_spacing( icon_view, 8 );

    load_demo_icons();
    load_demo_tree_view( (GtkTreeView*)tree_view );

    gtk_widget_show_all( demo );
    
    /* lxsession settings daemon is active and we don't use gtkrc files. */
    if( under_lxsession )
    {
    	/* The demo window and the main dialog are in the same process */
    	GtkWidget* demo_box = (GtkWidget*)wid;
    	gtk_container_add( GTK_CONTAINER(demo_box), demo );
    	g_signal_connect( gtk_icon_theme_get_default(), "changed", G_CALLBACK( load_demo_icons ), NULL );
    }
    else
    {
    	/* we are in another process with a different gtkrc file. */
		plug = gtk_plug_new( wid );	/* plug our demo window into the main process */
		gtk_container_add( (GtkContainer*)plug, demo );
		gtk_widget_show( plug );    	
    }
}
