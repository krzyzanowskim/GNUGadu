/* $Id: gui_support.h,v 1.3 2004/01/17 00:44:59 shaster Exp $ */

#ifndef GGadu_GUI_PLUGIN_FIND_H
#define GGadu_GUI_PLUGIN_FIND_H 1

#include <gtk/gtk.h>
#include "gui_main.h"

void gui_remove_all_chat_sessions(gpointer protocols_list);

/* obsolete */
gui_chat_session *gui_session_find_confer(gui_protocol * gp, GSList * recipients);

/* obsolete */
gui_chat_session *gui_session_find(gui_protocol * gp, gchar * id);

gui_protocol *gui_find_protocol(gchar * plugin_name, GSList * protocolsl);

GGaduStatusPrototype *gui_find_status_prototype(GGaduProtocol * gp, guint status);

GGaduContact *gui_find_user(gchar * id, gui_protocol * gp);

/* utility functions */

GtkWidget *create_image(const gchar * filename);

GdkPixbuf *create_pixbuf(const gchar * filename);

GtkWidget *lookup_widget(GtkWidget * widget, const gchar * widget_name);

#endif
