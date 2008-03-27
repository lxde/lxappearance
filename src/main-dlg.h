#include <gtk/gtk.h>

void main_dlg_init( GtkWidget* dlg );

void
on_apply_clicked                       (GtkButton       *button,
                                        gpointer         user_data);

void
on_font_changed                        (GtkFontButton   *fontbutton,
                                        gpointer         user_data);

void
on_install_theme_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_remove_theme_clicked                (GtkButton       *button,
                                        gpointer         user_data);
