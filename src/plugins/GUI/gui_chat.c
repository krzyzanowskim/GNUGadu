/* $Id: gui_chat.c,v 1.2 2003/03/23 11:18:49 krzyzak Exp $ */

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
	GtkWidget *p = NULL;
	GtkWidget *lb = NULL;
	gchar *txt = NULL;

	if (chat_notebook)
		p = (GtkWidget *)gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook),page_num);
	
	if (p)
		lb = g_object_get_data(G_OBJECT(p),"tab_label_txt");
	
	if (lb)
		txt = (gchar *)g_object_get_data(G_OBJECT(p),"tab_label_txt_char");

	print_debug("gui_chat_notebook_switch\n");

	if ((txt != NULL) && (chat_window != NULL) && (lb != NULL)) {
		gchar *markup = g_strdup_printf("<span foreground=\"black\">%s</span>",txt);
		gtk_window_set_title(GTK_WINDOW(chat_window),txt);
		gtk_label_set_markup(GTK_LABEL(lb),markup);
		g_free(markup);
	}
}

void on_destroy_chat_window(GtkWidget *chat, gpointer user_data)
{
	gui_remove_all_chat_sessions(protocols);
	gtk_widget_destroy(chat);
	chat_window = NULL;
}

void on_destroy_chat(GtkWidget *chat, gpointer user_data)
{
	gint chat_type = (gint)config_var_get(gui_handler,"chat_type");
	gui_chat_session *session = user_data;
	gchar  *plugin_name = g_object_get_data(G_OBJECT(session->chat), "plugin_name");
	gui_protocol *gp = gui_find_protocol(plugin_name,protocols);

	print_debug("on_destroy_chat\n");

	if (chat_type == CHAT_TYPE_TABBED) {
		GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(chat_window),"chat_notebook");
		gint nr = gtk_notebook_page_num(GTK_NOTEBOOK(chat_notebook),session->chat);

		gulong id = (gulong)g_object_get_data(G_OBJECT(chat_window),"switch_page_id");
		g_signal_handler_block(chat_notebook,id);

		gtk_notebook_remove_page(GTK_NOTEBOOK(chat_notebook),nr);

		g_signal_handler_unblock(chat_notebook,id);

		if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(chat_notebook)) <= 0) {
			gui_remove_all_chat_sessions(protocols);
			gtk_widget_destroy(chat_window);
			chat_window = NULL;
		} else {
			gp->chat_sessions = g_slist_remove(gp->chat_sessions,session);
			g_free(session);
		}

	} else if (chat_type == CHAT_TYPE_CLASSIC) {
		gtk_widget_destroy(GTK_WIDGET(chat));
		chat_window = NULL;
		session->chat = NULL;
		gp->chat_sessions = g_slist_remove(gp->chat_sessions,session);
		g_free(session);
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

	if (event->keyval == 65293) {
		print_debug("main-gui : chat : wcisnieto Enter \n");
		on_send_clicked(s->chat,user_data);
		return TRUE;
	}

	if (event->state & GDK_CONTROL_MASK)
		return on_press_event_switching_tabs(object,event,user_data);
	else
		return FALSE;	
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
	gui_chat_session *s = user_data;
	GtkWidget *input = g_object_get_data(G_OBJECT(s->chat), "input");

	g_return_if_fail(input != NULL);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
		s->autosend_id = g_signal_connect(G_OBJECT(input), "key-press-event", G_CALLBACK(on_input_press_event), user_data);	
	else {
		g_signal_handler_disconnect(G_OBJECT(input),s->autosend_id);
		g_signal_connect(G_OBJECT(input), "key-press-event", G_CALLBACK(on_press_event_switching_tabs), user_data);	
	}

	config_var_set(gui_handler, "send_on_enter", (gpointer)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));
}

void on_send_clicked(GtkWidget *button, gpointer user_data)
{
	gui_chat_session *session = user_data;
	gchar *plugin_name = g_object_get_data(G_OBJECT(session->chat), "plugin_name");
	GtkWidget *input;
	GtkTextBuffer *buf;
	GtkTextIter start, end;
	GGaduMsg *msg;
	gchar *tmpmsg = NULL;

	input = g_object_get_data(G_OBJECT(session->chat), "input");

g_return_if_fail(input != NULL);

if (user_data == NULL)
	print_debug("PUSTE user_data w on_send_clicked\n");

	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(input));
	gtk_text_buffer_get_start_iter(buf, &start);
	gtk_text_buffer_get_end_iter(buf, &end);

	tmpmsg = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
	tmpmsg = g_strchug(g_strchomp(tmpmsg)); //obciete puste spacje z obu stron

	if (strlen(tmpmsg) > 0) { 
	msg = g_new0(GGaduMsg, 1);
	msg->id = session->id;
	msg->message = tmpmsg; 
	    
	if (g_slist_length(session->recipients) > 1)
	    msg->class = GGADU_CLASS_CONFERENCE;
	else
	    msg->class = GGADU_CLASS_CHAT;
		
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
    GtkWidget *emoticons_window;
    GtkWidget *vbox;
    GtkWidget *button_close;
    gui_chat_session *session = user_data;

    emoticons_window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_modal(GTK_WINDOW(emoticons_window), TRUE);
    gtk_window_set_position(GTK_WINDOW(emoticons_window), GTK_WIN_POS_MOUSE);

    g_object_set_data(G_OBJECT(emoticons_window), "session", session);
    g_object_set_data(G_OBJECT(emoticons_window), "emoticons_window", emoticons_window);

    vbox = gtk_vbox_new(TRUE, 0);
    gtk_container_add(GTK_CONTAINER(emoticons_window), vbox);

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
	    g_signal_connect(event_box, "button_press_event", G_CALLBACK(on_emoticon_press_event), gemo);
	    emottmp = emottmp->next;
	    count++;
	}
	    
	g_slist_free(emotlist);
	}

	button_close = gtk_button_new_with_mnemonic(_("_Close"));

	gtk_box_pack_start(GTK_BOX(vbox), button_close, TRUE, TRUE, 0);
	g_signal_connect_swapped(button_close, "clicked", G_CALLBACK(gtk_widget_destroy), emoticons_window);

	gtk_widget_show_all(emoticons_window);
}

void on_chat_find_clicked(GtkWidget *button, gpointer user_data)
{
	GGaduDialog *d = ggadu_dialog_new();
	gui_chat_session *session = user_data;
	gchar  *plugin_name = g_object_get_data(G_OBJECT(session->chat), "plugin_name");

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
    GtkWidget *history;
    GtkWidget *frame;
    GtkWidget *input;
    GtkWidget *paned;
    GtkWidget *sw;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *button_send;
    GtkWidget *button_autosend;
    GtkWidget *button_find;
    GtkWidget *button_close;
    GtkWidget *button_emoticons;
    GtkWidget *button_stick;
    GtkTextBuffer *buf;
    gchar *tmp = NULL;
    gint chat_type = (gint)config_var_get(gui_handler,"chat_type");


    /** ugly topic create - have to be changed **/
    if (g_slist_length(session->recipients) > 1) 
    {
	gchar *prev = NULL;
	gchar *tmprec = "";
	GSList *tmprl = session->recipients;
	
	while (tmprl)
	{
	    
	    if (tmprl->next != NULL)
	        tmprec = g_strconcat(tmprec,tmprl->data," and ",NULL);
	    else
	        tmprec = g_strconcat(tmprec,tmprl->data,NULL);
		
	    g_free(prev);
	    prev = tmprec;
	    tmprl = tmprl->next;
	}
	
	tmp = g_strdup_printf(_("Conference with %s"),tmprec);
	g_free(tmprec);
	
    } else {
    
	GGaduContact *k = gui_find_user(id,gui_find_protocol(plugin_name,protocols));
	
	if (chat_type == CHAT_TYPE_TABBED) {
	
		if (k)
			tmp = g_strdup_printf(_("%s"),k->nick);
		else
			tmp = g_strdup_printf(_("%s"),id);
		
	} else if (chat_type == CHAT_TYPE_CLASSIC) {
	

	    if (k) {
		gui_protocol 	 *gp = gui_find_protocol(plugin_name, protocols);
		GGaduStatusPrototype *sp = gui_find_status_prototype(gp->p,k->status);
		gchar 		 *st = g_strdup_printf("- (%s)", (sp ? sp->description : ""));
		tmp = g_strdup_printf(_("Talking to %s (%s) %s"),k->nick,id, (sp ? st : ""));
		g_free(st);
	    }
	    else
		tmp = g_strdup_printf(_("Talking to %s"),id);
		
	    
	}
	    
    }

    print_debug("chat_type = %d\n",chat_type);

    vbox = gtk_vbox_new(FALSE, 0);
    session->chat = vbox;


    switch (chat_type) 
    {
	case CHAT_TYPE_CLASSIC: 
	    chat_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	    gtk_window_set_title(GTK_WINDOW(chat_window), tmp);
	    gtk_container_add(GTK_CONTAINER(chat_window), vbox);
	    g_free(tmp);
	    break;
	case CHAT_TYPE_TABBED: 
	    {
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
		/* ZONK */		
		id_tmp = g_signal_connect_swapped(G_OBJECT(chat_notebook),"switch-page",G_CALLBACK(gui_chat_notebook_switch),NULL);
		g_object_set_data(G_OBJECT(chat_window),"switch_page_id",(gpointer)id_tmp);
//		g_signal_connect_swapped(G_OBJECT(chat_notebook),"switch-page",G_CALLBACK(gui_chat_notebook_switch),vbox);
		
		gtk_notebook_popup_enable(GTK_NOTEBOOK(chat_notebook));
		g_object_set_data(G_OBJECT(chat_window), "chat_notebook", chat_notebook);
		gtk_container_add(GTK_CONTAINER(chat_window), chat_notebook);
		
		g_signal_connect(G_OBJECT(chat_notebook), "key-press-event", G_CALLBACK(on_press_event_switching_tabs), NULL);
		g_signal_connect(G_OBJECT(chat_window), "key-press-event", G_CALLBACK(on_press_event_switching_tabs), NULL);

	    } else {
		chat_notebook = g_object_get_data(G_OBJECT(chat_window),"chat_notebook");
	    }

	    tab_label_hbox  = gtk_hbox_new(FALSE,FALSE);
	    tab_label_txt   = gtk_label_new(tmp);
	    tab_label_close = gtk_button_new();
	    
	    gtk_button_set_relief(GTK_BUTTON(tab_label_close),GTK_RELIEF_NONE);
	    g_signal_connect(tab_label_close,  "clicked",  G_CALLBACK(on_destroy_chat), session);

	    gtk_widget_set_usize(GTK_WIDGET(tab_label_close),12,1);
	    
	    gtk_image_set_from_stock(GTK_IMAGE(tab_label_close_image),"gtk-close",GTK_ICON_SIZE_MENU);
	    gtk_container_add(GTK_CONTAINER(tab_label_close),GTK_WIDGET(tab_label_close_image));
	    
	    gtk_box_pack_start_defaults(GTK_BOX(tab_label_hbox),tab_label_txt);
	    gtk_box_pack_start_defaults(GTK_BOX(tab_label_hbox),tab_label_close);
	    
	    g_object_set_data(G_OBJECT(session->chat),"tab_label_txt",tab_label_txt);
	    g_object_set_data_full(G_OBJECT(session->chat),"tab_label_txt_char",g_strdup(tmp),g_free);
	    
	    gtk_notebook_append_page(GTK_NOTEBOOK(chat_notebook),session->chat,tab_label_hbox);
	    gtk_notebook_set_menu_label_text(GTK_NOTEBOOK(chat_notebook),session->chat,tmp);
	    
	    gtk_widget_show_all(tab_label_hbox);
	    }
	    break;
    }
    
    gtk_window_set_default_size(GTK_WINDOW(chat_window), 400, 300);
    gtk_window_set_modal(GTK_WINDOW(chat_window), FALSE);
    gtk_widget_set_name(GTK_WIDGET(chat_window),"GGChat");

    /* g_free(tmp); */
    g_object_set_data(G_OBJECT(session->chat), "plugin_name", plugin_name);
    g_object_set_data(G_OBJECT(session->chat), "top_window", chat_window);
    
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
    
    {
	gchar *colorstr, *fontstr;

	colorstr = config_var_get(gui_handler, "msg_header_color");
	fontstr = config_var_get(gui_handler, "msg_header_font");

	gtk_text_buffer_create_tag (buf, "incoming_header",
				    "foreground",
				    (colorstr) ? colorstr : DEFAULT_TEXT_COLOR,
				    "font", 
				    (fontstr) ? fontstr : DEFAULT_FONT,
				    NULL);

	colorstr = config_var_get(gui_handler, "msg_body_color");
	fontstr = config_var_get(gui_handler, "msg_body_font");
				    
	gtk_text_buffer_create_tag (buf, "incoming_text",
				    "foreground", 
				    (colorstr) ? colorstr : DEFAULT_TEXT_COLOR,
				    "font", 
				    (fontstr) ? fontstr : DEFAULT_FONT,
				    NULL);

	colorstr = config_var_get(gui_handler, "msg_out_header_color");
	fontstr = config_var_get(gui_handler, "msg_out_header_font");

	gtk_text_buffer_create_tag (buf, "outgoing_header",
				    "foreground",
				    (colorstr) ? colorstr : DEFAULT_TEXT_COLOR,
				    "font", 
				    (fontstr) ? fontstr : DEFAULT_FONT,
				    NULL);

	colorstr = config_var_get(gui_handler, "msg_out_body_color");
	fontstr = config_var_get(gui_handler, "msg_out_body_font");

	gtk_text_buffer_create_tag (buf, "outgoing_text",
				    "foreground", 
				    (colorstr) ? colorstr : DEFAULT_TEXT_COLOR,
				    "font", 
				    (fontstr) ? fontstr : DEFAULT_FONT,
				    NULL);
    }		      
    input = gtk_text_view_new();
    gtk_widget_set_name(GTK_WIDGET(input),"GGInput");
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(input),GTK_WRAP_WORD);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(input),2);

    gtk_widget_ref(input);
    g_object_set_data_full(G_OBJECT(session->chat), "input", input, (GDestroyNotify) gtk_widget_unref);
	
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), 
				GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(sw), history);
	
    paned = gtk_vpaned_new();
    
    gtk_paned_add1(GTK_PANED(paned), sw);
 
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), 
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(sw), input);   
    gtk_paned_add2(GTK_PANED(paned), sw);
    
    gtk_paned_set_position(GTK_PANED(paned),150);
    
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(frame), paned);
	
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    button_send = gtk_button_new_with_mnemonic(_("_Send"));
    gtk_button_set_relief(GTK_BUTTON(button_send),GTK_RELIEF_NONE);

    gtk_box_pack_start(GTK_BOX(hbox), button_send, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(button_send), "plugin_name", plugin_name);

    button_autosend = gtk_toggle_button_new();
    gtk_button_set_relief(GTK_BUTTON(button_autosend),GTK_RELIEF_NONE);

    if (config_var_get(gui_handler, "send_on_enter")) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_autosend),TRUE);
	session->autosend_id = g_signal_connect(G_OBJECT(input), "key-press-event", G_CALLBACK(on_input_press_event), session);
    } else {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_autosend),FALSE);
	g_signal_connect(G_OBJECT(input), "key-press-event", G_CALLBACK(on_press_event_switching_tabs), NULL);
    }

    g_object_set_data(G_OBJECT(button_autosend), "plugin_name", plugin_name);

    gtk_container_add(GTK_CONTAINER(button_autosend), 
	    create_image("arrow.png"));

    gtk_box_pack_start(GTK_BOX(hbox), button_autosend, FALSE, FALSE, 0);
    
    button_find = gtk_button_new_from_stock("gtk-find");
    gtk_button_set_relief(GTK_BUTTON(button_find),GTK_RELIEF_NONE);

    gtk_box_pack_start(GTK_BOX(hbox), button_find, FALSE, FALSE, 0);

    if (emoticons) {
	button_emoticons = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(button_emoticons),GTK_RELIEF_NONE);

        gtk_container_add(GTK_CONTAINER(button_emoticons), create_image("emoticon.gif"));
        gtk_box_pack_start(GTK_BOX(hbox), button_emoticons, FALSE, FALSE, 0);
	g_signal_connect(button_emoticons, "clicked", G_CALLBACK(on_emoticons_clicked), session);
    }

    button_close = gtk_button_new_from_stock("gtk-close");
    gtk_button_set_relief(GTK_BUTTON(button_close),GTK_RELIEF_NONE);

    g_object_set_data(G_OBJECT(button_close),"plugin_name",plugin_name);
    gtk_box_pack_end(GTK_BOX(hbox), button_close, FALSE, FALSE, 0);

    
    button_stick = gtk_toggle_button_new_with_mnemonic(_("S_tick"));
    gtk_button_set_relief(GTK_BUTTON(button_stick),GTK_RELIEF_NONE);
    
    gtk_box_pack_end(GTK_BOX(hbox), button_stick, FALSE, FALSE, 0);
    
    if (visible) {
	gtk_widget_show_all (session->chat);
	
	if (chat_window != NULL) 
	    gtk_widget_show_all (chat_window);
	
    } else {
	invisible_chats = g_slist_append(invisible_chats,chat_window);
    }

    g_signal_connect(button_send, "clicked", G_CALLBACK(on_send_clicked), session);
    g_signal_connect(button_autosend, "clicked", G_CALLBACK(on_autosend_clicked), session);
    g_signal_connect(button_find, "clicked", G_CALLBACK(on_chat_find_clicked), session);
    g_signal_connect(button_stick, "toggled", G_CALLBACK(on_stick_clicked), session);
    
    if (chat_type == CHAT_TYPE_CLASSIC) {
	g_signal_connect_swapped(button_close, "clicked", G_CALLBACK(gtk_widget_destroy), chat_window);
	g_signal_connect(chat_window,  "destroy",  G_CALLBACK(on_destroy_chat), session);
    } else if (chat_type == CHAT_TYPE_TABBED) {
	g_signal_connect(button_close,  "clicked",  G_CALLBACK(on_destroy_chat), session);
	g_signal_connect(chat_window,  "destroy",  G_CALLBACK(on_destroy_chat_window), NULL);
    }
    
    gtk_widget_grab_focus(GTK_WIDGET(input));
    
    return vbox;
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
	    GtkWidget 	 *chat_notebook    = g_object_get_data(G_OBJECT(chat_window),"chat_notebook");
	    gint	 curr_page         = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
	    GtkWidget	 *curr_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(chat_notebook),curr_page);
		
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

    if (self == FALSE)
        gtk_text_buffer_insert_with_tags_by_name(buf, &iter, header, -1, "incoming_header", NULL);
    else
        gtk_text_buffer_insert_with_tags_by_name(buf, &iter, header, -1, "outgoing_header", NULL);

	    
    tmp = g_strconcat(text,"\n",NULL);
	
    if ((mark_start = gtk_text_buffer_get_mark(buf,"gg_new_text_start")) == NULL) 
	mark_start = gtk_text_buffer_create_mark(buf,"gg_new_text_start",&iter,FALSE);
    else
        gtk_text_buffer_move_mark(buf,mark_start,&iter);
		
    if (self == FALSE)
        gtk_text_buffer_insert_with_tags_by_name(buf, &iter, tmp, -1, "incoming_text",NULL);
    else
        gtk_text_buffer_insert_with_tags_by_name(buf, &iter, tmp, -1, "outgoing_text",NULL);
	
    g_free(tmp);

	/* scroll only if we at the bottom of the history :) I'm genius */
/*	if (GTK_TEXT_VIEW(history)->vadjustment) 
	{
	    GtkAdjustment *adj = GTK_TEXT_VIEW(history)->vadjustment;

	    if ((adj->value + adj->page_size) == adj->upper) 
*/	    {

		gtk_text_buffer_get_end_iter(buf, &iter);
		gtk_text_buffer_place_cursor(buf,&iter);
		gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(history), 
					    gtk_text_buffer_get_insert(buf)
					    , 0.0, TRUE, 0.5, 0.5);

	    }

	/* emoticons engine initial, it works !, but is case sensitive */
    {
        GtkTextIter istart;
        GtkTextIter iend;
        GtkWidget   *widget = NULL;
        GSList *emottmp = emoticons;
	    
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
    }

	/* Is it should be called somewhere or not ? this is a question, rather no, but....*/
	/* gtk_text_buffer_delete_mark_by_name(buf,"gg_end"); */

    if (((gint)config_var_get(gui_handler,"chat_window_auto_raise") == TRUE) && (!self) && 
	(GTK_WIDGET_VISIBLE(chat))) {
    	    GtkWidget *win = g_object_get_data(G_OBJECT(chat),"top_window");
	    gtk_window_present(GTK_WINDOW(win));
    }
	
    g_free(timestamp);
    g_free(header);
    g_free(text);
}
