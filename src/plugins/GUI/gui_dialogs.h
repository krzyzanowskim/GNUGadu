/* $Id: gui_dialogs.h,v 1.2 2003/05/01 20:18:09 shaster Exp $ */

#ifndef GGadu_GUI_DIALOGS_H
#define GGadu_GUI_DIALOGS_H 1

void gui_user_data_window(gpointer signal, gboolean change);

void gui_show_dialog(gpointer signal, gboolean change);

void gui_show_about(gpointer signal);

void gui_show_message_box(gint type, gpointer signal);

void gui_show_window_with_text(gpointer signal);

GtkWidget *gui_build_dialog_gtk_table(GSList *list, gint cols);

void gui_about();

#endif
