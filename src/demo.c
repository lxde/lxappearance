#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "demo.h"
#include "demo-ui.h"
#include "glade-support.h"

static GtkIconView* icon_view = NULL;

static void load_demo_icons()
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

void show_demo( GdkNativeWindow wid )
{
    GtkWidget* demo = create_demo_window();
    GtkWidget* plug = gtk_plug_new( wid );
    GtkWidget* top_vbox;

    icon_view = lookup_widget( demo, "icon_view" );
    gtk_widget_show_all( demo );
    gtk_container_add( (GtkContainer*)plug, demo );

    gtk_widget_show( plug );
    gtk_icon_view_set_pixbuf_column( icon_view, 0 );
    gtk_icon_view_set_text_column( icon_view, 1 );
    gtk_icon_view_set_item_width( icon_view, 64 );
    gtk_icon_view_set_column_spacing( icon_view, 8 );
    gtk_icon_view_set_row_spacing( icon_view, 8 );

    load_demo_icons();
}
