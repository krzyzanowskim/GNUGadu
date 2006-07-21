/* $Id: gui_support.h,v 1.11 2006/07/21 22:34:58 krzyzak Exp $ */

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

#ifndef GGadu_GUI_PLUGIN_FIND_H
#define GGadu_GUI_PLUGIN_FIND_H 1

#include <gtk/gtk.h>
#include "gui_main.h"

void gui_remove_all_chat_sessions(gpointer protocols_list);

gint gui_count_visible_tabs(GtkNotebook * notebook);

/* obsolete */
gui_chat_session *gui_session_find_confer(gui_protocol * gp, GSList * recipients);

/* obsolete */
gui_chat_session *gui_session_find(gui_protocol * gp, gchar * id);

gboolean gui_check_for_sessions(GSList * protocolsl);

gui_protocol *gui_find_protocol(gchar * plugin_name, GSList * protocolsl);

/*GGaduStatusPrototype *gui_find_status_prototype(GGaduProtocol * gp, gint status);*/

GGaduContact *gui_find_user(gchar * id, gui_protocol * gp);

/* utility functions */

GtkWidget *create_image(const gchar * filename);

GdkPixbuf *create_pixbuf(const gchar * filename);

GtkWidget *lookup_widget(GtkWidget * widget, const gchar * widget_name);

gchar *ggadu_escape_html(const char *html);

void gaim_str_strip_char(char *str, char thechar);
gboolean gaim_str_has_prefix(const char *s, const char *p);
char *gaim_unescape_html(const char *html);


#endif
