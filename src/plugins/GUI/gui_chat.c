/* $Id: gui_chat.c,v 1.123 2004/10/15 14:42:52 krzyzak Exp $ */

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
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "ggadu_support.h"
#include "signals.h"
#include "plugins.h"
#include "ggadu_conf.h"
#include "ggadu_dialog.h"
#include "GUI_plugin.h"
#include "gui_main.h"
#include "gui_support.h"
#include "gui_chat.h"
#include "gui_handlers.h"

extern GSList *protocols;
extern GSList *emoticons;
extern GSList *invisible_chats;
extern GGaduPlugin *gui_handler;

static GtkWidget *chat_window = NULL;

static void gui_chat_notebook_switch(GtkWidget * notebook, GtkNotebookPage * page, guint page_num, gpointer user_data)
{
	GtkWidget *chat_notebook = NULL;
	GtkWidget *chat = NULL;
	GtkWidget *lb = NULL;
	gchar *txt = NULL;
	const gchar *txt2 = NULL;

	if (chat_window)
		chat_notebook = g_object_get_data(G_OBJECT(chat_window), "chat_notebook");
	else
		return;

	if ((chat_notebook) && (chat_window))
		chat = (GtkWidget *) gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook), page_num);
	else
		return;

	if (chat)
		lb = g_object_get_data(G_OBJECT(chat), "tab_label_txt");
	else
		return;

	if (lb)
	{
		txt = (gchar *) g_object_get_data(G_OBJECT(chat), "tab_label_txt_char");
		txt2 = (const gchar *) g_strdup(g_object_get_data(G_OBJECT(chat), "tab_window_title_char"));
	}
	else
		return;

	if ((txt != NULL) && (chat_window != NULL) && (lb != NULL) && (txt2 != NULL))
	{
		gchar *markup = g_strdup_printf("<span foreground=\"black\">%s</span>", txt);
		gtk_window_set_title(GTK_WINDOW(chat_window), txt2);
		gtk_label_set_markup(GTK_LABEL(lb), markup);

		g_free(markup);
	}

	print_debug("gui_chat_notebook_switch");
}

static void on_destroy_chat_window(GtkWidget * chat, gpointer user_data)
{
	if (G_IS_OBJECT(chat) && GTK_IS_WINDOW(chat))
		gui_remove_all_chat_sessions(protocols);

	print_debug("destroy_chat_window");
	chat_window = NULL;
}


static void on_destroy_chat(GtkWidget * button, gpointer user_data)
{
	gint chat_type = (gint) ggadu_config_var_get(gui_handler, "chat_type");
	gui_chat_session *session = NULL;
	gui_protocol *gp = NULL;
	gchar *plugin_name = NULL;

	print_debug("on_destroy_chat");

	switch (chat_type)
	{
	case CHAT_TYPE_TABBED:
		{
			gint sess_count, tab_count;
			guint nr;
			GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window),
								     "chat_notebook");
			GtkWidget *chat = NULL;
			GtkWidget *input = NULL;

			if (!user_data)
			{
				nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
			}
			else
			{
				gui_chat_session *s = (gui_chat_session *) user_data;
				nr = gtk_notebook_page_num(GTK_NOTEBOOK(chat_notebook), s->chat);
			}

			chat = gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook), nr);

			plugin_name = g_object_get_data(G_OBJECT(chat), "plugin_name");

			session = (gui_chat_session *) g_object_get_data(G_OBJECT(chat), "gui_session");
			gp = gui_find_protocol(plugin_name, protocols);

			input = g_object_get_data(G_OBJECT(chat), "input");

#ifdef USE_GTKSPELL
			if (ggadu_config_var_get(gui_handler, "use_spell"))
			{
				GtkSpell *spell = NULL;
				if ((spell = gtkspell_get_from_text_view(GTK_TEXT_VIEW(input))) != NULL)
					gtkspell_detach(spell);
			}
#endif
			gtk_notebook_remove_page(GTK_NOTEBOOK(chat_notebook), nr);


			gtk_notebook_set_show_tabs(GTK_NOTEBOOK(chat_notebook),
						   (gtk_notebook_get_n_pages(GTK_NOTEBOOK(chat_notebook)) <=
						    1) ? FALSE : TRUE);

			gtk_widget_queue_draw(GTK_WIDGET(chat_notebook));

			gp->chat_sessions = g_slist_remove(gp->chat_sessions, session);
			g_free(session);

			sess_count = g_slist_length(gp->chat_sessions);
			tab_count = gui_count_visible_tabs(GTK_NOTEBOOK(chat_notebook));

			if ((sess_count == 0) && (tab_count == 0))
				gtk_widget_destroy(chat_window);
			else if ((sess_count > 0) && (tab_count == 0))
				gtk_widget_hide(chat_window);
			else
				gui_chat_notebook_switch(chat_notebook, NULL,
							 gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook)),
							 NULL);

		}
		break;
	case CHAT_TYPE_CLASSIC:
		{
			GtkWidget *input = NULL;
			session = (gui_chat_session *) user_data;

			input = g_object_get_data(G_OBJECT(session->chat), "input");

#ifdef USE_GTKSPELL
			if (ggadu_config_var_get(gui_handler, "use_spell"))
			{
				GtkSpell *spell = NULL;
				if ((spell = gtkspell_get_from_text_view(GTK_TEXT_VIEW(input))) != NULL)
					gtkspell_detach(spell);
			}
#endif
			plugin_name = g_object_get_data(G_OBJECT(session->chat), "plugin_name");
			if (!plugin_name)
				return;
			gp = gui_find_protocol(plugin_name, protocols);

			chat_window = NULL;
			gtk_widget_destroy(button);
			session->chat = NULL;
			gp->chat_sessions = g_slist_remove(gp->chat_sessions, session);
			g_free(session);
		}
		break;
	}

	print_debug("main-gui : chat : zwalniam session");
}

static gboolean on_press_event_switching_tabs(gpointer object, GdkEventKey * event, gpointer user_data)
{
	gint chat_type = (gint) ggadu_config_var_get(gui_handler, "chat_type");

	if ((chat_type == CHAT_TYPE_TABBED) && (event->state & GDK_CONTROL_MASK))
	{
		GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window), "chat_notebook");
		gint act_tab_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));

		if (event->keyval == GDK_Tab)
		{
			act_tab_page++;

			if (act_tab_page >= gtk_notebook_get_n_pages(GTK_NOTEBOOK(chat_notebook)))
				act_tab_page = 0;

			gtk_notebook_set_current_page(GTK_NOTEBOOK(chat_notebook), act_tab_page);
			return TRUE;
		}
	}

	return FALSE;
}

void on_clear_clicked(GtkWidget * button, gpointer user_data)
{
	gint chat_type = (gint) ggadu_config_var_get(gui_handler, "chat_type");
	gui_chat_session *session = NULL;
	GtkWidget *textview = NULL;
	
	if (chat_type == CHAT_TYPE_TABBED)
	{
		GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window), "chat_notebook");
		guint nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
		GtkWidget *chat = gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook), nr);

		textview = g_object_get_data(G_OBJECT(chat), "history");
	}
	else if (chat_type == CHAT_TYPE_CLASSIC)
	{
		session = user_data;
		textview = g_object_get_data(G_OBJECT(session->chat), "history");
	}
	
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	gtk_text_buffer_set_text(buf, "", -1);
}

void on_autosend_clicked(GtkWidget * button, gpointer user_data)
{
	gint chat_type = (gint) ggadu_config_var_get(gui_handler, "chat_type");

	ggadu_config_var_set(gui_handler, "send_on_enter",
			     (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));
	/*
	 * change in every open session 
	 */
	if (chat_type == CHAT_TYPE_CLASSIC)
	{
		GSList *protocols_local = (GSList *) protocols;

		while (protocols_local)
		{
			gui_protocol *protocol = (gui_protocol *) protocols_local->data;
			GSList *sessions = (GSList *) protocol->chat_sessions;

			while (sessions)
			{
				gui_chat_session *s = (gui_chat_session *) sessions->data;
				GtkWidget *autosend_button = g_object_get_data(G_OBJECT(s->chat),
									       "autosend_button");
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(autosend_button),
							     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));
				sessions = sessions->next;
			}

			protocols_local = protocols_local->next;
		}
	}
}

static void on_send_clicked(GtkWidget * button, gpointer user_data)
{
	gint chat_type = (gint) ggadu_config_var_get(gui_handler, "chat_type");
	gui_chat_session *session = NULL;
	GGaduMsg *msg = NULL;
	gchar *plugin_name = NULL;
	GtkWidget *input = NULL;
	GtkTextBuffer *buf = NULL;
	GtkTextIter start, end;
	gchar *tmpmsg = NULL;

	if (chat_type == CHAT_TYPE_TABBED)
	{
		GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window), "chat_notebook");
		guint nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
		GtkWidget *chat = gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook), nr);

		input = g_object_get_data(G_OBJECT(chat), "input");
		plugin_name = g_object_get_data(G_OBJECT(chat), "plugin_name");
		session = (gui_chat_session *) g_object_get_data(G_OBJECT(chat), "gui_session");

	}
	else if (chat_type == CHAT_TYPE_CLASSIC)
	{

		session = (gui_chat_session *)user_data;
		input = g_object_get_data(G_OBJECT(session->chat), "input");
		plugin_name = g_object_get_data(G_OBJECT(session->chat), "plugin_name");
	}

	g_return_if_fail(input != NULL);

	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(input));
	gtk_text_buffer_get_start_iter(buf, &start);
	gtk_text_buffer_get_end_iter(buf, &end);

	tmpmsg = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
	
	if (tmpmsg)
	{
	    tmpmsg = g_strchug(g_strchomp(tmpmsg));	/* obciete puste spacje z obu stron */
	}

	if (tmpmsg && strlen(tmpmsg) > 0)
	{
		gchar *soundfile = NULL;
		msg = g_new0(GGaduMsg, 1);
		msg->id = g_strdup(session->id);
		msg->message = tmpmsg;

		msg->class = (g_slist_length(session->recipients) > 1) ? GGADU_CLASS_CONFERENCE : GGADU_CLASS_CHAT;
		msg->recipients = g_slist_copy(session->recipients);

		/* it's me who send it */
		gui_chat_append(session->chat, msg->message, TRUE, FALSE);
		if ((soundfile = ggadu_config_var_get(gui_handler, "sound_msg_out")))
		{
			signal_emit_full("main-gui", "sound play file", soundfile, "sound*", NULL);
		}
		
		signal_emit_full("main-gui", "send message", msg, plugin_name, GGaduMsg_free);
		auto_away_start( gui_find_protocol(plugin_name, protocols) );
	}
	else if (tmpmsg)
	{
		g_free(tmpmsg);
	}

	gtk_text_buffer_set_text(buf, "", -1);
	gtk_widget_grab_focus(GTK_WIDGET(input));
}


static gboolean on_input_press_event(gpointer object, GdkEventKey * event, gpointer user_data)
{
	gui_chat_session *s = user_data;

	if ((event->keyval == GDK_Return) && (ggadu_config_var_get(gui_handler, "send_on_enter")))
	{
		print_debug("main-gui : chat : wcisnieto Enter \n");

		if (!(event->state & GDK_SHIFT_MASK))
		{
			on_send_clicked(s->chat, user_data);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	return (event->state & GDK_CONTROL_MASK) ? on_press_event_switching_tabs(object, event, user_data) : FALSE;
}

static void on_stick_clicked(GtkWidget * button, gpointer user_data)
{
	gui_chat_session *s = user_data;
	gboolean q = FALSE;
	GtkWidget *win = NULL;

	if ((!s) || (!s->chat))
		return;

	q = (gboolean) g_object_get_data(G_OBJECT(s->chat), "stick");
	win = g_object_get_data(G_OBJECT(s->chat), "top_window");

	if (q)
	{
		g_object_set_data(G_OBJECT(s->chat), "stick", (gpointer) FALSE);
		gtk_window_unstick(GTK_WINDOW(win));
	}
	else
	{
		g_object_set_data(G_OBJECT(s->chat), "stick", (gpointer) TRUE);
		gtk_window_stick(GTK_WINDOW(win));
	}
}

static void on_emoticon_press_event(GtkWidget * event_box, GdkEventButton * event, gpointer user_data)
{
	gint chat_type = (gint) ggadu_config_var_get(gui_handler, "chat_type");
	GtkWidget *input;
	GtkWidget *emoticons_window;
	gui_emoticon *gemo = user_data;
	GtkTextBuffer *buf;
	GtkTextIter end;
	gui_chat_session *session;

	/*
	 * ZONK 
	 */
	emoticons_window = lookup_widget(event_box, "emoticons_window");

	g_return_if_fail(emoticons_window != NULL);

	
	if (chat_type == CHAT_TYPE_TABBED)
	{
		GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window), "chat_notebook");
		guint nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
		GtkWidget *chat = gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook), nr);

		session = (gui_chat_session *) g_object_get_data(G_OBJECT(chat), "gui_session");
	} else {
		session = g_object_get_data(G_OBJECT(emoticons_window), "session");
	}
	
	input = g_object_get_data(G_OBJECT(session->chat), "input");

	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(input));
	gtk_text_buffer_get_end_iter(buf, &end);

	gtk_text_buffer_insert(buf, &end, gemo->emoticon, -1);

	gtk_widget_hide(emoticons_window);
}

static gboolean find_emoticon(gchar * name, GSList * here)
{
	GSList *tmp = here;

	while (tmp)
	{
		gui_emoticon *e = (gui_emoticon *) tmp->data;

		if (!ggadu_strcasecmp(name, e->file))
			return TRUE;

		tmp = tmp->next;
	}

	return FALSE;
}

static void on_emoticons_clicked(GtkWidget * button, gpointer user_data)
{
	gint chat_type = (gint) ggadu_config_var_get(gui_handler, "chat_type");
	GtkWidget *emoticons_window = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *outervbox = NULL;
	GtkWidget *button_close = NULL;
	GtkWidget *scrolledwindow1;
	GtkWidget *viewport1;
	gui_chat_session *session = NULL;

	if (chat_type == CHAT_TYPE_TABBED)
	{
		GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window), "chat_notebook");
		guint nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
		GtkWidget *chat = gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook), nr);

		session = (gui_chat_session *) g_object_get_data(G_OBJECT(chat), "gui_session");

	}
	else if (chat_type == CHAT_TYPE_CLASSIC)
	{
		session = user_data;
	}

	emoticons_window = g_object_get_data(G_OBJECT(button), "emoticons_window");
	if (emoticons_window == NULL)
	{
		emoticons_window = gtk_window_new(GTK_WINDOW_POPUP);
		g_object_set_data(G_OBJECT(button),"emoticons_window",emoticons_window);
		/* destroy window when button is destroyed */
		g_signal_connect(G_OBJECT(button),"destroy",G_CALLBACK(gtk_widget_destroy),emoticons_window);
		
		gtk_window_set_modal(GTK_WINDOW(emoticons_window), TRUE);
		gtk_window_set_position(GTK_WINDOW(emoticons_window), GTK_WIN_POS_MOUSE);

		gtk_widget_set_usize(emoticons_window, 510, 300);

		g_object_set_data(G_OBJECT(emoticons_window), "session", session);

		g_object_set_data(G_OBJECT(emoticons_window), "emoticons_window", emoticons_window);


		/*
		 * dodajê po drodze viewport i scroll Pawe³ Marchewka
		 * <pmgiant@student.uci.agh.edu.pl> 12.04.2003 
		 */
		outervbox = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(emoticons_window), outervbox);

		scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
		gtk_widget_set_name(scrolledwindow1, "scrolledwindow1");
		gtk_widget_ref(scrolledwindow1);
		gtk_object_set_data_full(GTK_OBJECT(emoticons_window), "scrolledwindow1", scrolledwindow1,
					 (GtkDestroyNotify) gtk_widget_unref);
		gtk_box_pack_start(GTK_BOX(outervbox), scrolledwindow1, TRUE, TRUE, 0);

		gtk_container_set_border_width(GTK_CONTAINER(scrolledwindow1), 5);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1), GTK_POLICY_NEVER,
					       GTK_POLICY_ALWAYS);

		viewport1 = gtk_viewport_new(NULL, NULL);
		gtk_widget_set_name(viewport1, "viewport1");
		gtk_widget_ref(viewport1);
		gtk_object_set_data_full(GTK_OBJECT(emoticons_window), "viewport1", viewport1,
					 (GtkDestroyNotify) gtk_widget_unref);
		gtk_container_add(GTK_CONTAINER(scrolledwindow1), viewport1);

		vbox = gtk_vbox_new(TRUE, 0);
		gtk_container_add(GTK_CONTAINER(viewport1), vbox);

		if (emoticons)
		{
			GSList *emotlist = NULL;
			GSList *emottmp = emoticons;
			GtkWidget *widget;
			GtkWidget *hbox;
			GtkWidget *event_box;
			gint count = 0;

			hbox = gtk_hbox_new(TRUE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

			while (emottmp)
			{
				gui_emoticon *gemo = (gui_emoticon *) emottmp->data;

				if (!find_emoticon(gemo->file, emotlist))
					emotlist = g_slist_append(emotlist, gemo);

				emottmp = emottmp->next;
			}

			emottmp = emotlist;

			while (emottmp)
			{
				GtkTooltips *tip;
				gui_emoticon *gemo = (gui_emoticon *) emottmp->data;
				widget = create_image(gemo->file);
				event_box = gtk_event_box_new();
				if (widget)
				{
					if (count >= MAX_EMOTICONS_IN_ROW)
					{
						hbox = gtk_hbox_new(TRUE, 0);
						gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
						count = 0;
					}

					gtk_container_add(GTK_CONTAINER(event_box), widget);
					gtk_box_pack_start(GTK_BOX(hbox), event_box, FALSE, FALSE, 0);
					gtk_widget_set_usize(event_box, 60, 30);

					tip = gtk_tooltips_new();
					gtk_tooltips_set_tip(tip, event_box, gemo->emoticon, gemo->file);
					g_signal_connect(event_box, "button_press_event",
							 G_CALLBACK(on_emoticon_press_event), gemo);
					count++;
				}
				emottmp = emottmp->next;

			}
			g_slist_free(emotlist);
		}

		button_close = gtk_button_new_with_mnemonic(_("_Close"));

		gtk_box_pack_start(GTK_BOX(outervbox), button_close, FALSE, FALSE, 0);

		/*g_signal_connect_swapped(button_close, "clicked", G_CALLBACK(gtk_widget_destroy), emoticons_window);*/
		g_signal_connect_swapped(button_close, "clicked", G_CALLBACK(gtk_widget_hide), emoticons_window);
	}

	gtk_widget_show_all(emoticons_window);
}

static void on_chat_find_clicked(GtkWidget * button, gpointer user_data)
{
	gint chat_type = (gint) ggadu_config_var_get(gui_handler, "chat_type");
	GGaduDialog *dialog = g_new0(GGaduDialog, 1);
	gui_chat_session *session = NULL;
	gchar *plugin_name = NULL;

	if (chat_type == CHAT_TYPE_TABBED)
	{
		GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window), "chat_notebook");
		guint nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
		GtkWidget *chat = gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook), nr);

		plugin_name = g_object_get_data(G_OBJECT(chat), "plugin_name");
		session = (gui_chat_session *) g_object_get_data(G_OBJECT(chat), "gui_session");

	}
	else if (chat_type == CHAT_TYPE_CLASSIC)
	{
		session = user_data;
		plugin_name = g_object_get_data(G_OBJECT(session->chat), "plugin_name");
	}


	print_debug("SEARCH %s\n", session->id);

	ggadu_dialog_add_entry(dialog, GGADU_SEARCH_ID, NULL, VAR_STR, session->id, VAR_FLAG_NONE);

	dialog->response = GGADU_OK;
	signal_emit("main-gui", "search", dialog, plugin_name);
}

void gui_chat_update_tags()
{
	GSList *tmplist = protocols;
	GSList *tmpsessions;

	while (tmplist)
	{
		gui_protocol *gp = tmplist->data;
		tmpsessions = gp->chat_sessions;

		while (tmpsessions)
		{
			gui_chat_session *session = tmpsessions->data;
			GtkWidget *history = g_object_get_data(G_OBJECT(session->chat),
							       "history");
			GtkTextBuffer *buf;
			GtkTextTagTable *tagtable;
			GtkTextTag *tag;
			gchar *tagstr;

			buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(history));
			tagtable = gtk_text_buffer_get_tag_table(buf);

			tag = gtk_text_tag_table_lookup(tagtable, "incoming_header");

			tagstr = ggadu_config_var_get(gui_handler, "msg_header_color");
			g_object_set(G_OBJECT(tag), "foreground", (tagstr) ? tagstr : DEFAULT_TEXT_COLOR, NULL);

			tagstr = ggadu_config_var_get(gui_handler, "msg_header_font");
			g_object_set(G_OBJECT(tag), "font", (tagstr) ? tagstr : DEFAULT_FONT, NULL);

			tag = gtk_text_tag_table_lookup(tagtable, "incoming_text");

			tagstr = ggadu_config_var_get(gui_handler, "msg_body_color");
			g_object_set(G_OBJECT(tag), "foreground", (tagstr) ? tagstr : DEFAULT_TEXT_COLOR, NULL);

			tagstr = ggadu_config_var_get(gui_handler, "msg_body_font");
			g_object_set(G_OBJECT(tag), "font", (tagstr) ? tagstr : DEFAULT_FONT, NULL);

			tag = gtk_text_tag_table_lookup(tagtable, "outgoing_header");

			tagstr = ggadu_config_var_get(gui_handler, "msg_out_header_color");
			g_object_set(G_OBJECT(tag), "foreground", (tagstr) ? tagstr : DEFAULT_TEXT_COLOR, NULL);

			tagstr = ggadu_config_var_get(gui_handler, "msg_out_header_font");
			g_object_set(G_OBJECT(tag), "font", (tagstr) ? tagstr : DEFAULT_FONT, NULL);

			tag = gtk_text_tag_table_lookup(tagtable, "outgoing_text");

			tagstr = ggadu_config_var_get(gui_handler, "msg_out_body_color");
			g_object_set(G_OBJECT(tag), "foreground", (tagstr) ? tagstr : DEFAULT_TEXT_COLOR, NULL);

			tagstr = ggadu_config_var_get(gui_handler, "msg_out_body_font");
			g_object_set(G_OBJECT(tag), "font", (tagstr) ? tagstr : DEFAULT_FONT, NULL);

			tmpsessions = tmpsessions->next;
		}
		tmplist = tmplist->next;
	}
}

static gboolean window_event_signal(GtkWidget *window, GdkEvent *event, gpointer user_data)
{
	if  (((event->type == GDK_WINDOW_STATE) || (event->type == GDK_FOCUS_CHANGE)) 
	    && g_object_get_data(G_OBJECT(window),"new-message-mark"))
	{
	    GdkPixbuf *image = NULL;
	    print_debug("chat_window : GDK_WINDOW_STATE || GDK_FOCUS_CHANGE");
	    image = create_pixbuf(GGADU_DEFAULT_ICON_FILENAME);
	    gtk_window_set_icon(GTK_WINDOW(window), image);
	    gdk_pixbuf_unref(image);
	    g_object_set_data(G_OBJECT(window),"new-message-mark",(gpointer) FALSE);
	}
	    
	return FALSE;
}

static gboolean window_resize_signal(GtkWidget * window, GdkEventConfigure * event, gpointer user_data)
{
	if (event->send_event == FALSE)
	{
		gint chat_type = (gint) ggadu_config_var_get(gui_handler, "chat_type");
		gint percent = (gint) ggadu_config_var_get(gui_handler, "chat_paned_size");
		gui_chat_session *session;
		GtkWidget *chat;
		GtkWidget *paned;
		GtkWidget *hbox_buttons;
		GtkRequisition r;
		GtkRequisition rb;
		float tab_minus = 0;
		float position = 0;

		if (chat_type == CHAT_TYPE_CLASSIC)
		{
			session = user_data;
			chat = session->chat;

			paned = g_object_get_data(G_OBJECT(chat), "paned");
			hbox_buttons = g_object_get_data(G_OBJECT(chat), "hbox_buttons");

			if ((paned != NULL) && (GTK_IS_PANED(paned)))
			{

				r.height = event->height;
				if (hbox_buttons)
					gtk_widget_size_request(GTK_WIDGET(hbox_buttons), &rb);
				position =
					((((float) r.height - (float) rb.height) / 100) * (float) percent) + tab_minus;
				gtk_paned_set_position(GTK_PANED(paned), (gint) position);
			}
		}
		else
		{
			GtkWidget *chat_notebook;
			gint current_page = 0;
			gint num_pages;

			if (chat_window)
				chat_notebook = g_object_get_data(G_OBJECT(chat_window), "chat_notebook");

			if ((chat_notebook) && (chat_window))
				num_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(chat_notebook));

			while (current_page < num_pages)
			{
				chat = (GtkWidget *) gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook),
									       current_page);

				paned = g_object_get_data(G_OBJECT(chat), "paned");
				hbox_buttons = g_object_get_data(G_OBJECT(chat), "hbox_buttons");

				if ((paned != NULL) && (GTK_IS_PANED(paned)))
				{

					GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(window), "chat_notebook");
					if (gtk_notebook_get_show_tabs(GTK_NOTEBOOK(chat_notebook)))
					{
						GtkWidget *label =
							gtk_notebook_get_tab_label(GTK_NOTEBOOK(chat_notebook),
										   GTK_WIDGET(chat));
						GtkRequisition rbnl;
						gtk_widget_size_request(GTK_WIDGET(label), &rbnl);
						tab_minus = -rbnl.height;
					}
					r.height = event->height;
					if (hbox_buttons)
						gtk_widget_size_request(GTK_WIDGET(hbox_buttons), &rb);
					position =
						((((float) r.height - (float) rb.height) / 100) * (float) percent) +
						tab_minus;
					gtk_paned_set_position(GTK_PANED(paned), (gint) position);
				}
				current_page++;
			}
		}
	}
	return FALSE;
}


GtkWidget *create_chat(gui_chat_session * session, gchar * plugin_name, gchar * id, gboolean visible)
{
	gint chat_type = (gint) ggadu_config_var_get(gui_handler, "chat_type");
	gint width = (gint) ggadu_config_var_get(gui_handler, "chat_window_width");
	gint height = (gint) ggadu_config_var_get(gui_handler, "chat_window_height");
	gboolean conference;	/* Conference ? */
	GGaduContact *k = NULL;	/* Contact that we talk to */
	gui_protocol *gp = NULL;	/* data for this protocol */
	gchar *status_desc = NULL;	/* */
	GtkWidget *history;
	GtkWidget *input;
	GtkWidget *paned;
	GtkWidget *sw;
	GtkWidget *vbox = NULL;
	GtkWidget *vbox_in_out = NULL;
	GtkWidget *hbox_buttons;
	GtkWidget *button_send;
	GtkWidget *button_autosend;
	GtkWidget *button_find;
	GtkWidget *button_close;
	GtkWidget *button_emoticons;
	GtkWidget *button_stick;
	GtkWidget *button_clear;
	GtkWidget *bs_image = create_image("send-im.png");
	GtkWidget *bas_image;
	GtkWidget *bs_hbox = gtk_hbox_new(FALSE, 0);
	GtkWidget *bs_label = gtk_label_new_with_mnemonic(_("_Send"));
	GtkTooltips *tips = gtk_tooltips_new();
	GtkTextBuffer *buf = NULL;
	gchar *wintitle = NULL;
	gchar *confer_title = NULL;
	gchar *colorstr = NULL;
	gchar *fontstr = NULL;
	GGaduStatusPrototype *sp = NULL;
	GdkPixbuf *image = NULL;
	GtkRequisition r, rb;
	float position = 0;
	float tab_minus = 0;
	gint percent = 0;
	GtkWidget *chat_notebook_paned_size = NULL;

	if (!session || !plugin_name || !id)
		return NULL;


	conference = (g_slist_length(session->recipients) > 1) ? TRUE : FALSE;

	gp = gui_find_protocol(plugin_name, protocols);

	if (!gp)
		return NULL;

	k = gui_find_user(id, gui_find_protocol(plugin_name, protocols));

	/*
	 * confer_topic create 
	 */
	if (conference)
	{
		guint i = 0;
		GSList *tmprl = session->recipients;
		gchar *recipients_str_arr[g_slist_length(session->recipients)];

		while (tmprl)
		{
			GGaduContact *k1 = NULL;
			gchar *recipient = (gchar *) tmprl->data;

			k1 = gui_find_user(recipient, gui_find_protocol(plugin_name, protocols));
			recipients_str_arr[i++] = (k1) ? (gchar *) k1->nick : recipient;

			tmprl = tmprl->next;
		}

		recipients_str_arr[i] = NULL;
		confer_title = g_strjoinv(",", recipients_str_arr);
	};


	vbox = gtk_vbox_new(FALSE, 0);	/* up - vbox_in_out, down -
					 * buttons */
	hbox_buttons = gtk_hbox_new(FALSE, 0);	/* buttons hbox */
	/* hbox_buttons = gtk_hbutton_box_new(); */


	vbox_in_out = gtk_vbox_new(FALSE, 0);	/* up - input, down -
						 * history */
	session->chat = vbox_in_out;

	g_object_set_data(G_OBJECT(session->chat), "gui_session", session);

	/*
	 * find status description 
	 */
	if (k)
	{
		sp = ggadu_find_status_prototype((GGaduProtocol *) gp->p, (gint) k->status);
		status_desc = g_strdup_printf("- (%s)", (sp ? sp->description : ""));

		if ((k->status_descr) && (strlen(k->status_descr) > 0))
			status_desc = g_strdup_printf("- %s (%s)", (sp ? sp->description : ""), k->status_descr);
		else
			status_desc = g_strdup_printf("- %s", (sp ? sp->description : ""));
	}
	/*
	 * create approp. window style - tabbed or stand alone 
	 */
	switch (chat_type)
	{
	case CHAT_TYPE_TABBED:
		{
			gchar *title = NULL;
			GtkWidget *chat_notebook = NULL;
			GtkWidget *tab_label_hbox = NULL;
			GtkWidget *tab_label_txt = NULL;
			GtkWidget *tab_label_close = NULL;
			GtkWidget *tab_label_close_image = gtk_image_new_from_pixbuf(NULL);	/* empty */

			if (chat_window == NULL)
			{
				chat_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

				image = create_pixbuf(GGADU_DEFAULT_ICON_FILENAME);
				gtk_window_set_icon(GTK_WINDOW(chat_window), image);
				gdk_pixbuf_unref(image);

				gtk_window_set_title(GTK_WINDOW(chat_window), _("Chat Window"));
				gtk_window_set_wmclass(GTK_WINDOW(chat_window), "GM_NAME", "GNUGadu-chat");

				chat_notebook = gtk_notebook_new();
				g_object_set_data(G_OBJECT(chat_window), "chat_notebook", chat_notebook);

				gtk_notebook_set_scrollable(GTK_NOTEBOOK(chat_notebook), TRUE);

				gtk_box_pack_start(GTK_BOX(vbox), chat_notebook, TRUE, TRUE, 0);
				gtk_box_pack_end(GTK_BOX(vbox), hbox_buttons, FALSE, FALSE, 0);

				gtk_container_add(GTK_CONTAINER(chat_window), vbox);

				g_signal_connect(G_OBJECT(chat_notebook), "switch-page",
						 G_CALLBACK(gui_chat_notebook_switch), NULL);

				gtk_notebook_popup_enable(GTK_NOTEBOOK(chat_notebook));

			}
			else
			{
				chat_notebook = g_object_get_data(G_OBJECT(chat_window), "chat_notebook");
			}

			if (!chat_notebook)
				return NULL;

			/*
			 * for labels 
			 */
			title = (conference) ? g_strdup(confer_title) : g_strdup_printf("%s", k ? k->nick : id);
			/*
			 * for window title 
			 */
			wintitle =
				(conference) ? g_strdup(confer_title) : g_strdup_printf("%s %s", k ? k->nick : id,
											(sp) ? status_desc : "");

			tab_label_hbox = gtk_hbox_new(FALSE, FALSE);
			tab_label_txt = gtk_label_new(title);
			tab_label_close = gtk_button_new();

			gtk_button_set_relief(GTK_BUTTON(tab_label_close), GTK_RELIEF_NONE);
			g_signal_connect(tab_label_close, "clicked", G_CALLBACK(on_destroy_chat), session);

			gtk_widget_set_usize(GTK_WIDGET(tab_label_close), 12, 1);

			gtk_image_set_from_stock(GTK_IMAGE(tab_label_close_image), "gtk-close", GTK_ICON_SIZE_MENU);
			gtk_container_add(GTK_CONTAINER(tab_label_close), GTK_WIDGET(tab_label_close_image));

			gtk_box_pack_start_defaults(GTK_BOX(tab_label_hbox), tab_label_txt);
			gtk_box_pack_start_defaults(GTK_BOX(tab_label_hbox), tab_label_close);

			g_object_set_data(G_OBJECT(session->chat), "tab_label_txt", tab_label_txt);
			g_object_set_data_full(G_OBJECT(session->chat), "tab_label_txt_char", g_strdup(title), g_free);
			g_object_set_data_full(G_OBJECT(session->chat), "tab_window_title_char", g_strdup(wintitle),
					       g_free);

			/*
			 * append page 
			 */
			gtk_notebook_append_page(GTK_NOTEBOOK(chat_notebook), session->chat, tab_label_hbox);
			gtk_notebook_set_menu_label_text(GTK_NOTEBOOK(chat_notebook), session->chat, title);

			gtk_notebook_set_show_tabs(GTK_NOTEBOOK(chat_notebook),
						   (gtk_notebook_get_n_pages(GTK_NOTEBOOK(chat_notebook)) <=
						    1) ? FALSE : TRUE);

			gtk_widget_show_all(tab_label_hbox);

			g_free(title);
		}
		break;

	case CHAT_TYPE_CLASSIC:
		{
			if (k)
				wintitle =
					(conference) ? g_strdup(confer_title) : g_strdup_printf("%s (%s) %s", k->nick,
												id,
												(sp) ? status_desc :
												"");
			else
				wintitle = (conference) ? g_strdup(confer_title) : g_strdup_printf("%s", id);

			chat_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

			image = create_pixbuf(GGADU_DEFAULT_ICON_FILENAME);
			gtk_window_set_icon(GTK_WINDOW(chat_window), image);
			gdk_pixbuf_unref(image);

			gtk_window_set_title(GTK_WINDOW(chat_window), g_strdup(wintitle));
			gtk_window_set_wmclass(GTK_WINDOW(chat_window), "GM_NAME", "GNUGadu-chat");

			gtk_box_pack_start(GTK_BOX(vbox), vbox_in_out, TRUE, TRUE, 0);
			gtk_box_pack_end(GTK_BOX(vbox), hbox_buttons, FALSE, FALSE, 0);

			gtk_container_add(GTK_CONTAINER(chat_window), vbox);
		}
		break;
	}

	if (width < 50 || width > 2000)
		width = DEFAULT_CHAT_WINDOW_WIDTH;

	if (height < 50 || height > 2000)
		height = DEFAULT_CHAT_WINDOW_HEIGHT;

	gtk_window_set_default_size(GTK_WINDOW(chat_window), width, height);
	gtk_window_set_modal(GTK_WINDOW(chat_window), FALSE);
	gtk_widget_set_name(GTK_WIDGET(chat_window), "GGChat");

	g_object_set_data_full(G_OBJECT(session->chat), "plugin_name", g_strdup(plugin_name), g_free);
	g_object_set_data(G_OBJECT(session->chat), "top_window", chat_window);

	/*
	 * history 
	 */
	history = gtk_text_view_new();
	gtk_widget_set_name(GTK_WIDGET(history), "GGHistory");

	gtk_text_view_set_editable(GTK_TEXT_VIEW(history), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(history), GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(history), FALSE);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(history), 2);

	gtk_widget_ref(history);
	g_object_set_data_full(G_OBJECT(session->chat), "history", history, (GDestroyNotify) gtk_widget_unref);

	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(history));

	colorstr = ggadu_config_var_get(gui_handler, "msg_header_color");
	fontstr = ggadu_config_var_get(gui_handler, "msg_header_font");

	gtk_text_buffer_create_tag(buf, "incoming_header", "foreground",
				   (colorstr &&
				    (strlen(colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR, "font",
				   (fontstr) ? fontstr : DEFAULT_FONT, NULL);

	colorstr = ggadu_config_var_get(gui_handler, "msg_body_color");
	fontstr = ggadu_config_var_get(gui_handler, "msg_body_font");

	gtk_text_buffer_create_tag(buf, "incoming_text", "foreground",
				   (colorstr &&
				    (strlen(colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR, "font",
				   (fontstr) ? fontstr : DEFAULT_FONT, NULL);

	colorstr = ggadu_config_var_get(gui_handler, "msg_out_header_color");
	fontstr = ggadu_config_var_get(gui_handler, "msg_out_header_font");

	gtk_text_buffer_create_tag(buf, "outgoing_header", "foreground",
				   (colorstr &&
				    (strlen(colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR, "font",
				   (fontstr) ? fontstr : DEFAULT_FONT, NULL);

	colorstr = ggadu_config_var_get(gui_handler, "msg_out_body_color");
	fontstr = ggadu_config_var_get(gui_handler, "msg_out_body_font");

	gtk_text_buffer_create_tag(buf, "outgoing_text", "foreground",
				   (colorstr &&
				    (strlen(colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR, "font",
				   (fontstr) ? fontstr : DEFAULT_FONT, NULL);

	/*
	 * input 
	 */
	input = gtk_text_view_new();

#ifdef USE_GTKSPELL
	if (ggadu_config_var_get(gui_handler, "use_spell"))
	{
		GError *err = NULL;

		gchar *conf_dict = ggadu_config_var_get(gui_handler, "dictionary");
		gchar *dictionary = NULL;
		
		if (!conf_dict || (conf_dict && (!strcmp(conf_dict, "default"))))
			dictionary = g_strdup(g_getenv("LANG"));
		else
			dictionary = g_strdup(conf_dict);

		if (!gtkspell_new_attach(GTK_TEXT_VIEW(input), dictionary, &err))
		{
			GtkWidget *errdlg;
			errdlg = gtk_message_dialog_new(GTK_WINDOW(chat_window), GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
							_
							("Error initializing spell checking: \n%s\nTry install appropriate language support with aspell"),
							err->message);
			gtk_dialog_run(GTK_DIALOG(errdlg));
			gtk_widget_destroy(errdlg);
			g_error_free(err);
		}
		g_free(dictionary);
	}
#endif
	gtk_widget_set_name(GTK_WIDGET(input), "GGInput");
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(input), GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(input), 2);

	gtk_widget_ref(input);
	g_object_set_data_full(G_OBJECT(session->chat), "input", input, (GDestroyNotify) gtk_widget_unref);

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
	gtk_paned_pack1(GTK_PANED(paned), sw, TRUE, FALSE);

	/*
	 * SW2 
	 */
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_container_add(GTK_CONTAINER(sw), input);
	gtk_paned_pack2(GTK_PANED(paned), sw, FALSE, FALSE);

	/*
	 * attach paned to vbox_in_out 
	 */
	gtk_box_pack_start(GTK_BOX(vbox_in_out), paned, TRUE, TRUE, 0);

	/*
	 * buttons 
	 */


	button_send = gtk_button_new();
	gtk_box_pack_start_defaults(GTK_BOX(bs_hbox), bs_label);
	gtk_box_pack_end(GTK_BOX(bs_hbox), bs_image, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(button_send), bs_hbox);
	gtk_button_set_focus_on_click(GTK_BUTTON(button_send),FALSE);

	button_autosend = gtk_toggle_button_new();
	gtk_container_add(GTK_CONTAINER(button_autosend), create_image("arrow.png"));
	gtk_button_set_focus_on_click(GTK_BUTTON(button_autosend),FALSE);

	button_find = gtk_button_new();
	bas_image = gtk_image_new();
	gtk_image_set_from_stock(GTK_IMAGE(bas_image), "gtk-find", GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_container_add(GTK_CONTAINER(button_find), bas_image);
	gtk_button_set_focus_on_click(GTK_BUTTON(button_find),FALSE);

	button_close = gtk_button_new_from_stock("gtk-close");
	gtk_button_set_focus_on_click(GTK_BUTTON(button_close),FALSE);
	
	button_stick = gtk_toggle_button_new();
	bas_image = create_image("push-pin.png");
	gtk_container_add(GTK_CONTAINER(button_stick), bas_image);
	gtk_button_set_focus_on_click(GTK_BUTTON(button_stick),FALSE);

	button_clear = gtk_button_new();
	bas_image = gtk_image_new();
	gtk_image_set_from_stock(GTK_IMAGE(bas_image), "gtk-clear", GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_container_add(GTK_CONTAINER(button_clear), bas_image);
	gtk_button_set_focus_on_click(GTK_BUTTON(button_clear),FALSE);

	gtk_button_set_relief(GTK_BUTTON(button_find), GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(button_clear), GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(button_stick), GTK_RELIEF_NONE);

	gtk_box_pack_start(GTK_BOX(hbox_buttons), button_send, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_buttons), button_autosend, FALSE, FALSE, 2);
	gtk_box_pack_end(GTK_BOX(hbox_buttons), button_close, FALSE, FALSE, 2);
	gtk_box_pack_end(GTK_BOX(hbox_buttons), button_clear, FALSE, FALSE, 2);
	gtk_box_pack_end(GTK_BOX(hbox_buttons), button_stick, FALSE, FALSE, 2);
	gtk_box_pack_end(GTK_BOX(hbox_buttons), button_find, FALSE, FALSE, 0);
	
	gtk_tooltips_set_tip(tips, button_send, _("Send a message"), "");
	gtk_tooltips_set_tip(tips, button_autosend, _("Send message with enter"), "");
	gtk_tooltips_set_tip(tips, button_close, _("Close a window"), "");
	gtk_tooltips_set_tip(tips, button_clear, _("Clear the buffer"), "");
	gtk_tooltips_set_tip(tips, button_stick, _("Stick to all virtual desktops"), "");
	gtk_tooltips_set_tip(tips, button_find, _("Find a contact"), "");


	g_object_set_data(G_OBJECT(session->chat), "autosend_button", button_autosend);
	g_signal_connect(G_OBJECT(input), "key-press-event", G_CALLBACK(on_input_press_event), session);

	if (ggadu_config_var_get(gui_handler, "send_on_enter"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_autosend), TRUE);

	if (emoticons)
	{
		button_emoticons = gtk_button_new();
		gtk_button_set_relief(GTK_BUTTON(button_emoticons), GTK_RELIEF_NONE);
		gtk_button_set_focus_on_click (GTK_BUTTON(button_emoticons),FALSE);

		gtk_container_add(GTK_CONTAINER(button_emoticons), create_image("emoticon.gif"));

		gtk_box_pack_start(GTK_BOX(hbox_buttons), button_emoticons, FALSE, FALSE, 0);
		gtk_tooltips_set_tip(tips, button_emoticons, _("Emoticons"), "");
		g_signal_connect(button_emoticons, "clicked", G_CALLBACK(on_emoticons_clicked), session);
	}

	if (visible)
	{
		if (chat_window != NULL)
			gtk_widget_show_all(chat_window);

		if (ggadu_config_var_get(gui_handler, "chat_window_auto_raise") == FALSE)
		{
			gtk_widget_grab_focus(GTK_WIDGET(input));
		}

	}
	else
	{
		invisible_chats = g_slist_append(invisible_chats, session->chat);
	}


	/* chat_paned_size */
	chat_notebook_paned_size = g_object_get_data(G_OBJECT(chat_window), "chat_notebook");
	percent = (gint) ggadu_config_var_get(gui_handler, "chat_paned_size");

	if ((chat_type == CHAT_TYPE_TABBED) && (gtk_notebook_get_show_tabs(GTK_NOTEBOOK(chat_notebook_paned_size))))
	{
		GtkRequisition rbnl;
		GtkWidget *label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(chat_notebook_paned_size),
							      GTK_WIDGET(session->chat));
		gtk_widget_size_request(GTK_WIDGET(label), &rbnl);
		tab_minus = -rbnl.height;
	}

	gtk_window_get_size(GTK_WINDOW(chat_window), &r.width, &r.height);
	gtk_widget_size_request(GTK_WIDGET(hbox_buttons), &rb);

	position = ((((float) r.height - (float) rb.height) / 100) * (float) percent) + tab_minus;

	g_object_set_data(G_OBJECT(session->chat), "paned", paned);
	g_object_set_data(G_OBJECT(session->chat), "hbox_buttons", hbox_buttons);

	gtk_paned_set_position(GTK_PANED(paned), (gint) position);
	/* end chat_paned_size */
	

	g_signal_connect(chat_window, "configure-event", G_CALLBACK(window_resize_signal), session);
	g_signal_connect(chat_window, "event", G_CALLBACK(window_event_signal), session);
	g_signal_connect(button_autosend, "clicked", G_CALLBACK(on_autosend_clicked), session);
	g_signal_connect(button_send, "clicked", G_CALLBACK(on_send_clicked), session);
	g_signal_connect(button_stick, "toggled", G_CALLBACK(on_stick_clicked), session);
	g_signal_connect(button_find, "clicked", G_CALLBACK(on_chat_find_clicked), session);
	g_signal_connect(button_clear, "clicked", G_CALLBACK(on_clear_clicked),session);

	switch (chat_type)
	{
	case CHAT_TYPE_TABBED:
		g_signal_connect(button_close, "clicked", G_CALLBACK(on_destroy_chat), NULL);
		g_signal_connect(chat_window, "destroy", G_CALLBACK(on_destroy_chat_window), NULL);
		break;
	case CHAT_TYPE_CLASSIC:
		g_signal_connect_swapped(button_close, "clicked", G_CALLBACK(gtk_widget_hide), chat_window);
		g_signal_connect(chat_window, "destroy", G_CALLBACK(on_destroy_chat), session);

		break;
	}


	g_free(status_desc);
	g_free(wintitle);
	g_free(confer_title);

	return session->chat;
}

/* self mean that this is my message, not my opponent :) */
void gui_chat_append(GtkWidget * chat, gpointer msg, gboolean self, gboolean notice_message)
{
	gint chat_type = (gint) ggadu_config_var_get(gui_handler, "chat_type");
	GtkWidget *history = g_object_get_data(G_OBJECT(chat), "history");
	GtkTextBuffer *buf = NULL;
	GtkTextIter iter;
	gchar *header = NULL;
	gchar *text = NULL;
	gchar *tmp = NULL;
	gchar *notice_message_txt = NULL;
	GtkTextMark *mark_start;
	GtkTextMark *mark_append;
	GtkTextIter istart;
	GtkTextIter iend;
	GtkWidget *widget = NULL;
	GSList *emottmp = NULL;
	gui_chat_session *session = NULL;
	gboolean conference = FALSE;

	print_debug("gui_chat_append");

	g_return_if_fail(history != NULL);

	if (chat && self && !msg)
	{
		GtkWidget *input = g_object_get_data(G_OBJECT(chat), "input");
		GtkWidget *window = gtk_widget_get_ancestor(chat, GTK_TYPE_WINDOW);
		if (!GTK_WIDGET_VISIBLE(window))
		{
			gtk_widget_show(window);
		}
		gtk_widget_grab_focus(input);
	}

	if (!chat || !msg)
		return;
		
	if (chat && !self)
	{
	    gboolean is_active = FALSE;
	    GtkWidget *window = gtk_widget_get_ancestor(chat, GTK_TYPE_WINDOW);
	    g_object_get(G_OBJECT(window),"is-active",&is_active,NULL);

	    if (!is_active)
	    {
		if (!g_object_get_data(G_OBJECT(window),"new-message-mark"))
		{
		    GdkPixbuf *image = NULL;
		    image = create_pixbuf(GGADU_MSG_ICON_FILENAME);
		    gtk_window_set_icon(GTK_WINDOW(window), image);
		    gdk_pixbuf_unref(image);
		    g_object_set_data(G_OBJECT(window),"new-message-mark",(gpointer) TRUE);
		    print_debug("changing icon to msg");
		}
	    }
	}

	session = g_object_get_data(G_OBJECT(chat), "gui_session");
	conference = (g_slist_length(session->recipients) > 1) ? TRUE : FALSE;

	if (self == TRUE)
	{
		if (conference)
			header = g_strdup_printf("%s :: %s :: ", get_timestamp(0),
						 ggadu_config_var_get(gui_handler,
								      "use_username") ? g_get_user_name() : _("Me"));
		else
			header = g_strdup_printf("%s :: %s :: ",
						 ggadu_config_var_get(gui_handler,
								      "use_username") ? g_get_user_name() : _("Me"),
						 get_timestamp(0));


		text = g_strdup(msg);
	}
	else
	{
		gchar *plugin_name_ = NULL;
		GGaduContact *k = NULL;
		GGaduMsg *gmsg = (GGaduMsg *) msg;

		if (!gmsg || !gmsg->message)
			return;


		plugin_name_ = g_object_get_data(G_OBJECT(chat), "plugin_name");
		k = gui_find_user(gmsg->id, gui_find_protocol(plugin_name_, protocols));

		if (chat_type == CHAT_TYPE_TABBED)
		{
			GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window),
								     "chat_notebook");
			gint curr_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
			GtkWidget *curr_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook),
										curr_page);

			if ((gtk_notebook_get_n_pages(GTK_NOTEBOOK(chat_notebook)) > 1) && (curr_page_widget != chat))
			{
				gchar *markup = g_strdup_printf("<span foreground=\"blue\">%s</span>",
								(gchar *) g_object_get_data(G_OBJECT(chat),
											    "tab_label_txt_char"));
				gtk_label_set_markup(GTK_LABEL(g_object_get_data(G_OBJECT(chat), "tab_label_txt")),
						     markup);
				g_free(markup);
			}
		}

		if (conference)
			header = g_strdup_printf("%s :: %s :: ", get_timestamp(0), (k) ? k->nick : gmsg->id);
		else
		{
			gchar *timestamp = g_strdup(get_timestamp(0));
			header = g_strdup_printf("%s :: %s (%s) :: ", (k) ? k->nick : gmsg->id, timestamp,
						 get_timestamp(gmsg->time));
			g_free(timestamp);
		}
		text = g_strdup(gmsg->message);
	}

	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(history));
	gtk_text_buffer_get_end_iter(buf, &iter);
	mark_append = gtk_text_buffer_create_mark(buf, NULL, &iter, TRUE);

	if (!notice_message)
	{
		tmp = g_strconcat(header, (conference) ? "" : "\n", NULL);
		gtk_text_buffer_insert_with_tags_by_name(buf, &iter, tmp, -1, self ? "outgoing_header" : "incoming_header",
						 NULL);
	}
	 else
	{
		notice_message_txt = g_strdup_printf("%s *** ", get_timestamp(0));
		gtk_text_buffer_insert_with_tags_by_name(buf, &iter, notice_message_txt, -1, self ? "outgoing_header" : "incoming_header",
						 NULL);
		g_free(notice_message_txt);
	}
	g_free(tmp);

	tmp = g_strconcat(text, (notice_message) ? "" : "\n", NULL);
	gtk_text_buffer_insert_with_tags_by_name(buf, &iter, tmp, -1, self ? "outgoing_text" : "incoming_text", NULL);
	g_free(tmp);
	
	if (notice_message)
	{
		gtk_text_buffer_get_end_iter(buf, &iter);
		notice_message_txt = g_strdup_printf(" ***\n");
		gtk_text_buffer_insert_with_tags_by_name(buf, &iter, notice_message_txt, -1, self ? "outgoing_header" : "incoming_header",
						 NULL);
		g_free(notice_message_txt);
	}
	
	mark_start = gtk_text_buffer_get_insert(buf);

	/*
	 * scroll only if we at the bottom of the history :) I'm genius 
	 */
	if (GTK_TEXT_VIEW(history)->vadjustment)
	{
		GtkAdjustment *adj = GTK_TEXT_VIEW(history)->vadjustment;

		if ((adj->value + adj->page_size) == adj->upper)
		{

			gtk_text_buffer_get_end_iter(buf, &iter);
			gtk_text_buffer_place_cursor(buf, &iter);
			gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(history), gtk_text_buffer_get_insert(buf), 0.0, TRUE,
						     0.5, 0.5);
		}

	}

	/*
	 * emoticons engine initial, it works !, but is case sensitive 
	 */
	gtk_text_buffer_get_iter_at_mark(buf, &iter, mark_append);
	gtk_text_buffer_get_iter_at_mark(buf, &istart, mark_start);
	gtk_text_buffer_get_start_iter(buf, &iend);
	gtk_text_iter_backward_char(&istart);


	emottmp = emoticons;
	while (emottmp)
	{
		gui_emoticon *gemo = (gui_emoticon *) emottmp->data;

		while (gtk_text_iter_backward_search
		       (&istart, gemo->emoticon, GTK_TEXT_SEARCH_VISIBLE_ONLY, &istart, &iend, &iter))
		{
			GtkTextChildAnchor *anchor = NULL;

			widget = create_image(gemo->file);
			if (widget)
			{
				gtk_text_buffer_delete(buf, &istart, &iend);
				anchor = gtk_text_buffer_create_child_anchor(buf, &istart);

				gtk_text_view_add_child_at_anchor(GTK_TEXT_VIEW(history), widget, anchor);
				gtk_text_buffer_get_end_iter(buf, &istart);
				gtk_text_iter_backward_char(&istart);

				gtk_widget_show(widget);
			}
			gtk_text_buffer_get_iter_at_mark(buf, &iter, mark_append);
		}
		emottmp = emottmp->next;
	}

	//g_object_unref(mark_append);
//      g_object_unref(mark_start);

	if (((gint) ggadu_config_var_get(gui_handler, "chat_window_auto_raise") == TRUE) && (!self) &&
	    (GTK_WIDGET_VISIBLE(chat)))
	{
		/* will not bring window to actually active screen */
		gtk_window_deiconify(GTK_WINDOW(g_object_get_data(G_OBJECT(chat), "top_window")));
	}
	else if (chat_type == CHAT_TYPE_TABBED)
	{
		GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window),
							     "chat_notebook");
		guint nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
		GtkWidget *curr_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook), nr);

		gtk_widget_grab_focus(g_object_get_data(G_OBJECT(curr_page_widget), "input"));
		print_debug("current_page = %d", nr);
	}

	g_free(header);
	g_free(text);
}
