/* $Id: GUI_plugin.h,v 1.2 2003/11/06 19:17:59 thrulliq Exp $ */
#ifndef GGadu_GUI_PLUGIN_H
#define GGadu_GUI_PLUGIN_H 1

#include <gtk/gtk.h>
#include "gui_main.h"
#include "gg-types.h"
#include "unified-types.h"

gboolean nick_list_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data);

GtkWidget *create_status_menu(gui_protocol *gp, GtkWidget *status_image);

void gui_main_window_create(gboolean visible);

void gui_build_default_menu();

void gui_build_default_toolbar();

void gui_msg_receive(GGaduSignal *signal);

void gui_produce_menu_for_factory(GGaduMenu *menu,GtkItemFactory *item_factory, gchar *root, gpointer user_data);

GSList *gui_read_emoticons(gchar *path);

void gui_config_emoticons();

void gui_load_theme();

void gui_reload_images();

void set_selected_users_list(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data);

void gui_show_hide_window();

#endif


