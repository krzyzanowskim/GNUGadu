/* $Id: GUI_plugin.h,v 1.9 2005/07/27 15:54:00 mkobierzycki Exp $ */

/* 
 * GUI (gtk+) plugin for GNU Gadu 2 
 * 
 * Copyright (C) 2003-2005 GNU Gadu Team 
 * 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version. 
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef GGadu_GUI_PLUGIN_H
#define GGadu_GUI_PLUGIN_H 1

#include <gtk/gtk.h>
#include "gui_main.h"
#include "ggadu_types.h"

gboolean nick_list_clicked(GtkWidget * widget, GdkEventButton * event, gpointer user_data);

gboolean nick_list_pressed(GtkWidget * widget, GdkEventKey * event, gpointer user_data);

gboolean nick_list_row_activated(GtkWidget * widget, GtkTreePath *arg1, GtkTreeViewColumn *arg2, gpointer user_data);

gboolean nick_list_row_changed(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean cur_sel, gpointer user_data);

void nick_list_row_changed2(GtkTreeModel *treemodel,GtkTreePath *arg1,GtkTreeIter *arg2,gpointer user_data);

GtkWidget *create_status_menu(gui_protocol * gp, GtkWidget * status_image);

void gui_main_window_create(gboolean visible);

void gui_build_default_menu();

void gui_build_default_toolbar();

void gui_msg_receive(GGaduSignal * signal);

void gui_produce_menu_for_factory(GGaduMenu * menu, GtkItemFactory * item_factory, gchar * root, gpointer user_data);

GSList *gui_read_emoticons(gchar * path);

void gui_config_emoticons();

void gui_load_theme();

void gui_reload_images();

void set_selected_users_list(GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, gpointer data);

void gui_show_hide_window();

gpointer gui_status2clipboard_ext(gpointer user_data);

#endif
