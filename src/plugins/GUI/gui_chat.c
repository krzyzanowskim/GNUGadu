/* $Id: gui_chat.c,v 1.19 2003/04/16 14:40:56 shaster Exp $ */

#include <gtk/gtk.h>
#include <string.h>
#include "support.h"
#include "signals.h"
#include "plugins.h"
#include "dialog.h"
#include "GUI_plugin.h"
#include "gui_main.h"
#include "gui_support.h"
#include "gui_chat.h"

extern GGaduConfig *config;
extern GSList *protocols;
extern GSList *emoticons;
extern GSList *invisible_chats;
extern GGaduPlugin *gui_handler;

GtkWidget *chat_window = NULL;

void gui_chat_notebook_switch(GtkWidget *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data)
{
	GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window),"chat_notebook");
	GtkWidget *chat = NULL;
	GtkWidget *lb = NULL;
	gchar *txt = NULL;
	gchar *txt2 = NULL;

	if (chat_notebook)
		chat = (GtkWidget *)gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook),page_num);
	
	if (chat)
		lb = g_object_get_data(G_OBJECT(chat),"tab_label_txt");
	
	if (lb) {
		txt = (gchar *)g_object_get_data(G_OBJECT(chat),"tab_label_txt_char");
		txt2 = (gchar *)g_object_get_data(G_OBJECT(chat),"tab_window_title_char");
	}
	
	if ((txt != NULL) && (chat_window != NULL) && (lb != NULL)) {
		gchar *markup = g_strdup_printf("<span foreground=\"black\">%s</span>",txt);
		gtk_window_set_title(GTK_WINDOW(chat_window),txt2);
		gtk_label_set_markup(GTK_LABEL(lb),markup);
		g_free(markup);
	}

	print_debug("gui_chat_notebook_switch\n");
}

void on_destroy_chat_window(GtkWidget *chat, gpointer user_data)
{
	print_debug("on_destroy_chat_window\n");
	gui_remove_all_chat_sessions(protocols);
	chat_window = NULL;
}

void on_destroy_chat(GtkWidget *button, gpointer user_data)
{
	gint chat_type = (gint)config_var_get(gui_handler,"chat_type");
	gui_chat_session *session = NULL;
	gui_protocol *gp = NULL;
	gchar  *plugin_name = NULL;

	print_debug("on_destroy_chat\n");
	
	switch (chat_type) {
		case CHAT_TYPE_TABBED : 
		{
			GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window),"chat_notebook");
			guint			nr;
			GtkWidget *chat = NULL;
			gulong id;
		
			if (!user_data) {
				nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
			} else {
				gui_chat_session *s = (gui_chat_session *)user_data;
				nr = gtk_notebook_page_num(GTK_NOTEBOOK(chat_notebook),s->chat);
			}

			chat = gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook),nr);
		
			plugin_name = g_object_get_data(G_OBJECT(chat), "plugin_name");
			session = (gui_chat_session *)g_object_get_data(G_OBJECT(chat), "gui_session");
			gp = gui_find_protocol(plugin_name,protocols);

			id = (gulong)g_object_get_data(G_OBJECT(chat_window),"switch_page_id");
		
			g_signal_handler_block(chat_notebook,id);
			gtk_notebook_remove_page(GTK_NOTEBOOK(chat_notebook),nr);
			g_signal_handler_unblock(chat_notebook,id);

			gtk_notebook_set_show_tabs(GTK_NOTEBOOK(chat_notebook),(gtk_notebook_get_n_pages (GTK_NOTEBOOK(chat_notebook)) <= 1) ? FALSE : TRUE);
	
			if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(chat_notebook)) <= 0) {
				gui_remove_all_chat_sessions(protocols);
				gtk_widget_destroy(chat_window);
				chat_window = NULL;
			} else {
				gp->chat_sessions = g_slist_remove(gp->chat_sessions,session);
				gui_chat_notebook_switch(chat_notebook, NULL, gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook)), NULL);
				g_free(session);
			}
		}
		break;
		case CHAT_TYPE_CLASSIC : 
		{
			session = (gui_chat_session *)user_data;
			if (!session) return;
			plugin_name = g_object_get_data(G_OBJECT(session->chat), "plugin_name");
			if (!plugin_name) return;
			gp = gui_find_protocol(plugin_name,protocols);
			
			chat_window = NULL;
			gtk_widget_destroy(button);
			session->chat = NULL;
			gp->chat_sessions = g_slist_remove(gp->chat_sessions,session);
			g_free(session);
		}
		break;
	}

	print_debug("main-gui : chat : zwalniam session\n");
}

gboolean on_press_event_switching_tabs(gpointer object, GdkEventKey *event, gpointer user_data)
{
	gint chat_type = (gint)config_var_get(gui_handler,"chat_type");

	if ((chat_type == CHAT_TYPE_TABBED) && (event->state & GDK_CONTROL_MASK)) {
		GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window),"chat_notebook");
		gint act_tab_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));

		if (event->keyval == 65289) {
			act_tab_page++;

			if (act_tab_page >= gtk_notebook_get_n_pages(GTK_NOTEBOOK(chat_notebook))) 
				act_tab_page = 0;

			gtk_notebook_set_current_page(GTK_NOTEBOOK(chat_notebook),act_tab_page);
			return TRUE;
		}
	}

	return FALSE;
}

gboolean on_input_press_event(gpointer object, GdkEventKey *event, gpointer user_data)
{
	gui_chat_session *s = user_data;

	if ((event->keyval == 65293) && (config_var_get(gui_handler, "send_on_enter"))) {
		print_debug("main-gui : chat : wcisnieto Enter \n");
		on_send_clicked(s->chat,user_data);
		return TRUE;
	}
	
 return (event->state & GDK_CONTROL_MASK) ? on_press_event_switching_tabs(object,event,user_data) : FALSE;
}

void on_stick_clicked(GtkWidget *button, gpointer user_data)
{
	gui_chat_session *s = user_data;
	gboolean q = (gboolean)g_object_get_data(G_OBJECT(s->chat),"stick");
	GtkWidget *win = g_object_get_data(G_OBJECT(s->chat),"top_window");

	if (q) {
		gtk_window_unstick(GTK_WINDOW(win));
		g_object_set_data(G_OBJECT(s->chat),"stick",(gpointer)FALSE);
	} else {
		gtk_window_stick(GTK_WINDOW(win));
		g_object_set_data(G_OBJECT(s->chat),"stick",(gpointer)TRUE);
	}
}

void on_autosend_clicked(GtkWidget *button, gpointer user_data)
{
	gint chat_type = (gint)config_var_get(gui_handler,"chat_type");

	config_var_set(gui_handler, "send_on_enter", (gpointer)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));
	/* change in every open session */
	if (chat_type == CHAT_TYPE_CLASSIC) {
    GSList *protocols_local = (GSList *)protocols;
    
    while (protocols_local) {
			gui_protocol *protocol = (gui_protocol *)protocols_local->data;
			GSList *sessions = (GSList *)protocol->chat_sessions;
	
			while (sessions) {
	    	gui_chat_session *s = (gui_chat_session *)sessions->data;
	    	GtkWidget *autosend_button = g_object_get_data(G_OBJECT(s->chat),"autosend_button");
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(autosend_button),gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));
	    	sessions = sessions->next;
			}
	
			protocols_local = protocols_local->next;
    }
	}
}

void on_send_clicked(GtkWidget *button, gpointer user_data)
{
	gint chat_type = (gint)config_var_get(gui_handler,"chat_type");
	gui_chat_session *session = NULL;
	GGaduMsg *msg = NULL;
	gchar *plugin_name = NULL;
	GtkWidget *input = NULL;
	GtkTextBuffer *buf = NULL;
	GtkTextIter start, end;
	gchar *tmpmsg = NULL;


	if (chat_type == CHAT_TYPE_TABBED) {
		GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window),"chat_notebook");	
		guint			nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
		GtkWidget *chat = gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook),nr);
	
		input = g_object_get_data(G_OBJECT(chat), "input");
		plugin_name = g_object_get_data(G_OBJECT(chat), "plugin_name");
		session = (gui_chat_session *)g_object_get_data(G_OBJECT(chat), "gui_session");
	
	} else if (chat_type == CHAT_TYPE_CLASSIC) {
	
		session = user_data;
		input = g_object_get_data(G_OBJECT(session->chat), "input");
		plugin_name = g_object_get_data(G_OBJECT(session->chat), "plugin_name");
	}

	g_return_if_fail(input != NULL);

	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(input));
	gtk_text_buffer_get_start_iter(buf, &start);
	gtk_text_buffer_get_end_iter(buf, &end);

	tmpmsg = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
	tmpmsg = g_strchug(g_strchomp(tmpmsg)); //obciete puste spacje z obu stron

	if (strlen(tmpmsg) > 0) { 
		msg = g_new0(GGaduMsg, 1);
		msg->id = session->id;
		msg->message = tmpmsg; 
  
		msg->class = (g_slist_length(session->recipients) > 1) ? GGADU_CLASS_CONFERENCE : GGADU_CLASS_CHAT;
		msg->recipients = session->recipients;

		gui_chat_append(session->chat, msg->message, TRUE);
		signal_emit("main-gui", "send message", msg, plugin_name);
	} else 
		g_free(tmpmsg);

	gtk_text_buffer_set_text(buf,"",-1);
	gtk_widget_grab_focus(GTK_WIDGET(input));	
}

void on_emoticon_press_event(GtkWidget *event_box, GdkEventButton *event, gpointer user_data)
{
	GtkWidget *input;
	GtkWidget *emoticons_window;
	gui_emoticon *gemo = user_data;
	GtkTextBuffer *buf;
	GtkTextIter start, end;
	gui_chat_session *session;

	/* ZONK */
	emoticons_window = lookup_widget(event_box, "emoticons_window");

	g_return_if_fail(emoticons_window != NULL);

	session = g_object_get_data(G_OBJECT(emoticons_window), "session");
	input = g_object_get_data(G_OBJECT(session->chat), "input");
	
	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(input));
	gtk_text_buffer_get_start_iter(buf, &start);
	gtk_text_buffer_get_end_iter(buf, &end);

	gtk_text_buffer_insert(buf, &end, gemo->emoticon, -1);

	gtk_widget_destroy(emoticons_window);
}

gboolean find_emoticon(gchar *name,GSList *here)
{
	GSList *tmp = here;

	while (tmp)
	{
		gui_emoticon *e = (gui_emoticon *)tmp->data;

		if (!g_strcasecmp(name,e->file))
			return TRUE;

	tmp = tmp->next;
	}

	return FALSE;
}

void on_emoticons_clicked(GtkWidget *button, gpointer user_data)
{
	gint chat_type = (gint)config_var_get(gui_handler,"chat_type");
	GtkWidget *emoticons_window = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *outervbox = NULL;
	GtkWidget *button_close = NULL;
	GtkWidget *scrolledwindow1;
        GtkWidget *viewport1;
	gui_chat_session *session = NULL;

	if (chat_type == CHAT_TYPE_TABBED) {
		GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window),"chat_notebook");	
		guint			nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
		GtkWidget *chat = gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook),nr);
	
		session = (gui_chat_session *)g_object_get_data(G_OBJECT(chat), "gui_session");
	
	} else if (chat_type == CHAT_TYPE_CLASSIC) {
		session = user_data;
	}

	emoticons_window = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_modal(GTK_WINDOW(emoticons_window), TRUE);
	gtk_window_set_position(GTK_WINDOW(emoticons_window), GTK_WIN_POS_MOUSE);
	
	gtk_widget_set_usize (emoticons_window, 510, 300);
	
	g_object_set_data(G_OBJECT(emoticons_window), "session", session);
	
	g_object_set_data(G_OBJECT(emoticons_window), "emoticons_window", emoticons_window);


/* dodajê po drodze viewport i scroll 
	Pawe³ Marchewka <pmgiant@student.uci.agh.edu.pl> 12.04.2003
*/
        outervbox = gtk_vbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(emoticons_window), outervbox);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	        gtk_widget_set_name (scrolledwindow1, "scrolledwindow1");
	        gtk_widget_ref (scrolledwindow1);
	        gtk_object_set_data_full (GTK_OBJECT (emoticons_window), "scrolledwindow1", scrolledwindow1,
			(GtkDestroyNotify) gtk_widget_unref);
	        gtk_box_pack_start(GTK_BOX(outervbox), scrolledwindow1, TRUE, TRUE, 0);
	
		gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow1), 5);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
		GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
												
	viewport1 = gtk_viewport_new (NULL, NULL);
		gtk_widget_set_name (viewport1, "viewport1");
		gtk_widget_ref (viewport1);
		gtk_object_set_data_full (GTK_OBJECT (emoticons_window), "viewport1", viewport1,
			(GtkDestroyNotify) gtk_widget_unref);
		gtk_container_add (GTK_CONTAINER (scrolledwindow1), viewport1);
	
	vbox = gtk_vbox_new(TRUE, 0);
		gtk_container_add(GTK_CONTAINER(viewport1), vbox);

	if (emoticons) {
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
			gui_emoticon *gemo = (gui_emoticon *)emottmp->data;

			if (!find_emoticon(gemo->file,emotlist))
				emotlist = g_slist_append(emotlist,gemo);
		
			emottmp = emottmp->next;
		}

	emottmp = emotlist;

	while (emottmp) 
	{
		GtkTooltips *tip;
		gui_emoticon *gemo = (gui_emoticon *)emottmp->data;
		widget = GTK_WIDGET(create_image(gemo->file));
		event_box = gtk_event_box_new();

		if (count >= MAX_EMOTICONS_IN_ROW) {
			hbox = gtk_hbox_new(TRUE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
			count = 0;
		}

		gtk_container_add (GTK_CONTAINER (event_box), widget);
		gtk_box_pack_start(GTK_BOX(hbox), event_box, FALSE, FALSE, 0);
		gtk_widget_set_usize (event_box, 60, 30);
		
		tip = gtk_tooltips_new();
		gtk_tooltips_set_tip(tip, event_box, gemo->emoticon, gemo->file);
		g_signal_connect(event_box, "button_press_event", G_CALLBACK(on_emoticon_press_event), gemo);
		emottmp = emottmp->next;
		count++;
	}

	g_slist_free(emotlist);
	}

	button_close = gtk_button_new_with_mnemonic(_("_Close"));

	gtk_box_pack_start(GTK_BOX(outervbox), button_close, FALSE, FALSE, 0);

	g_signal_connect_swapped(button_close, "clicked", G_CALLBACK(gtk_widget_destroy), emoticons_window);

	gtk_widget_show_all(emoticons_window);
}

void on_chat_find_clicked(GtkWidget *button, gpointer user_data)
{
	gint chat_type = (gint)config_var_get(gui_handler,"chat_type");
	GGaduDialog *d = ggadu_dialog_new();
	gui_chat_session *session = NULL;
	gchar  *plugin_name = NULL;

	if (chat_type == CHAT_TYPE_TABBED) {
		GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window),"chat_notebook");	
		guint			nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
		GtkWidget *chat = gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook),nr);
	
		plugin_name = g_object_get_data(G_OBJECT(chat), "plugin_name");
		session = (gui_chat_session *)g_object_get_data(G_OBJECT(chat), "gui_session");
	
	} else if (chat_type == CHAT_TYPE_CLASSIC) {
	
		session = user_data;
		plugin_name = g_object_get_data(G_OBJECT(session->chat), "plugin_name");
	}


	print_debug("SEARCH %s\n",session->id);

	ggadu_dialog_add_entry(&(d->optlist), GGADU_SEARCH_ID, NULL, VAR_STR, session->id, VAR_FLAG_NONE);

	signal_emit("main-gui", "search", d, plugin_name);
}

void gui_chat_update_tags()
{
	GSList *tmplist = protocols;
	GSList *tmpsessions;

	while (tmplist) 
	{
	gui_protocol *gp = tmplist->data;
	tmpsessions = gp->chat_sessions;

	while (tmpsessions) {
		gui_chat_session *session = tmpsessions->data; 
		GtkWidget *history = g_object_get_data(G_OBJECT(session->chat), "history");
		GtkTextBuffer *buf;
		GtkTextTagTable *tagtable;
		GtkTextTag *tag;
		gchar *tagstr;

		buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(history));
		tagtable = gtk_text_buffer_get_tag_table(buf);

		tag = gtk_text_tag_table_lookup(tagtable, "incoming_header");

		tagstr = config_var_get(gui_handler, "msg_header_color");
		g_object_set(G_OBJECT(tag), "foreground", (tagstr) ? tagstr : DEFAULT_TEXT_COLOR, NULL);

		tagstr = config_var_get(gui_handler, "msg_header_font");
		g_object_set(G_OBJECT(tag), "font", (tagstr) ? tagstr : DEFAULT_FONT, NULL);

		tag = gtk_text_tag_table_lookup(tagtable, "incoming_text");

		tagstr = config_var_get(gui_handler, "msg_body_color");
		g_object_set(G_OBJECT(tag), "foreground", (tagstr) ? tagstr : DEFAULT_TEXT_COLOR, NULL);

		tagstr = config_var_get(gui_handler, "msg_body_font");
		g_object_set(G_OBJECT(tag), "font", (tagstr) ? tagstr : DEFAULT_FONT, NULL);

		tag = gtk_text_tag_table_lookup(tagtable, "outgoing_header");

		tagstr = config_var_get(gui_handler, "msg_out_header_color");
		g_object_set(G_OBJECT(tag), "foreground", (tagstr) ? tagstr : DEFAULT_TEXT_COLOR, NULL);

		tagstr = config_var_get(gui_handler, "msg_out_header_font");
		g_object_set(G_OBJECT(tag), "font", (tagstr) ? tagstr : DEFAULT_FONT, NULL);

		tag = gtk_text_tag_table_lookup(tagtable, "outgoing_text");

		tagstr = config_var_get(gui_handler, "msg_out_body_color");
		g_object_set(G_OBJECT(tag), "foreground", (tagstr) ? tagstr : DEFAULT_TEXT_COLOR, NULL);

		tagstr = config_var_get(gui_handler, "msg_out_body_font");
		g_object_set(G_OBJECT(tag), "font", (tagstr) ? tagstr : DEFAULT_FONT, NULL);

		tmpsessions = tmpsessions->next;
	}
	tmplist = tmplist->next;
	}
}

GtkWidget *create_chat(gui_chat_session *session, gchar *plugin_name, gchar *id, gboolean visible)
{
	gint chat_type = (gint)config_var_get(gui_handler,"chat_type");
	gboolean conference = (g_slist_length(session->recipients) > 1) ? TRUE : FALSE;
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
	GtkTextBuffer *buf = NULL;
	gchar *title = NULL;
	gchar *wintitle = NULL;
	gchar *confer_title = NULL;
	gchar *colorstr, *fontstr;
	gchar *st = NULL;
	GGaduContact *k = gui_find_user(id,gui_find_protocol(plugin_name,protocols));	
	gui_protocol *gp = NULL;
	GGaduStatusPrototype *sp = NULL;

	/* confer_topic create */
	if (conference)	{
		gchar *prev = NULL;
		gchar *tmp = "";
		GSList *tmprl = session->recipients;
		
		while (tmprl) {
			GGaduContact *k1 = gui_find_user(tmprl->data,gui_find_protocol(plugin_name,protocols));
			tmp = (tmprl->next) ? g_strconcat(tmp,k1 ? k1->nick : tmprl->data,",",NULL) : g_strconcat(tmp,k1 ? k1->nick : tmprl->data,NULL);
			g_free(prev);
			prev = tmp;
			tmprl = tmprl->next;
		}
		confer_title = g_strdup(tmp);
		g_free(tmp);
	};

	vbox = gtk_vbox_new(FALSE,0); /* up - vbox_in_out, down - buttons */
	hbox_buttons = gtk_hbox_new(FALSE, 0); /* buttons hbox */
	vbox_in_out = gtk_vbox_new(FALSE, 0);		/* up - input, down - history */
	session->chat = vbox_in_out;
	g_object_set_data(G_OBJECT(session->chat),"gui_session",session);

	/* find status description */
	if (k) {
		gp = gui_find_protocol(plugin_name, protocols);
		sp = gui_find_status_prototype(gp->p,k->status);
		st = g_strdup_printf("- (%s)", (sp ? sp->description : ""));
		if (k->status_descr)
		    st = g_strdup_printf("- %s (%s)", (sp ? sp->description : ""), k->status_descr);
		else
		    st = g_strdup_printf("- %s", (sp ? sp->description : ""));
	}

	/* create approp. window style - tabbed or stand alone */
	switch (chat_type) 
	{
		case CHAT_TYPE_CLASSIC: {
			if (k)
				wintitle = (conference) ? g_strdup(confer_title) : g_strdup_printf(_("Talking to %s (%s) %s"),k->nick,id, (sp ? st : ""));
			else 
				wintitle = (conference) ? g_strdup(confer_title) : g_strdup_printf(_("Talking to %s"),id);

			chat_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
			gtk_window_set_title(GTK_WINDOW(chat_window), g_strdup(wintitle));
		
			gtk_box_pack_start(GTK_BOX(vbox), vbox_in_out, TRUE, TRUE, 0);
			gtk_box_pack_end(GTK_BOX(vbox), hbox_buttons, FALSE, FALSE, 0);
			gtk_container_add(GTK_CONTAINER(chat_window), vbox);
		}
			break;

		case CHAT_TYPE_TABBED: {
		    GtkWidget *chat_notebook = NULL;
		    GtkWidget *tab_label_hbox = NULL;
		    GtkWidget *tab_label_txt = NULL;
		    GtkWidget *tab_label_close = NULL;
		    GtkWidget *tab_label_close_image = gtk_image_new_from_pixbuf(NULL); /* empty */
	    
			if (chat_window == NULL) {
				gulong id_tmp;
		
				chat_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
				gtk_window_set_title(GTK_WINDOW(chat_window), _("Chat Window"));

				chat_notebook = gtk_notebook_new();
				gtk_notebook_set_scrollable(GTK_NOTEBOOK(chat_notebook),TRUE);

				gtk_box_pack_start(GTK_BOX(vbox), chat_notebook, TRUE, TRUE, 0);
				gtk_box_pack_end(GTK_BOX(vbox), hbox_buttons, FALSE, FALSE, 0);
			
				gtk_container_add(GTK_CONTAINER(chat_window), vbox);
			
				/* ZONK */		
				id_tmp = g_signal_connect(G_OBJECT(chat_notebook),"switch-page",G_CALLBACK(gui_chat_notebook_switch),NULL);
				g_object_set_data(G_OBJECT(chat_window),"switch_page_id",(gpointer)id_tmp);
		
				gtk_notebook_popup_enable(GTK_NOTEBOOK(chat_notebook));
				g_object_set_data(G_OBJECT(chat_window), "chat_notebook", chat_notebook);
			
//				g_signal_connect(G_OBJECT(chat_notebook), "key-press-event", G_CALLBACK(on_press_event_switching_tabs), NULL);
//				g_signal_connect(G_OBJECT(chat_window), "key-press-event", G_CALLBACK(on_press_event_switching_tabs), NULL);

			} else {
				chat_notebook = g_object_get_data(G_OBJECT(chat_window),"chat_notebook");
			}

			/* for labels */
			title = (conference) ? g_strdup(confer_title) : g_strdup_printf("%s",k ? k->nick : id);
			/* for window title */
			wintitle = (conference) ? g_strdup(confer_title) : g_strdup_printf("%s %s",k ? k->nick : id, sp ? st : "");

			tab_label_hbox  = gtk_hbox_new(FALSE,FALSE);
			tab_label_txt   = gtk_label_new(title);
			tab_label_close = gtk_button_new();
	    
			gtk_button_set_relief(GTK_BUTTON(tab_label_close),GTK_RELIEF_NONE);
			g_signal_connect(tab_label_close,  "clicked",  G_CALLBACK(on_destroy_chat), session);

			gtk_widget_set_usize(GTK_WIDGET(tab_label_close),12,1);
	    
			gtk_image_set_from_stock(GTK_IMAGE(tab_label_close_image),"gtk-close",GTK_ICON_SIZE_MENU);
			gtk_container_add(GTK_CONTAINER(tab_label_close),GTK_WIDGET(tab_label_close_image));
	    
			gtk_box_pack_start_defaults(GTK_BOX(tab_label_hbox),tab_label_txt);
			gtk_box_pack_start_defaults(GTK_BOX(tab_label_hbox),tab_label_close);
	    
			g_object_set_data(G_OBJECT(session->chat),"tab_label_txt",tab_label_txt);
			g_object_set_data_full(G_OBJECT(session->chat),"tab_label_txt_char",g_strdup(title),g_free);
			g_object_set_data_full(G_OBJECT(session->chat),"tab_window_title_char",g_strdup(wintitle),g_free);

			gtk_notebook_set_menu_label_text(GTK_NOTEBOOK(chat_notebook),session->chat,title);
	
			/* append page */
			gtk_notebook_append_page(GTK_NOTEBOOK(chat_notebook),vbox_in_out,tab_label_hbox);
			gtk_notebook_set_menu_label_text(GTK_NOTEBOOK(chat_notebook),vbox_in_out,title);

			gtk_notebook_set_show_tabs(GTK_NOTEBOOK(chat_notebook),(gtk_notebook_get_n_pages (GTK_NOTEBOOK(chat_notebook)) <= 1) ? FALSE : TRUE);
	    
			gtk_widget_show_all(tab_label_hbox);
		}
		break;
	}


	gtk_window_set_default_size(GTK_WINDOW(chat_window), 400, 300);
	gtk_window_set_modal(GTK_WINDOW(chat_window), FALSE);
	gtk_widget_set_name(GTK_WIDGET(chat_window),"GGChat");

	g_object_set_data(G_OBJECT(vbox_in_out), "plugin_name", g_strdup(plugin_name));
	g_object_set_data(G_OBJECT(session->chat), "top_window", chat_window);
    
	/* history */
	history = gtk_text_view_new();
	gtk_widget_set_name(GTK_WIDGET(history),"GGHistory");
    
	gtk_text_view_set_editable(GTK_TEXT_VIEW(history), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(history),GTK_WRAP_CHAR);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(history), FALSE);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(history),2);

	gtk_widget_ref(history);
	g_object_set_data_full(G_OBJECT(session->chat), "history", history, 
    					(GDestroyNotify) gtk_widget_unref);

	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(history));

	colorstr = config_var_get(gui_handler, "msg_header_color");
	fontstr = config_var_get(gui_handler, "msg_header_font");

	gtk_text_buffer_create_tag (buf, "incoming_header",
				    "foreground",
				    (colorstr && (strlen(colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR,
				    "font", 
				    (fontstr) ? fontstr : DEFAULT_FONT,
				    NULL);

	colorstr = config_var_get(gui_handler, "msg_body_color");
	fontstr = config_var_get(gui_handler, "msg_body_font");
				    
	gtk_text_buffer_create_tag (buf, "incoming_text",
				    "foreground", 
				    (colorstr && (strlen(colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR,
				    "font", 
				    (fontstr) ? fontstr : DEFAULT_FONT,
				    NULL);

	colorstr = config_var_get(gui_handler, "msg_out_header_color");
	fontstr = config_var_get(gui_handler, "msg_out_header_font");

	gtk_text_buffer_create_tag (buf, "outgoing_header",
				    "foreground",
				    (colorstr && (strlen(colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR,
				    "font", 
				    (fontstr) ? fontstr : DEFAULT_FONT,
				    NULL);

	colorstr = config_var_get(gui_handler, "msg_out_body_color");
	fontstr = config_var_get(gui_handler, "msg_out_body_font");

	gtk_text_buffer_create_tag (buf, "outgoing_text",
				    "foreground", 
				    (colorstr && (strlen(colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR,
				    "font", 
				    (fontstr) ? fontstr : DEFAULT_FONT,
					NULL);

	/* input */
	input = gtk_text_view_new();
	gtk_widget_set_name(GTK_WIDGET(input),"GGInput");
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(input),GTK_WRAP_WORD);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(input),2);

	gtk_widget_ref(input);
	g_object_set_data_full(G_OBJECT(session->chat), "input", input, (GDestroyNotify) gtk_widget_unref);

	/* paned */
	paned = gtk_vpaned_new();

	/* SW1 */
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), 
			GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	gtk_container_add(GTK_CONTAINER(sw), history);
	gtk_paned_add1(GTK_PANED(paned), sw);
 
 	/* SW2 */
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), 
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_container_add(GTK_CONTAINER(sw), input);   
	gtk_paned_add2(GTK_PANED(paned), sw);
    
	gtk_paned_set_position(GTK_PANED(paned),150);
    
	/* attach paned to vbox_in_out */
	gtk_box_pack_start(GTK_BOX(vbox_in_out), paned, TRUE, TRUE, 0);
	
	/* buttons */
	button_send = gtk_button_new_with_mnemonic(_("_Send"));
	button_autosend = gtk_toggle_button_new();
	button_find = gtk_button_new_from_stock("gtk-find");
	button_close = gtk_button_new_from_stock("gtk-close");
	button_stick = gtk_toggle_button_new_with_mnemonic(_("S_tick"));

	gtk_button_set_relief(GTK_BUTTON(button_send),GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(button_autosend),GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(button_find),GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(button_close),GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(button_stick),GTK_RELIEF_NONE);

	gtk_box_pack_start(GTK_BOX(hbox_buttons), button_send, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_buttons), button_autosend, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_buttons), button_find, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox_buttons), button_close, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox_buttons), button_stick, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(button_autosend), create_image("arrow.png"));
	
	g_object_set_data(G_OBJECT(session->chat),"autosend_button",button_autosend);
	g_signal_connect(G_OBJECT(input), "key-press-event", G_CALLBACK(on_input_press_event), session);

	if (config_var_get(gui_handler, "send_on_enter"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_autosend),TRUE);


	if (emoticons) {
		button_emoticons = gtk_button_new();
		gtk_button_set_relief(GTK_BUTTON(button_emoticons),GTK_RELIEF_NONE);

		gtk_container_add(GTK_CONTAINER(button_emoticons), create_image("emoticon.gif"));
	
		gtk_box_pack_start(GTK_BOX(hbox_buttons), button_emoticons, FALSE, FALSE, 0);
		g_signal_connect(button_emoticons, "clicked", G_CALLBACK(on_emoticons_clicked), session);
	}
    
	if (visible) {
		gtk_widget_show_all (vbox);
	
		if (chat_window != NULL) 
			gtk_widget_show_all (chat_window);
	
	} else {
		invisible_chats = g_slist_append(invisible_chats,chat_window);
	}

//	g_signal_connect(button_close,  "clicked",  G_CALLBACK(on_destroy_chat), session);
	g_signal_connect(button_autosend, "clicked", G_CALLBACK(on_autosend_clicked), session);
	g_signal_connect(button_send, "clicked", G_CALLBACK(on_send_clicked), session);
	g_signal_connect(button_stick, "toggled", G_CALLBACK(on_stick_clicked), session);
	g_signal_connect(button_find, "clicked", G_CALLBACK(on_chat_find_clicked), session);

	switch (chat_type) {
		case CHAT_TYPE_TABBED  : 
				g_signal_connect(button_close,  "clicked",  G_CALLBACK(on_destroy_chat), NULL);
				g_signal_connect(chat_window,  "destroy",  G_CALLBACK(on_destroy_chat_window), NULL);
		break;
		case CHAT_TYPE_CLASSIC : 
				g_signal_connect_swapped(button_close,  "clicked",  G_CALLBACK(gtk_widget_destroy), chat_window);
				g_signal_connect(chat_window,  "destroy",  G_CALLBACK(on_destroy_chat), session);
		break;
	}

	gtk_widget_grab_focus(GTK_WIDGET(input));
    
	g_free(st);
	g_free(title);
	g_free(wintitle);
	g_free(confer_title);

	return vbox_in_out;
}

void gui_chat_append(GtkWidget *chat, gpointer msg, gboolean self)
{
	GtkWidget *history = g_object_get_data(G_OBJECT(chat), "history");
	GtkTextBuffer *buf = NULL;
	GtkTextIter iter;
	gchar *header = NULL;
	gchar *text = NULL;
	gchar *timestamp = NULL;
	gchar *tmp = NULL;
	GGaduMsg *gmsg = NULL;
	GtkTextMark *mark_start;
	gint chat_type = (gint)config_var_get(gui_handler,"chat_type");
	GtkTextIter istart;
	GtkTextIter iend;
	GtkWidget   *widget = NULL;
	GSList *emottmp = emoticons;
	
	g_return_if_fail(history != NULL);
	
	if (!chat) {
		print_debug("pusty CHAT\n");
		return;
	}
	
	if (!msg) {
		print_debug("pusty MSG\n");
		return;
	}

	if (self == FALSE) {
		gmsg = (GGaduMsg *)msg;
		if (!gmsg->message) return;
	}

	timestamp = get_timestamp(0);

	if (self == TRUE) {
		header = g_strdup_printf(_("Me :: %s ::\n"), timestamp);
		text = g_strdup(msg);
	} else {
		gchar	 *timestamp_send = get_timestamp(gmsg->time);
		gchar	 *plugin_name_   = g_object_get_data(G_OBJECT(chat), "plugin_name");
		GGaduContact *k		 = gui_find_user(gmsg->id,gui_find_protocol(plugin_name_,protocols));
	
		if (chat_type == CHAT_TYPE_TABBED) {
			GtkWidget	*chat_notebook	= g_object_get_data(G_OBJECT(chat_window),"chat_notebook");
			gint	curr_page	= gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
			GtkWidget	*curr_page_widget	= gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook),curr_page);
		
			if ((gtk_notebook_get_n_pages(GTK_NOTEBOOK(chat_notebook)) > 1) && (curr_page_widget != chat)) {
				gchar *markup = g_strdup_printf("<span foreground=\"blue\">%s</span>",(gchar *)g_object_get_data(G_OBJECT(chat),"tab_label_txt_char"));
				gtk_label_set_markup(GTK_LABEL(g_object_get_data(G_OBJECT(chat),"tab_label_txt")),markup);
				g_free(markup);
			}
		}

		header = g_strdup_printf("%s :: %s (%s) ::\n", (k) ? k->nick : gmsg->id, timestamp, timestamp_send);
		text = g_strdup(gmsg->message);
	    
		g_free(timestamp_send);
	}
	
	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(history));
	    
	gtk_text_buffer_get_end_iter(buf, &iter);

	gtk_text_buffer_insert_with_tags_by_name(buf, &iter, header, -1, self ? "outgoing_header" : "incoming_header", NULL);

	tmp = g_strconcat(text,"\n",NULL);
	
	if ((mark_start = gtk_text_buffer_get_mark(buf,"gg_new_text_start")) == NULL) 
		mark_start = gtk_text_buffer_create_mark(buf,"gg_new_text_start",&iter,FALSE);
	else
		gtk_text_buffer_move_mark(buf,mark_start,&iter);

	gtk_text_buffer_insert_with_tags_by_name(buf, &iter, tmp, -1, self ? "outgoing_text" : "incoming_text",NULL);
	
	g_free(tmp);

	/* scroll only if we at the bottom of the history :) I'm genius */
	if (GTK_TEXT_VIEW(history)->vadjustment) {
	    GtkAdjustment *adj = GTK_TEXT_VIEW(history)->vadjustment;

		 if ((adj->value + adj->page_size) == adj->upper) {

			gtk_text_buffer_get_end_iter(buf, &iter);
			gtk_text_buffer_place_cursor(buf,&iter);
			gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(history), 
					    gtk_text_buffer_get_insert(buf)
					    , 0.0, TRUE, 0.5, 0.5);
		 }

	}

	/* emoticons engine initial, it works !, but is case sensitive */
	gtk_text_buffer_get_iter_at_mark(buf,&istart,mark_start);
	gtk_text_buffer_get_start_iter(buf, &iend);
	gtk_text_iter_backward_char(&istart);
	    
	while (emottmp) 
	{
		gui_emoticon *gemo = (gui_emoticon *)emottmp->data;
		
		while (gtk_text_iter_backward_search(&istart, gemo->emoticon, GTK_TEXT_SEARCH_VISIBLE_ONLY, &istart, &iend, NULL))
		{
			GtkTextChildAnchor *anchor;
	    
			gtk_text_buffer_delete(buf, &istart, &iend);
			widget = GTK_WIDGET(create_image(gemo->file));
			anchor = gtk_text_buffer_create_child_anchor(buf,&istart);
	    
			gtk_text_view_add_child_at_anchor( GTK_TEXT_VIEW(history), widget, anchor);
			gtk_text_buffer_get_end_iter(buf, &istart);
			gtk_text_iter_backward_char(&istart);

			gtk_widget_show_all(widget);
		}

	emottmp = emottmp->next;
	}

	/* Is it should be called somewhere or not ? this is a question, rather no, but....*/
	/* gtk_text_buffer_delete_mark_by_name(buf,"gg_end"); */

	if (((gint)config_var_get(gui_handler,"chat_window_auto_raise") == TRUE) && (!self) && (GTK_WIDGET_VISIBLE(chat)))
		gtk_window_present(GTK_WINDOW(g_object_get_data(G_OBJECT(chat),"top_window")));
	
	g_free(timestamp);
	g_free(header);
	g_free(text);
}
