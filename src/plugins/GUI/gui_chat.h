/* $Id: gui_chat.h,v 1.4 2004/01/17 00:44:59 shaster Exp $ */

#ifndef GGadu_GUI_PLUGIN_CHAT_H
#define GGadu_GUI_PLUGIN_CHAT_H 1

#include <gtk/gtk.h>
#include "gui_main.h"

#define MAX_EMOTICONS_IN_ROW 8
#define DEFAULT_TEXT_COLOR "#000001"
#define DEFAULT_FONT "Sans"

GtkWidget *create_chat(gui_chat_session * session, gchar * plugin_name, gchar * id, gboolean visible);

void gui_chat_append(GtkWidget * chat, gpointer msg, gboolean self);

void gui_chat_update_tags();

#endif
