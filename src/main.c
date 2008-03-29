/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>

#include "main-dlg-ui.h"
#include "demo.h"
#include "glade-support.h"

char tmp_rc_file[] = "/tmp/gtkrc-2.0-XXXXXX";

int main (int argc, char *argv[])
{
    GtkWidget *dlg;

#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    if( argc >= 3 && strcmp( argv[1], "demo" ) == 0 )
    {
        char* files[] = { argv[3], NULL };
        gtk_rc_set_default_files(files);

        gtk_set_locale ();
        gtk_init (&argc, &argv);
        show_demo( (GdkNativeWindow)atoi( argv[2] ) );
        gtk_main();
        return 0;
    }

    mkstemp( tmp_rc_file );

    gtk_set_locale ();
    gtk_init (&argc, &argv);

    /* Dirty hack: "gtk-toolbar-style" is installed in class_init of GtkToolbar */
    gtk_widget_destroy( gtk_toolbar_new() );

    dlg = create_dlg ();
    main_dlg_init( dlg );

    gtk_main ();

    unlink( tmp_rc_file );
    return 0;
}

