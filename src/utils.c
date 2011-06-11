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
#include "lxappearance.h"
#include <glib/gi18n.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <glib/gstdio.h>

#include "icon-theme.h"

static void on_pid_exit(GPid pid, gint status, gpointer user_data)
{
    GtkDialog* dlg = GTK_DIALOG(user_data);
    gtk_dialog_response(dlg, GTK_RESPONSE_OK);
    g_debug("pid exit");
}

static void on_progress_dlg_response(GtkDialog* dlg, int res, gpointer user_data)
{
    if(res != GTK_RESPONSE_OK)
    {
        GPid* ppid = (GPid*)user_data;
        int status;
        kill(*ppid, SIGTERM);
        waitpid(*ppid, &status, WNOHANG);
    }
}

static gboolean on_progress_timeout(GtkProgressBar* progress)
{
    gtk_progress_bar_pulse(progress);
    return TRUE;
}

gboolean show_progress_for_pid(GtkWindow* parent, const char* title, const char* msg, GPid pid)
{
    gint res;
    GtkWidget* dlg = gtk_dialog_new_with_buttons(title, parent,
#if GTK_CHECK_VERSION(3, 0, 0)
                            GTK_DIALOG_MODAL,
#else
                            GTK_DIALOG_NO_SEPARATOR|GTK_DIALOG_MODAL,
#endif
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    GtkWidget* progress = gtk_progress_bar_new();
    GtkWidget* vbox = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget* label = gtk_label_new(msg);

    guint child_watch = g_child_watch_add(pid, on_pid_exit, dlg);
    guint timeout = g_timeout_add(300, (GSourceFunc)on_progress_timeout, progress);

    gtk_window_set_default_size(GTK_WINDOW(dlg), 240, -1);
    gtk_box_set_spacing(GTK_BOX(vbox), 6);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
    gtk_widget_show(progress);
    gtk_box_pack_start(GTK_BOX(vbox), progress, FALSE, TRUE, 0);
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progress));
    g_signal_connect(dlg, "response", G_CALLBACK(on_progress_dlg_response), &pid);

    res = gtk_dialog_run(GTK_DIALOG(dlg));

    g_source_remove(child_watch);
    g_source_remove(timeout);
    gtk_widget_destroy(dlg);

    return (res == GTK_RESPONSE_OK);
}

static void insert_theme_to_models(IconTheme* theme)
{
    int icon_theme_pos = 0;
    int cursor_theme_pos = 0;
    GSList* l;
    GtkTreeIter it;

    for(l = app.icon_themes; l; l=l->next)
    {
        IconTheme* theme2 = (IconTheme*)l->data;
        if(l->data == theme)
            break;
        if(theme2->has_icon)
            ++icon_theme_pos;
        if(theme2->has_cursor)
            ++cursor_theme_pos;
    }
    if(theme->has_icon)
        gtk_list_store_insert_with_values(app.icon_theme_store, &it, icon_theme_pos, 0, theme->disp_name, 1, theme, -1);

    if(theme->has_cursor)
        gtk_list_store_insert_with_values(app.cursor_theme_store, &it, cursor_theme_pos, 0, theme->disp_name, 1, theme, -1);
}

static gboolean install_icon_theme_package(const char* package_path)
{
    GPid pid = -1;
    const char* user_icons_dir = icon_theme_dirs[0];
    char* tmp_dir = g_build_filename(user_icons_dir, "tmp.XXXXXX", NULL);
    char* argv[]= {
        "tar",
        NULL,
        "-C",
        tmp_dir,
        "-xf",
        (char*)package_path,
        NULL
    };

    if(g_mkdir_with_parents(user_icons_dir, 0700) == -1)
        return FALSE;

    if(!mkdtemp(tmp_dir))
        return FALSE;

    if(g_str_has_suffix(package_path, ".tar.gz"))
        argv[1] = "--gzip";
    else if(g_str_has_suffix(package_path, ".tar.bz2"))
        argv[1] = "--bzip2";
    else /* the file format is not supported */
        goto _out;

    char* cmd = g_strjoinv(" ", argv);
    g_debug("extract: %s", cmd);
    g_free(cmd);

    if(g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH|G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL))
    {
        g_debug("pid = %d", pid);
        /* show progress UI for this pid */
        if(show_progress_for_pid(GTK_WINDOW(app.dlg), "Install themes", "Installing...", pid))
        {
            /* move files in tmp_dir to user_icons_dir */
            GDir* dir;
            GKeyFile* kf = g_key_file_new();

            /* convert the themes in the dir to IconTheme structs and add them to app.icon_themes list */
            load_icon_themes_from_dir(user_icons_dir, tmp_dir, kf);
            g_key_file_free(kf);

            /* now really move this themes to ~/.icons dir and also update the GUI */
            dir = g_dir_open(tmp_dir, 0, NULL);
            if(dir)
            {
                char* name;
                while(name == (char*)g_dir_read_name(dir))
                {
                    char* index_theme = g_build_filename(tmp_dir, name, "index.theme", NULL);
                    gboolean is_theme = g_file_test(index_theme, G_FILE_TEST_EXISTS);
                    g_free(index_theme);
                    if(is_theme)
                    {
                        char* theme_tmp = g_build_filename(tmp_dir, name, NULL);
                        char* theme_target = g_build_filename(user_icons_dir, name, NULL);
                        if(g_rename(theme_tmp, theme_target) == 0)
                        {
                            /* the theme is already installed to ~/.icons */
                            GSList* l= g_slist_find_custom(app.icon_themes, name, (GCompareFunc)icon_theme_cmp_name);
                            if(l)
                            {
                                IconTheme* theme = (IconTheme*)l->data;
                                g_debug("installed theme: %p, %s", theme, theme->name);
                                /* update UI */
                                insert_theme_to_models(theme);
                            }
                        }
                        else
                        {
                            /* errors happened */
                        }
                        g_free(theme_target);
                        g_free(theme_tmp);
                    }
                }
                g_dir_close(dir);

                /* remove remaining files. FIXME: will this cause problems? */
                name = g_strdup_printf("rm -rf '%s'", tmp_dir);
                g_spawn_command_line_sync(name, NULL, NULL, NULL, NULL);
                g_free(name);
            }
        }
    }

_out:
    g_free(tmp_dir);
    return (pid != -1);
}

gboolean install_icon_theme(GtkWindow* parent)
{
    GtkFileFilter* filter = gtk_file_filter_new();
    char* file = NULL;
    int res;
    GtkWidget* fc = gtk_file_chooser_dialog_new( _("Select an icon theme"), NULL,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL );
    gtk_window_set_transient_for(GTK_WINDOW(fc), GTK_WINDOW(app.dlg));
    gtk_file_filter_add_pattern( filter, "*.tar.gz" );
    gtk_file_filter_add_pattern( filter, "*.tar.bz2" );
    gtk_file_filter_set_name( filter, _("*.tar.gz, *.tar.bz2 (Icon Theme)") );

    gtk_file_chooser_add_filter( GTK_FILE_CHOOSER(fc), filter );
    gtk_file_chooser_set_filter( GTK_FILE_CHOOSER(fc), filter );

    res = gtk_dialog_run( (GtkDialog*)fc );
    file = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(fc) );
    gtk_widget_destroy( fc );

    if( res == GTK_RESPONSE_OK )
        install_icon_theme_package(file);

    g_free(file);
    return TRUE;
}

gboolean remove_icon_theme(GtkWindow* parent, IconTheme* theme)
{
    gboolean ret = TRUE;
    char* dir = g_build_filename(theme->base_dir, theme->name, NULL);
    char* tmp_dir = g_build_filename(theme->base_dir, "tmp.XXXXXX", NULL);
g_debug("tmp_dir = %s", tmp_dir);
    /* move the theme to a tmp dir first. so we can make the
     * removal atomic. */
    if(mkdtemp(tmp_dir))
    {
        char* tmp_dest = g_build_filename(tmp_dir, theme->name, NULL);
        if(g_rename(dir, tmp_dest) == 0)
        {
            char* argv[] = {
                "rm",
                "-rf",
                tmp_dir,
                NULL
            };
            GPid pid;
            if(g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH|G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL))
            {
                ret = show_progress_for_pid(GTK_WINDOW(app.dlg), "Remove icon theme", "Removing...", pid);
            }
        }
        g_free(tmp_dest);
    }
    else
        ret = FALSE;

    g_free(dir);
    return ret;
}
