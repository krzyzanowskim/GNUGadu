/* $Id: gui_chat_session.h,v 1.6 2004/01/28 23:40:21 shaster Exp $ */

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

#ifndef __GTK_CHAT_SESSION_H__
#define __GTK_CHAT_SESSION_H__

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>
#include "gui_main.h"

#define DEFAULT_TEXT_COLOR "#000001"
#define DEFAULT_FONT "Sans"

typedef struct _GUIChatSession GUIChatSession;
typedef struct _GUIChatSessionClass GUIChatSessionClass;

#define GUI_CHAT_SESSION_TYPE		  	(gui_chat_session_get_type ())
#define GUI_CHAT_SESSION(obj)		  	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GUI_CHAT_SESSION_TYPE, GUIChatSession))
#define GUI_CHAT_SESSION_CLASS(klass)	  	(G_TYPE_CHECK_CLASS_CAST ((klass), GUI_CHAT_SESSION_TYPE, GUIChatSessionClass))
#define GUI_CHAT_SESSION_IS_SESSION(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GUI_CHAT_SESSION_TYPE))
#define GUI_CHAT_SESSION_IS_SESSION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GUI_CHAT_SESSION_TYPE))
#define GUI_CHAT_SESSION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), GUI_CHAT_SESSION_TYPE, GUIChatSessionClass))

GType gui_chat_session_get_type(void);

enum
{
    GUI_CHAT_SESSION_HISTORY_RECV = 1,
    GUI_CHAT_SESSION_HISTORY_SEND
};

typedef struct
{
    guint type;			/* GUI_CHAT_SESSION_HISTORY_RECV,GUI_CHAT_SESSION_HISTORY_SEND */
    gchar *text;
    GTimeVal date1;		/* receive date */
    GTimeVal date2;		/* send date */
} gcs_history;

struct _GUIChatSession
{
    GObject patent;

    /* instance members */
    GList *recipients;		/* list of recipients, typically there is only one, but during conversation there may be a few */
    GList *history_list;	/* gcs_history */
    /* gtk widget to put in some container */
    GtkWidget *widget;
    GtkWidget *buttons_widget;
    gboolean visibility;
};

struct _GUIChatSessionClass
{
    GObjectClass parent;
    /* class members */
};

/* public */
GUIChatSession *gui_chat_session_new(gui_protocol * gp);

void gui_chat_session_add_recipient(GUIChatSession * gsc, gchar * id);

GList *gui_chat_session_get_recipients_list(GUIChatSession * gsc);

guint gui_chat_session_get_session_type(GUIChatSession * gsc);

GUIChatSession *gui_chat_session_find(gui_protocol * gp, GList * recipients_arg);

GtkWidget *gui_chat_session_get_widget(GUIChatSession * gcs);

GtkWidget *gui_chat_session_create_gtk_widget(GUIChatSession * gcs);

void gui_chat_sessions_create_visible_chat_window(GUIChatSession * gcs);


#endif
