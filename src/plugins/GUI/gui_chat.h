/* $Id: gui_chat.h,v 1.9 2004/06/11 01:25:33 krzyzak Exp $ */

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

#ifndef GGadu_GUI_PLUGIN_CHAT_H
#define GGadu_GUI_PLUGIN_CHAT_H 1

#include <gtk/gtk.h>
#ifdef USE_GTKSPELL
#include <gtkspell/gtkspell.h>
#include <aspell.h>
#endif /* USE_GTKSPELL */
#include "gui_main.h"

#define MAX_EMOTICONS_IN_ROW 8
#define DEFAULT_TEXT_COLOR "#000001"
#define DEFAULT_FONT "Sans"

GtkWidget *create_chat(gui_chat_session * session, gchar * plugin_name, gchar * id, gboolean visible);

void gui_chat_append(GtkWidget * chat, gpointer msg, gboolean self, gboolean notice_message);

void gui_chat_update_tags();

#endif
