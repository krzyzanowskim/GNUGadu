/* $Id: gui_dialogs.h,v 1.5 2004/05/24 14:28:15 krzyzak Exp $ */

/* 
 * GUI (gtk+) plugin for GNU Gadu 2 
 * 
 * Copyright (C) 2003-2004 GNU Gadu Team 
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

#ifndef GGadu_GUI_DIALOGS_H
#define GGadu_GUI_DIALOGS_H 1

void gui_user_data_window(gpointer signal, gboolean change);

void gui_show_dialog(gpointer signal, gboolean change);

void gui_show_about(gpointer signal);

void gui_show_message_box(gint type, gpointer signal);

void gui_show_window_with_text(gpointer signal);

GtkWidget *gui_build_dialog_gtk_table(GSList * list, gint cols, gboolean use_progress);

void gui_about();

#endif
