/* $Id: gui_main.h,v 1.15 2004/05/04 21:39:09 krzyzak Exp $ */

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

#ifndef GGadu_GUI_PLUGIN_MAIN_H
#define GGadu_GUI_PLUGIN_MAIN_H 1

#include <gtk/gtk.h>
#include "ggadu_types.h"

/* default size of a window */
#define DEFAULT_WIDTH 160
#define DEFAULT_HEIGHT 488

/* default size of a chat window */
#define DEFAULT_CHAT_WINDOW_WIDTH 400
#define DEFAULT_CHAT_WINDOW_HEIGHT 300

/* default icon's filename */
#define GGADU_DEFAULT_ICON_FILENAME "icon.png"
#define GGADU_MSG_ICON_FILENAME "new-msg.png"


typedef struct
{
	gchar *id;
	GtkWidget *chat;
	GSList *recipients;
} gui_chat_session;


typedef struct
{
	gchar *plugin_name;
	GSList *userlist;
	GSList *chat_sessions;
	GtkListStore *users_liststore;
	GtkWidget *add_info_label;
	GtkWidget *statuslist_eventbox;
	gchar *tree_path;
	guint blinker;
	GdkPixbuf *blinker_image1;
	GdkPixbuf *blinker_image2;
	guint aaway_timer;
	GGaduProtocol *p;
	GtkTooltips *tooltips;
} gui_protocol;

typedef struct
{
	gchar *emoticon;
	gchar *file;
} gui_emoticon;

enum
{
	CHAT_TYPE_CLASSIC,
	CHAT_TYPE_TABBED
};


void grm_free(gpointer signal);

void gautl_free(gpointer signal);

void gn_free(gpointer signal);

#endif
