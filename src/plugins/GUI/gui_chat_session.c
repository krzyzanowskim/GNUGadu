/* $Id: gui_chat_session.c,v 1.9 2004/01/28 23:40:21 shaster Exp $ */

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

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>
#include <string.h>
#include "unified-types.h"
#include "support.h"
#include "plugins.h"
#include "ggadu_conf.h"
#include "gui_main.h"
#include "gui_chat_session.h"
#include "gui_support.h"

extern GGaduPlugin *gui_handler;

static void gui_chat_sessions_create_chat_window(GUIChatSession * gcs);
/* 
 * public 
 */

GUIChatSession *gui_chat_session_new(gui_protocol * gp)
{
    GUIChatSession *new_session = NULL;

    g_return_val_if_fail(gp != NULL, NULL);

    new_session = g_object_new(GUI_CHAT_SESSION_TYPE, NULL);
    gp->chat_sessions = g_slist_append(gp->chat_sessions, new_session);

    gui_chat_session_create_gtk_widget(new_session);

    return new_session;
}

GtkWidget *gui_chat_session_get_widget(GUIChatSession * gcs)
{
    g_return_val_if_fail(GUI_CHAT_SESSION_IS_SESSION(gcs), NULL);
    return gcs->widget;
}

/* you can g_free(id) after that if you want */
void gui_chat_session_add_recipient(GUIChatSession * gcs, gchar * id)
{
    g_return_if_fail(id != NULL);
    g_return_if_fail(GUI_CHAT_SESSION_IS_SESSION(gcs));
    gcs->recipients = g_list_append(gcs->recipients, g_strdup(id));
}

GList *gui_chat_session_get_recipients_list(GUIChatSession * gcs)
{
    g_return_val_if_fail(GUI_CHAT_SESSION_IS_SESSION(gcs), NULL);
    return gcs->recipients;
}

guint gui_chat_session_get_session_type(GUIChatSession * gcs)
{
    g_return_val_if_fail(GUI_CHAT_SESSION_IS_SESSION(gcs), 0);
    g_return_val_if_fail(gcs->recipients != NULL, 0);

    if (g_list_length(gcs->recipients) > 1)
	return GGADU_CLASS_CONFERENCE;

    return GGADU_CLASS_CHAT;
}

void gui_chat_session_add_message(GUIChatSession * gcs, gchar * text, GTimeVal * send_date)
{
    gcs_history *history_entry = NULL;

    g_return_if_fail(text != NULL);
    g_return_if_fail(GUI_CHAT_SESSION_IS_SESSION(gcs));

    history_entry = g_new0(gcs_history, sizeof (gcs_history));

    history_entry->text = g_strdup(text);
    g_get_current_time(&history_entry->date1);
    g_get_current_time(&history_entry->date2);

    if (send_date != NULL)
	memcpy(send_date, &history_entry->date2, sizeof (GTimeVal));

    gcs->history_list = g_list_append(gcs->history_list, history_entry);
}

GUIChatSession *gui_chat_session_find(gui_protocol * gp, GList * recipients_arg)
{
    GSList *sessions_list = NULL;

    g_return_val_if_fail(gp != NULL, NULL);
    g_return_val_if_fail(gp->chat_sessions != NULL, NULL);

    sessions_list = gp->chat_sessions;

    while (sessions_list)
    {
	GList *recipients = NULL;
	GList *session_recipients = NULL;
	GUIChatSession *session = NULL;
	guint hit = 0;

	session = GUI_CHAT_SESSION(sessions_list->data);

	recipients = recipients_arg;
	while (recipients)
	{
	    session_recipients = gui_chat_session_get_recipients_list(session);
	    while (session_recipients)
	    {
		if (!ggadu_strcasecmp((gchar *) session_recipients->data, (gchar *) recipients->data))
		    hit++;

		session_recipients = session_recipients->next;
	    }
	    recipients = recipients->next;
	}

	if (hit == g_list_length(recipients))
	    return session;

	sessions_list = sessions_list->next;
    }

    return NULL;
}

/* private */
static GtkWidget *gui_chat_session_create_buttons_box(GUIChatSession * gcs)
{
    GtkWidget *hbox_buttons;
    GtkWidget *button_send;
    GtkWidget *button_autosend;
    GtkWidget *button_find;
    GtkWidget *button_close;
/*
    GtkWidget *button_emoticons;
*/
    GtkWidget *button_stick;

    hbox_buttons = gtk_hbox_new(FALSE, 0);

    /*
     * buttons 
     */
    button_send = gtk_button_new_with_mnemonic(_("_Send"));

    button_autosend = gtk_toggle_button_new();
    gtk_container_add(GTK_CONTAINER(button_autosend), create_image("arrow.png"));

    button_find = gtk_button_new_from_stock("gtk-find");
    button_close = gtk_button_new_from_stock("gtk-close");
    button_stick = gtk_toggle_button_new_with_mnemonic(_("S_tick"));

    gtk_button_set_relief(GTK_BUTTON(button_send), GTK_RELIEF_NONE);
    gtk_button_set_relief(GTK_BUTTON(button_autosend), GTK_RELIEF_NONE);
    gtk_button_set_relief(GTK_BUTTON(button_find), GTK_RELIEF_NONE);
    gtk_button_set_relief(GTK_BUTTON(button_close), GTK_RELIEF_NONE);
    gtk_button_set_relief(GTK_BUTTON(button_stick), GTK_RELIEF_NONE);

    gtk_box_pack_start(GTK_BOX(hbox_buttons), button_send, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), button_autosend, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), button_find, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox_buttons), button_close, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox_buttons), button_stick, FALSE, FALSE, 0);

/*
    g_object_set_data (G_OBJECT (session->chat), "autosend_button", button_autosend);
    g_signal_connect (G_OBJECT (input), "key-press-event", G_CALLBACK (on_input_press_event), session);

    g_signal_connect (chat_window, "configure-event", G_CALLBACK (window_resize_signal), session);
    g_signal_connect (button_autosend, "clicked", G_CALLBACK (on_autosend_clicked), session);
    g_signal_connect (button_send, "clicked", G_CALLBACK (on_send_clicked), session);
    g_signal_connect (button_stick, "toggled", G_CALLBACK (on_stick_clicked), session);
    g_signal_connect (button_find, "clicked", G_CALLBACK (on_chat_find_clicked), session);

    switch (chat_type)
      {
      case CHAT_TYPE_TABBED:
	  g_signal_connect (button_close, "clicked", G_CALLBACK (on_destroy_chat), NULL);
	  g_signal_connect (chat_window, "destroy", G_CALLBACK (on_destroy_chat_window), NULL);
	  break;
      case CHAT_TYPE_CLASSIC:
	  g_signal_connect_swapped (button_close, "clicked", G_CALLBACK (gtk_widget_destroy), chat_window);
	  g_signal_connect (chat_window, "destroy", G_CALLBACK (on_destroy_chat), session);
	  break;
      }
*/

    if (ggadu_config_var_get(gui_handler, "send_on_enter"))
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_autosend), TRUE);

    gcs->buttons_widget = hbox_buttons;
    return hbox_buttons;

}

void gui_chat_sessions_create_visible_chat_window(GUIChatSession * gcs)
{
    GtkWidget *chat_window = NULL;
    gui_chat_sessions_create_chat_window(gcs);

    chat_window = gtk_widget_get_ancestor(gcs->widget, GTK_WINDOW_TOPLEVEL);

    gtk_widget_show_all(chat_window);
}

static void gui_chat_sessions_create_chat_window(GUIChatSession * gcs)
{
    gint chat_type = (gint) ggadu_config_var_get(gui_handler, "chat_type");
    GtkWidget *chat_window = NULL;
    GtkWidget *vbox = NULL;
    GdkPixbuf *image = NULL;

    switch (chat_type)
    {
    case CHAT_TYPE_CLASSIC:
	{
	    GtkWidget *buttons_hbox = NULL;

	    chat_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	    image = create_pixbuf(GGADU_DEFAULT_ICON_FILENAME);
	    gtk_window_set_icon(GTK_WINDOW(chat_window), image);
	    gdk_pixbuf_unref(image);
/*
	    gtk_window_set_title (GTK_WINDOW (chat_window), g_strdup (wintitle));
*/
	    gtk_window_set_wmclass(GTK_WINDOW(chat_window), "GM_NAME", "GNUGadu-chat");

	    buttons_hbox = gui_chat_session_create_buttons_box(gcs);

	    gtk_box_pack_start(GTK_BOX(vbox), gcs->widget, TRUE, TRUE, 0);
	    gtk_box_pack_end(GTK_BOX(vbox), gcs->buttons_widget, FALSE, FALSE, 0);

	    gtk_container_add(GTK_CONTAINER(chat_window), vbox);
	}
	break;
    case CHAT_TYPE_TABBED:
	{
	}
	break;
    }
}

GtkWidget *gui_chat_session_create_gtk_widget(GUIChatSession * gcs)
{
    GtkWidget *widget = NULL;
    GtkWidget *history;
    GtkWidget *input;
    GtkWidget *paned;
    GtkWidget *sw;
    GtkTextBuffer *buf = NULL;
    gchar *colorstr = NULL;
    gchar *fontstr = NULL;

    g_return_val_if_fail(GUI_CHAT_SESSION_IS_SESSION(gcs), NULL);

    widget = gtk_vbox_new(FALSE, 0);	/* main container */

    /*
     * history 
     */
    history = gtk_text_view_new();
    gtk_widget_set_name(GTK_WIDGET(history), "GGHistory");

    gtk_text_view_set_editable(GTK_TEXT_VIEW(history), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(history), GTK_WRAP_CHAR);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(history), FALSE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(history), 2);

    gtk_widget_ref(history);
    g_object_set_data_full(G_OBJECT(widget), "history", history, (GDestroyNotify) gtk_widget_unref);

    buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(history));

    colorstr = ggadu_config_var_get(gui_handler, "msg_header_color");
    fontstr = ggadu_config_var_get(gui_handler, "msg_header_font");

    gtk_text_buffer_create_tag(buf, "incoming_header", "foreground", (colorstr && (strlen(colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR, "font",
			       (fontstr) ? fontstr : DEFAULT_FONT, NULL);

    colorstr = ggadu_config_var_get(gui_handler, "msg_body_color");
    fontstr = ggadu_config_var_get(gui_handler, "msg_body_font");

    gtk_text_buffer_create_tag(buf, "incoming_text", "foreground", (colorstr && (strlen(colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR, "font",
			       (fontstr) ? fontstr : DEFAULT_FONT, NULL);

    colorstr = ggadu_config_var_get(gui_handler, "msg_out_header_color");
    fontstr = ggadu_config_var_get(gui_handler, "msg_out_header_font");

    gtk_text_buffer_create_tag(buf, "outgoing_header", "foreground", (colorstr && (strlen(colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR, "font",
			       (fontstr) ? fontstr : DEFAULT_FONT, NULL);

    colorstr = ggadu_config_var_get(gui_handler, "msg_out_body_color");
    fontstr = ggadu_config_var_get(gui_handler, "msg_out_body_font");

    gtk_text_buffer_create_tag(buf, "outgoing_text", "foreground", (colorstr && (strlen(colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR, "font",
			       (fontstr) ? fontstr : DEFAULT_FONT, NULL);

    /*
     * input 
     */
    input = gtk_text_view_new();
    gtk_widget_set_name(GTK_WIDGET(input), "GGInput");
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(input), GTK_WRAP_WORD);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(input), 2);

    gtk_widget_ref(input);
    g_object_set_data_full(G_OBJECT(widget), "input", input, (GDestroyNotify) gtk_widget_unref);

    /*
     * paned 
     */
    paned = gtk_vpaned_new();

    /*
     * SW1 
     */
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

    gtk_container_add(GTK_CONTAINER(sw), history);
    gtk_paned_add1(GTK_PANED(paned), sw);

    /*
     * SW2 
     */
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(sw), input);
    gtk_paned_add2(GTK_PANED(paned), sw);

    /* attach paned to widget */
    gtk_box_pack_start(GTK_BOX(widget), paned, TRUE, TRUE, 0);

    gcs->widget = widget;

    return widget;
}

/* 
 * private 
 */

static GObject *gui_chat_session_constructor(GType type, guint n_construct_properties, GObjectConstructParam * construct_properties)
{
    GObject *obj;

    {
	/* Invoke parent constructor. */
	GUIChatSessionClass *klass;
	GObjectClass *parent_class;
	klass = GUI_CHAT_SESSION_CLASS(g_type_class_peek(GUI_CHAT_SESSION_TYPE));
	parent_class = G_OBJECT_CLASS(g_type_class_peek_parent(klass));
	obj = parent_class->constructor(type, n_construct_properties, construct_properties);
    }

    /* do stuff. */


    return obj;
}

static void gui_chat_session_instance_init(GTypeInstance * instance, gpointer g_class)
{
/*
    GUIChatSession *self = (GUIChatSession *) instance;
*/
    /* do stuff */
}

static void gui_chat_session_class_init(gpointer g_class, gpointer g_class_data)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(g_class);
/*
    GUIChatSession *klass = GUI_CHAT_SESSION_CLASS (g_class);
*/
    gobject_class->constructor = gui_chat_session_constructor;
}

GType gui_chat_session_get_type(void)
{
    static GType type = 0;

    if (type == 0)
    {
	static const GTypeInfo info = {
	    sizeof (GUIChatSessionClass),
	    NULL,		/* base_init */
	    NULL,		/* base_finalize */
	    gui_chat_session_class_init,	/* class_init */
	    NULL,		/* class_finalize */
	    NULL,		/* class_data */
	    sizeof (GUIChatSession),
	    0,			/* n_preallocs */
	    gui_chat_session_instance_init	/* instance_init */
	};

	type = g_type_register_static(G_TYPE_OBJECT, "GUIChatSessionType", &info, 0);
    }

    return type;
}
