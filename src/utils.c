/*
 *      utils.c
 *
 *      Copyright 2010 PCMan <pcman.tw@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include "utils.h"
#include <glib/gi18n.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>

static void on_pid_exit(GPid pid, gint status, gpointer user_data)
{
    GtkDialog* dlg = GTK_DIALOG(user_data);
    gtk_dialog_response(dlg, GTK_RESPONSE_CLOSE);
    g_debug("pid exit");
}

static void on_progress_dlg_response(GtkDialog* dlg, int res, gpointer user_data)
{
    GPid* ppid = (GPid*)user_data;
    int status;
    kill(*ppid, SIGTERM);
    waitpid(*ppid, &status, WNOHANG);
}

void show_progress_for_pid(GtkWindow* parent, const char* title, const char* msg, GPid pid)
{
    GtkWidget* dlg = gtk_dialog_new_with_buttons(title, parent,
                            GTK_DIALOG_NO_SEPARATOR|GTK_DIALOG_MODAL,
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    guint child_watch = g_child_watch_add(pid, on_pid_exit, dlg);
    GtkWidget* vbox = gtk_dialog_get_content_area(dlg);
    GtkWidget* label = gtk_label_new(msg);
    gtk_widget_show(label);
    gtk_box_pack_start(vbox, label, FALSE, TRUE, 0);
    g_signal_connect(dlg, "response", G_CALLBACK(on_progress_dlg_response), &pid);
    gtk_dialog_run(dlg);
    g_source_remove(child_watch);
    gtk_widget_destroy(dlg);
}

gboolean install_icon_theme(GtkWindow* parent)
{
    GtkFileFilter* filter = gtk_file_filter_new();
    char* file = NULL;
    GPid pid = -1;
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
        file = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(fc) );
        char* argv[]={
            PACKAGE_DATA_DIR"/install-icon-theme.sh",
            file, NULL };
        g_spawn_async(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL);
        g_debug("%s\n%s\npid = %d", file, argv[0], pid);
    }
    gtk_widget_destroy( fc );

    if(pid >=0)
        show_progress_for_pid(parent, "Install themes", "Installing...", pid);
    g_free(file);
}

