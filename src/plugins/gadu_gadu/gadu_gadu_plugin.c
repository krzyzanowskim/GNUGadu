/* $Id: gadu_gadu_plugin.c,v 1.23 2003/04/10 18:11:51 krzyzak Exp $ */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <libgadu.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "support.h"
#include "menu.h"
#include "gadu_gadu_plugin.h"
#include "dialog.h"

extern GGaduConfig *config;

GGaduPlugin *handler;

struct gg_session *session;

static gint connect_count = 0;
static guint watch = 0;
gint http_watch = 0;

GSList *userlist = NULL;
GIOChannel *source_chan = NULL;

gboolean connected = FALSE;
gchar *this_configdir = NULL;

GGaduProtocol *p;
GGaduMenu *menu_pluginmenu;

GGadu_PLUGIN_INIT("gadu-gadu",GGADU_PLUGIN_TYPE_PROTOCOL);


/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 						PRIVATE STUFF
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
void test() {
}


void ggadu_gadu_gadu_disconnect()
{
    GSList *tmplist = userlist;
    
    connected = FALSE;
    g_source_remove(watch);
    gg_logoff(session);
    gg_free_session(session);
    session = NULL;
    signal_emit(GGadu_PLUGIN_NAME, "gui disconnected",NULL,"main-gui");

    while (tmplist) {
	GGaduContact *k = tmplist->data;
	fprintf(stderr, "\t%s\n", k->id);
	k->status = GG_STATUS_NOT_AVAIL;
	tmplist = tmplist->next;
    }
}

void ggadu_gadu_gadu_disconnect_msg(gchar *txt)
{
    ggadu_gadu_gadu_disconnect();
    print_debug("%s\n",txt);
    signal_emit(GGadu_PLUGIN_NAME, "gui show warning",g_strdup((txt) ? txt : _("Connection failed!")),"main-gui");
}

gchar *insert_cr(gchar *txt)
{
    gchar *in = txt;
    gchar *out = g_malloc0(strlen(txt)*2);
    gchar *start = out;

    while (*in) 
    {
	
	if (*in == '\n') {
	    *out = '\r';
	    out++;
	}

	*out = *in;
	
	in++;
	out++;
    }
    
    start = g_try_realloc(start,strlen(start)+1);
    
    return start;
}

gpointer gadu_gadu_login(gpointer desc, gint status)
{
    struct gg_login_params p;
    gchar *serveraddr = (gchar *)config_var_get(handler,"server");
    gchar **serv_addr = NULL;

    if (connected) {
    	gg_logoff(session);
	gg_free_session(session);
	connected = FALSE;
	return FALSE;
    }
    
    memset(&p, 0, sizeof(p));

    p.server_port =  GG_DEFAULT_PORT;
    if (serveraddr == NULL)
	serveraddr = g_strdup("217.17.41.85");
    else {
	serv_addr = g_strsplit(serveraddr,":",2);
	
	if (serv_addr) {
	    serveraddr = serv_addr[0];
	    if (serv_addr[1] != NULL)
		p.server_port =  g_strtod(serv_addr[1],NULL);
	    else
		p.server_port =  GG_DEFAULT_PORT;
	}
    }
    
    print_debug("loguje sie GG# %d do serwera %s %d\n",(gint)config_var_get(handler,"uin"),serveraddr,p.server_port);

    p.uin	= (int) config_var_get(handler,"uin");
    p.password	= (gchar *)config_var_get(handler,"password");
    p.async	= 1;
    p.status	= status;
		p.status_descr = desc;
    if (config_var_get(handler, "private"))
	p.status |= GG_STATUS_FRIENDS_MASK;
    
    if (serveraddr != NULL)
	p.server_addr = inet_addr(serveraddr);

    if (!p.uin || (!p.password || !*p.password)) {
	user_preferences_action(NULL);
	signal_emit(GGadu_PLUGIN_NAME, "gui show warning",g_strdup(_("You have to enter your GG# and password first!")),"main-gui");
	ggadu_gadu_gadu_disconnect();
	return NULL;
    }
    
    g_free(serveraddr);
    
    if (!(session = gg_login(&p))) {
	print_debug("%s \t\t connection failed\n", GGadu_PLUGIN_NAME);
	ggadu_gadu_gadu_disconnect_msg(NULL);
	return NULL;
    }

    /* do tego chyba przydal by sie jakis nasz wrapper choc tak naprawde nie jest potrzebny */
    source_chan = g_io_channel_unix_new(session->fd);
    watch = g_io_add_watch(source_chan, G_IO_IN | G_IO_ERR | G_IO_HUP, test_chan,NULL);
    return NULL;
}

gboolean gadu_gadu_ping(gpointer data)
{
    if (connected == FALSE)
	return FALSE;

    gg_ping(session);
    print_debug("PING! send!\n");

    return TRUE;    
}

void handle_search_event(struct gg_event *e)
{
    gg_pubdir50_t res = e->event.pubdir50;
    gint count, i;
    GSList *list = NULL;

    if ((count = gg_pubdir50_count(res)) < 1) {
	signal_emit(GGadu_PLUGIN_NAME, "gui show message",g_strdup(_("No users have been found!")),"main-gui");
    	return;
    }

    for (i = 0; i < count; i++) {
	GGaduContact *k = g_new0(GGaduContact, 1);
	const gchar * uin = gg_pubdir50_get(res, i, GG_PUBDIR50_UIN);
	const gchar * first_name = gg_pubdir50_get(res, i, GG_PUBDIR50_FIRSTNAME);
	const gchar * nick = gg_pubdir50_get(res, i, GG_PUBDIR50_NICKNAME);
	const gchar * status = gg_pubdir50_get(res, i, GG_PUBDIR50_STATUS);

	k->id = g_strdup((uin) ? uin : "?");

	if (first_name != NULL) to_utf8("CP1250", first_name, k->first_name);

	if (nick != NULL) to_utf8("CP1250", nick, k->nick);

	k->status = (status) ? atoi(status) : GG_STATUS_NOT_AVAIL;
    
	list = g_slist_append(list, k);
    }
    signal_emit(GGadu_PLUGIN_NAME, "gui show search results", list, "main-gui");
}

void ggadu_gg_save_history(gchar *to,gchar *txt)
{

	if (config_var_get(handler, "log")) {
	    gchar *dir = g_build_filename(this_configdir, "history",NULL);
	    gchar *path = g_build_filename(this_configdir, "history", to, NULL);

	    if (!g_file_test(dir,G_FILE_TEST_IS_DIR))
		mkdir(dir, 0700);

	    write_line_to_file(path,txt,"CP1250");

	    g_free(path);
	    g_free(dir);
	}
}

gboolean test_chan(GIOChannel *source, GIOCondition condition, gpointer data) 
{
	struct gg_event *e = NULL;
	struct gg_notify_reply *n = NULL;
	GSList *slistmp = NULL;
	uint32_t  *uins;
	GGaduNotify *notify = NULL;
	GGaduMsg *msg = NULL;
	gint    i,j;

    /* w przypadku b³êdu/utraty po³±czenia post±p tak jak w przypadku disconnect */
	if (!(e = gg_watch_fd(session)) || (condition & G_IO_ERR) || (condition & G_IO_HUP)) {
		connected = FALSE;

		if ( ++connect_count < 3){
			ggadu_gadu_gadu_disconnect();
			gadu_gadu_login(NULL,GG_STATUS_AVAIL);
		} else {
			gchar *txt = g_strdup(_("Disconnected"));
			ggadu_gadu_gadu_disconnect_msg(txt);
			connect_count = 0;
			g_free(txt);
		}
		
		return FALSE;
		
	}

	switch (e->type) 
    {
		case GG_EVENT_NONE:
			print_debug("GG_EVENT_NONE!!\n");
			break;

		case GG_EVENT_PONG:
			gg_ping(session);
			print_debug("GG_EVENT_PONG!\n");
			break;

		case GG_EVENT_CONN_SUCCESS:
			print_debug("po³±czono!\n");
			connected = TRUE;

			/* notify wysylam */
			slistmp = userlist; 
			i=0;

			while (slistmp) { 
			    i++; 
			    slistmp = slistmp->next;
			}
			
			uins = g_malloc0(i * sizeof(uint32_t));
			slistmp = userlist; 
			j=0;

			while (slistmp) { 
			    GGaduContact *k = slistmp->data;

			    uins[j++] = atoi(k->id);
			    slistmp = slistmp->next;
			}

			gg_notify(session,uins,i);
			g_free(uins);
			/* pingpong */
			g_timeout_add(100000, gadu_gadu_ping,NULL);
			signal_emit(GGadu_PLUGIN_NAME, "sound play file", config_var_get(handler, "sound_app_file"),"sound*");
			signal_emit(GGadu_PLUGIN_NAME, "gui status changed", 
				    (gpointer) ((session->status & GG_STATUS_FRIENDS_MASK) ? session->status ^ GG_STATUS_FRIENDS_MASK : session->status), 
				    "main-gui");
			break;

		case GG_EVENT_CONN_FAILED:
			print_debug("nie uda³o siê polaczyc\n");
			ggadu_gadu_gadu_disconnect_msg(_("Unable to authenticate (propably)"));
			break;

		case GG_EVENT_DISCONNECT:
			ggadu_gadu_gadu_disconnect_msg(_("Disconnected"));
			gadu_gadu_login(NULL,GG_STATUS_AVAIL);
			break;

		case GG_EVENT_MSG:
			print_debug("you have message!\n");
	    		print_debug("from: %d\n", e->event.msg.sender);
			print_debug("body: %s\n", e->event.msg.message);
			
			msg = g_new0(GGaduMsg,1);
			msg->id = g_strdup_printf("%d",e->event.msg.sender);
			to_utf8("CP1250",e->event.msg.message,msg->message);
			msg->class = e->event.msg.msgclass;
			msg->time = e->event.msg.time;
			
			if (e->event.msg.recipients_count > 0) {
			    gint i = 0;
			    uin_t *recipients_gg = e->event.msg.recipients;
			    
			    print_debug("CONFERENCE MESSAGE %d\n",e->event.msg.recipients_count);

			    msg->class = GGADU_CLASS_CONFERENCE;
			    msg->recipients = g_slist_append(msg->recipients,(gpointer)g_strdup_printf("%d",e->event.msg.sender));
			    
			    for (i=0;i<e->event.msg.recipients_count;i++)
				msg->recipients = g_slist_append(msg->recipients,(gpointer)g_strdup_printf("%d",recipients_gg[i]));
			
			}
			
			{
			    gchar *line = g_strdup_printf(":: %s (%s)::\n%s\n\n",msg->id,get_timestamp(msg->time),msg->message);
			    print_debug("%s\n",line);
			    ggadu_gg_save_history(msg->id,line);
			    g_free(line);
			}
			
			signal_emit(GGadu_PLUGIN_NAME, "gui msg receive",msg,"main-gui");
			signal_emit(GGadu_PLUGIN_NAME, "sound play file", config_var_get(handler, "sound_msg_file"),"sound*");
			break;

		case GG_EVENT_NOTIFY:
			n = e->event.notify;
			
			while (n->uin) 
			{
			    print_debug("%s : GG_EVENT_NOTIFY : %d  %d\n",GGadu_PLUGIN_NAME,n->uin,n->status);
			    
			    notify = g_new0(GGaduNotify,1);
			    notify->id = g_strdup_printf("%d",n->uin);
			    notify->status = n->status;
			    set_userlist_status(notify->id, n->status, NULL, userlist);
			    signal_emit(GGadu_PLUGIN_NAME,"gui notify",notify,"main-gui");

			    n++;
			}
			
			break;

		case GG_EVENT_NOTIFY_DESCR:
			n = e->event.notify_descr.notify;
			
			while (n->uin) 
			{
			    print_debug("%s : GG_EVENT_NOTIFY_DESCR : %d  %d %s\n",GGadu_PLUGIN_NAME,n->uin,n->status,e->event.notify_descr.descr);
			
			    notify = g_new0(GGaduNotify,1);
			    notify->id = g_strdup_printf("%d",n->uin);

			    notify->status = n->status;
			    set_userlist_status(notify->id, n->status, e->event.notify_descr.descr,userlist);
			    signal_emit(GGadu_PLUGIN_NAME,"gui notify",notify,"main-gui");

			    n++;
			}
			break;

		case GG_EVENT_STATUS:
			print_debug("%s : GG_EVENT_STATUS : %d %s\n",GGadu_PLUGIN_NAME,e->event.status.uin,
											   e->event.status.descr);
			notify = g_new0(GGaduNotify,1);
			notify->id = g_strdup_printf("%d",e->event.status.uin);
			notify->status = e->event.status.status;
			set_userlist_status(notify->id, notify->status, e->event.status.descr,userlist);
			signal_emit(GGadu_PLUGIN_NAME,"gui notify",notify,"main-gui");
			break;

		case GG_EVENT_ACK:

			if (e->event.ack.status == GG_ACK_QUEUED) 
			    print_debug("wiadomo¶æ bedzie dostarczona pozniej do %d.\n",e->event.ack.recipient);
			else
			    print_debug("wiadomo¶æ dotar³a do %d.\n",e->event.ack.recipient);
			    
 			break;
		case GG_EVENT_PUBDIR50_SEARCH_REPLY:
			handle_search_event(e);			
			break;
	}

	gg_free_event(e);
	
	return TRUE;
}


void exit_signal_handler() {
		if (connected) 
			gg_logoff(session);

    show_error(_("Program is about to leave"));
}

gpointer user_chat_action(gpointer user_data)
{
    GSList	*users = (GSList *)user_data;
    GGaduMsg    *msg   = g_new0(GGaduMsg,1);

    if (!user_data) return NULL;
    
    if (g_slist_length(users) > 1) {
        msg->class = GGADU_CLASS_CONFERENCE;

	while(users) {
	    GGaduContact *k = (GGaduContact *)users->data;
	    msg->id = k->id;
	    msg->recipients = g_slist_append(msg->recipients,k->id);
	    users = users->next;
	}
	
    } else {
	GGaduContact *k    = (GGaduContact *)users->data;
        msg->class = GGADU_CLASS_CHAT;
        msg->id = k->id;
    }

    msg->message = NULL;
    signal_emit(GGadu_PLUGIN_NAME, "gui msg receive",msg,"main-gui");
    return NULL;
}

gpointer user_view_history_action(gpointer user_data)
{
    gsize length, terminator;
    GIOChannel	 *ch   = NULL;
    gchar	 *line = NULL;
    gchar	 *path = NULL;
    gchar	 *utf8str = NULL;
    GString	 *hist_buf = g_string_new(NULL);
    GSList 	 *users	   = (GSList *)user_data;
    GGaduContact *k			= (users) ? (GGaduContact *)users->data : NULL;

		if (!k) return NULL;
    
    path = g_build_filename(this_configdir, "history", k->id, NULL);
    ch   = g_io_channel_new_file(path,"r",NULL);
    g_free(path);
    
    if (!ch) return NULL;
    
    g_io_channel_set_encoding(ch,"CP1250",NULL);
    
    while (g_io_channel_read_line(ch, &line,  &length, &terminator, NULL) != G_IO_STATUS_EOF)
    {
	if (line != NULL)
	    g_string_append(hist_buf,line);
    }

    g_io_channel_shutdown(ch,TRUE,NULL);

    signal_emit(GGadu_PLUGIN_NAME, "gui show window with text",hist_buf->str,"main-gui");
    g_free(utf8str);
    
    /* zwonic ten hist_buf */
    g_string_free(hist_buf,TRUE);
    
    return NULL;
}

gpointer user_remove_user_action(gpointer user_data)
{
	GSList *users = (GSList *)user_data;

	while (users) 
	{
		GGaduContact *k = (GGaduContact *)users->data;
		userlist = g_slist_remove(userlist,k);

		if (connected && session)
			gg_remove_notify(session,atoi(k->id));

		GGaduContact_free(k);
		users = users->next;
	}

	if ((user_data) && (userlist)) {
		signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
		save_addressbook_file(userlist);
    }
	
	return NULL;
}

gpointer user_change_user_action(gpointer user_data)
{
    GSList	 *optlist  	= NULL;
    GSList 	 *users		= (GSList *)user_data;
    GGaduContact *k		= (GGaduContact *)users->data;

    ggadu_dialog_add_entry(&optlist, GGADU_ID, "GG#", VAR_STR, k->id, VAR_FLAG_INSENSITIVE); 
    ggadu_dialog_add_entry(&optlist, GGADU_NICK, _("Nick"), VAR_STR, k->nick, VAR_FLAG_SENSITIVE); 
    ggadu_dialog_add_entry(&optlist, GGADU_FIRST_NAME, _("First Name"), VAR_STR, k->first_name, VAR_FLAG_SENSITIVE); 
    ggadu_dialog_add_entry(&optlist, GGADU_LAST_NAME, _("Last Name"), VAR_STR, k->last_name, VAR_FLAG_SENSITIVE); 
    ggadu_dialog_add_entry(&optlist, GGADU_MOBILE, _("Phone"), VAR_STR, k->mobile, VAR_FLAG_SENSITIVE); 

    /* wywoluje okienko zmiany usera */
    signal_emit(GGadu_PLUGIN_NAME, "gui change user window", optlist, "main-gui");

    return NULL;
    
}

gpointer user_add_user_action(gpointer user_data)
{
    GSList *optlist   = NULL;

    ggadu_dialog_add_entry(&optlist, GGADU_ID, "GG#", VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&optlist, GGADU_NICK, _("Nick"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&optlist, GGADU_FIRST_NAME, _("First Name"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&optlist, GGADU_LAST_NAME, _("Last Name"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&optlist, GGADU_MOBILE, _("Phone"), VAR_STR, NULL, VAR_FLAG_NONE);

    /* wywoluje okienko dodawania usera */
    signal_emit(GGadu_PLUGIN_NAME, "gui add user window", optlist, "main-gui");
    
    return NULL;
}

gpointer user_preferences_action(gpointer user_data)
{
	gchar *utf = NULL;
	GGaduDialog *d = ggadu_dialog_new();
	GSList *statuslist_names = NULL;
	GSList *tmplist = p->statuslist;

	while (tmplist) {
		GGaduStatusPrototype *sp = tmplist->data;
		if (!sp->receive_only)
			statuslist_names  = g_slist_append(statuslist_names,sp->description);	
		tmplist = tmplist->next;
	}


	ggadu_dialog_set_title(d, _("Gadu-gadu plugin configuration"));
	ggadu_dialog_set_type(d, GGADU_DIALOG_CONFIG);       
	ggadu_dialog_callback_signal(d,"update config");
	ggadu_dialog_add_entry(&(d->optlist), GGADU_GADU_GADU_CONFIG_ID, "GG#", VAR_INT, config_var_get(handler, "uin"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(&(d->optlist), GGADU_GADU_GADU_CONFIG_PASSWORD, _("Password"), VAR_STR, config_var_get(handler, "password"), VAR_FLAG_PASSWORD);
	ggadu_dialog_add_entry(&(d->optlist), GGADU_GADU_GADU_CONFIG_SERVER, _("Server"), VAR_STR, config_var_get(handler, "server"), VAR_FLAG_NONE);
	to_utf8("ISO-8859-2",config_var_get(handler, "reason"),utf);
	ggadu_dialog_add_entry(&(d->optlist), GGADU_GADU_GADU_CONFIG_REASON, _("Default reason"), VAR_STR, utf, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(&(d->optlist), GGADU_GADU_GADU_CONFIG_HISTORY, _("Log chats to history file"), VAR_BOOL, config_var_get(handler, "log"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(&(d->optlist), GGADU_GADU_GADU_CONFIG_AUTOCONNECT, _("Autoconnect on startup"), VAR_BOOL, config_var_get(handler, "autoconnect"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(&(d->optlist), GGADU_GADU_GADU_CONFIG_AUTOCONNECT_STATUS, _("Autoconnect status"), VAR_LIST, statuslist_names, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(&(d->optlist), GGADU_GADU_GADU_CONFIG_FRIENDS_MASK, _("Available only for friends"), VAR_BOOL, config_var_get(handler, "private"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(&(d->optlist), GGADU_GADU_GADU_CONFIG_SOUND_APP_FILE, _("Sound file (app)"), VAR_FILE_CHOOSER, config_var_get(handler, "sound_app_file"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(&(d->optlist), GGADU_GADU_GADU_CONFIG_SOUND_CHAT_FILE, _("Sound file (chat)"), VAR_FILE_CHOOSER, config_var_get(handler, "sound_chat_file"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(&(d->optlist), GGADU_GADU_GADU_CONFIG_SOUND_MSG_FILE, _("Sound file (msg)"), VAR_FILE_CHOOSER, config_var_get(handler, "sound_msg_file"), VAR_FLAG_NONE);

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");
    
	return NULL;
}

gpointer search_action(gpointer user_data)
{
    GGaduDialog *d = NULL;
    
    if (!connected) {
	signal_emit(GGadu_PLUGIN_NAME, "gui show warning",g_strdup(_("You have to be connected to perform searching!")),"main-gui");
	return NULL;
    }
    
    d = ggadu_dialog_new();
    
    ggadu_dialog_set_title(d,_("Gadu-Gadu search"));
    ggadu_dialog_callback_signal(d,"search");
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SEARCH_FIRSTNAME, _("First name:"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SEARCH_LASTNAME, _("Last name:"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SEARCH_NICKNAME, _("Nick:"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SEARCH_CITY, _("City:"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SEARCH_ID, _("GG#"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SEARCH_ACTIVE, _("Search only for active users"), VAR_BOOL, NULL, VAR_FLAG_NONE);
    
    signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");
        
    return NULL;
}

gchar *userlist_dump(GSList *list)
{
	gchar *dump = NULL;
	GSList *us = list;
    
	while (us)
	{
		gchar	 *line		= NULL;
		GGaduContact *k		= (GGaduContact *)us->data;    
	    
		line = g_strdup_printf("%s;%s;%s;%s;%s;%s;%s;;;;;\r\n",k->first_name,k->last_name,k->nick,k->comment,k->mobile,k->group,k->id);
	    
		if (!dump) 
			dump = g_strdup(line);
		else {
			gchar *tmp = dump;
			dump = g_strjoin(NULL, dump, line, NULL);
			g_free(tmp);
		}
	    
		g_free(line);
		us = us->next;
	}
	
	print_debug("userlist_dump\n");
	return dump;
}
gboolean user_exists(gchar *id)
{
    GSList *tmp = userlist;
    
    while (tmp) {
	GGaduContact *k = tmp->data;
	if (!ggadu_strcasecmp(k->id,id)) { 
	    return TRUE;
	}
	tmp = tmp->next;
    }
    return FALSE;
}

void import_userlist(gchar *list)
{
	gchar **all, **tmp;
	all = g_strsplit(list, "\r\n", 1000);
	tmp = all;
	
	while (*tmp) {
		gchar *first_name, *last_name, *nick, *comment, *mobile, *group, *uin;
		gchar **l = NULL;
		GGaduContact *k;
		print_debug("pointer %p -> %p\n", tmp, *tmp);
		l = g_strsplit(*tmp, ";", 12);
		tmp++;

		if (!l[0])
		    continue;
		
		first_name = l[0];
		last_name = l[1];
		nick = l[2];
		mobile = l[4];
		group = l[5];
		uin = l[6];
		comment = l[7];
		
		print_debug("dupa %s %p %p\n", uin, uin, mobile);
		if ((!uin) && (!mobile))
			continue;

		if (user_exists(uin))
			continue;

		k = g_new0(GGaduContact, 1);

		k->id = uin ? g_strdup(uin) : g_strdup("");
		
		k->first_name = g_strdup(first_name);
		k->last_name = g_strdup(last_name);
		k->nick = (!strlen(nick)) ? g_strconcat(first_name," ",last_name,NULL) : g_strdup(nick);
		k->comment = g_strdup(comment);
		k->mobile = g_strdup(mobile);
		k->group = g_strdup(group);
		k->status = GG_STATUS_NOT_AVAIL;

		userlist = g_slist_append(userlist, k);
		
		if (connected && session)
		    gg_add_notify(session,atoi(k->id));
		
		g_strfreev(l);
		print_debug("dupa end %s\n", uin);
    }
    
    signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
    save_addressbook_file(userlist);

    g_strfreev(all);
    g_free(list);
}

gboolean handle_userlist(GIOChannel *source, GIOCondition condition, gpointer data) 
{
    struct gg_http *h = data;

    if (!h)
	return FALSE;

    if ((condition & G_IO_ERR)) 
    {
	print_debug("Condition with error\n");
	goto abort;
    }
    
    if (condition & G_IO_IN || condition & G_IO_OUT) {
    
	if (h->callback(h) == -1 || h->state == GG_STATE_ERROR) {
	    print_debug("error ocurred\n");
	    goto abort;
	}

	if (h->state == GG_STATE_CONNECTING) {
	    return TRUE;
	}
	
	if (h->state == GG_STATE_READING_HEADER) {
	    g_io_add_watch(source, G_IO_IN | G_IO_ERR, handle_userlist, h);
	    return FALSE;
	}
	
	if (h->state != GG_STATE_DONE)
	    return TRUE;
	
	if (h->type == GG_SESSION_USERLIST_GET) {
	    if (h->data) {
		gchar *list;
		to_utf8("CP1250", h->data, list);
		import_userlist(list);
	    	signal_emit(GGadu_PLUGIN_NAME, "gui show message",g_strdup(_("Userlist has been imported succesfully!")),"main-gui");
	    } else
	    	signal_emit(GGadu_PLUGIN_NAME, "gui show warning",g_strdup(_("Server returned empty userlist!")),"main-gui");

	} else if (h->type == GG_SESSION_USERLIST_PUT) {
	    if (!h->user_data) {
	        if (h->data)
		    signal_emit(GGadu_PLUGIN_NAME, "gui show message",g_strdup(_("Userlist export succeeded!")),"main-gui");
		else 
		    signal_emit(GGadu_PLUGIN_NAME, "gui show warning",g_strdup(_("Userlist export failed!")),"main-gui");
	    } else {
	        if (h->data)
		    signal_emit(GGadu_PLUGIN_NAME, "gui show message",g_strdup(_("Userlist delete succeeded!")),"main-gui");
		else 
		    signal_emit(GGadu_PLUGIN_NAME, "gui show warning",g_strdup(_("Userlist delete failed!")),"main-gui");
	    }
	}
    }

abort:   
    print_debug("shutting down channel\n"); 
    g_io_channel_shutdown(source, FALSE, NULL);
    g_io_channel_unref(source);
    
    h->destroy(h);
    
    return FALSE;
}

gpointer import_userlist_action(gpointer user_data)
{
    struct gg_http *h;
    GIOChannel *chan;
    
    if (!(h = gg_userlist_get((gint)config_var_get(handler, "uin"), config_var_get(handler, "password"), 1))) {
	print_debug("userlist get error!\n");
	return NULL;
    }

    chan = g_io_channel_unix_new(h->fd);
    g_io_add_watch(chan, G_IO_IN | G_IO_OUT | G_IO_ERR, handle_userlist, h);

    return NULL;
}

gpointer export_userlist_action(gpointer user_data)
{
	struct gg_http *h;
	gchar *dump, *tmp = userlist_dump(userlist);
	GIOChannel *chan;
        
	from_utf8("CP1250", tmp, dump);
	g_free(tmp);
    
	if (!(h = gg_userlist_put((gint)config_var_get(handler, "uin"), config_var_get(handler, "password"), dump, 1))) {
		print_debug("userlist put error!\n");
		return NULL;
	}
    
	chan = g_io_channel_unix_new(h->fd);
	g_io_add_watch(chan, G_IO_IN | G_IO_OUT | G_IO_ERR, handle_userlist, h);
	g_free(dump);
    
	return NULL;
}

gpointer delete_userlist_action(gpointer user_data)
{
    struct gg_http *h;
    GIOChannel *chan;
    gchar *dump = g_strdup("");
    
    if (!(h = gg_userlist_put((gint)config_var_get(handler, "uin"), config_var_get(handler, "password"), dump, 1))) {
	print_debug("userlist put error!\n");
	return NULL;
    }
    
    h->user_data = (gchar *) 2;
        
    chan = g_io_channel_unix_new(h->fd);
    g_io_add_watch(chan, G_IO_IN | G_IO_OUT | G_IO_ERR, handle_userlist, h);
    g_free(dump);
    
    return NULL;
}


void register_userlist_menu() 
{
	GGaduMenu *umenu 	= ggadu_menu_create();
	GGaduMenu *listmenu = NULL;

	ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("Chat"),user_chat_action,NULL)   );
	ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("View History"),user_view_history_action,NULL)   );
    
	listmenu = ggadu_menu_new_item(_("Contact"),NULL,NULL);
	ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Add"),user_add_user_action,NULL)   );
	ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Remove"),user_remove_user_action,NULL)   );
	ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Info"),user_change_user_action,NULL)   );

	ggadu_menu_add_submenu(umenu, listmenu );

	ggadu_menu_print(umenu,NULL);

	signal_emit(GGadu_PLUGIN_NAME, "gui register userlist menu", umenu, "main-gui");
}


GGaduMenu *build_plugin_menu() 
{
	GGaduMenu *root	= ggadu_menu_create();
	GGaduMenu *item_gg	= ggadu_menu_add_item(root,"Gadu-Gadu",NULL,NULL);
    
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Add Contact"),user_add_user_action,NULL)   );
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Preferences"),user_preferences_action,NULL)   );
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Search for friends"),search_action,NULL)   );
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item("",NULL,NULL)   );
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Import userlist"),import_userlist_action,NULL)   );
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Export userlist"),export_userlist_action,NULL)   );
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Delete userlist"),delete_userlist_action,NULL)   );

	return root;
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 						PLUGIN STUFF
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
static void handle_sighup()
{
	if (session && session->state != GG_STATE_IDLE) {
		print_debug("disconected");
		gg_free_session(session);
		session= NULL;
	}
	
	signal(SIGHUP, handle_sighup);
}
 
GGaduPlugin *initialize_plugin(gpointer conf_ptr) 
{
	gchar *path = NULL;

	print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);

	signal(SIGHUP,handle_sighup);

#if DEBUG    
	gg_debug_level = 255;
#endif

	GGadu_PLUGIN_ACTIVATE(conf_ptr); /* wazne zeby wywolac to makro w tym miejscu */

	handler = (GGaduPlugin *)register_plugin(GGadu_PLUGIN_NAME,_("Gadu-Gadu(c) protocol"));

	config_var_add(handler, "uin",	VAR_INT);
	config_var_add(handler, "password",	VAR_STR);
	config_var_add(handler, "server",	VAR_STR);
	config_var_add(handler, "sound_msg_file", VAR_STR);
	config_var_add(handler, "sound_chat_file", VAR_STR);
	config_var_add(handler, "sound_app_file", VAR_STR);
	config_var_add(handler, "log", VAR_BOOL);
	config_var_add(handler, "autoconnect", VAR_BOOL);
	config_var_add(handler, "status", VAR_INT);
	config_var_add(handler, "reason", VAR_STR);
	config_var_add(handler, "private", VAR_BOOL);


	if (g_getenv("CONFIG_DIR"))
		this_configdir = g_build_filename(g_get_home_dir(),g_getenv("CONFIG_DIR"),"gg",NULL);
	else
		this_configdir = g_build_filename(g_get_home_dir(),".gg",NULL);

	path = g_build_filename(this_configdir,"config_test",NULL);
	
	mkdir(this_configdir, 0700);
	
	set_config_file_name((GGaduPlugin *)handler, path);

	if (!config_read(handler)) {
		g_free(path);
		path = g_build_filename(this_configdir,"config",NULL);

		set_config_file_name((GGaduPlugin *)handler, path);

		if (!config_read(handler)) 
		  	g_warning(_("Unable to read configuration file for plugin %s"),"gadu-gadu");
	}

	if (config_var_get(handler,"sound_msg_file") == NULL)
		config_var_set(handler,"sound_msg_file",g_build_filename(PACKAGE_DATA_DIR,"sounds","usr.wav",NULL));

	if (config_var_get(handler,"sound_app_file") == NULL)
		config_var_set(handler,"sound_app_file",g_build_filename(PACKAGE_DATA_DIR,"sounds","yahoo.wav",NULL));

	if (config_var_get(handler,"sound_chat_file") == NULL)
		config_var_set(handler,"sound_chat_file",g_build_filename(PACKAGE_DATA_DIR,"sounds","msg.wav",NULL));
	
	register_signal_receiver((GGaduPlugin *)handler, (signal_func_ptr)my_signal_receive);
	return handler;
}

GSList *status_init() 
{
    GSList *list = NULL;
    GGaduStatusPrototype *sp;
    
    sp = g_new0(GGaduStatusPrototype, 9);
    
    sp->status = GG_STATUS_AVAIL;
    sp->description = g_strdup(_("Available"));
    sp->image = g_strdup("online.png");
    sp->receive_only = FALSE;
    list = g_slist_append(list, sp);
    sp++;

    sp->status = GG_STATUS_AVAIL_DESCR;
    sp->description = g_strdup(_("Available with description"));
    sp->image = g_strdup("online-descr.png");
    sp->receive_only = FALSE;
    list = g_slist_append(list, sp);
    sp++;
    
    sp->status = GG_STATUS_BUSY;
    sp->description = g_strdup(_("Busy"));
    sp->image = g_strdup("away.png");
    sp->receive_only = FALSE;
    list = g_slist_append(list, sp);
    sp++;

    sp->status = GG_STATUS_BUSY_DESCR;
    sp->description = g_strdup(_("Busy with description"));
    sp->image = g_strdup("away-descr.png");
    sp->receive_only = FALSE;
    list = g_slist_append(list, sp);
    sp++;

    sp->status = GG_STATUS_INVISIBLE;
    sp->description = g_strdup(_("Invisible"));
    sp->image = g_strdup("invisible.png");
    sp->receive_only = FALSE;
    list = g_slist_append(list, sp);
    sp++;

    sp->status = GG_STATUS_INVISIBLE_DESCR;
    sp->description = g_strdup(_("Invisible with description"));
    sp->image = g_strdup("invisible-descr.png");
    sp->receive_only = FALSE;
    list = g_slist_append(list, sp);
    sp++;

    sp->status = GG_STATUS_NOT_AVAIL_DESCR;
    sp->description = g_strdup(_("Offline with description"));
    sp->image = g_strdup("offline-descr.png");
    sp->receive_only = FALSE;
    list = g_slist_append(list, sp);
    sp++;

    sp->status = GG_STATUS_NOT_AVAIL;
    sp->description = g_strdup(_("Offline"));
    sp->image = g_strdup("offline.png");
    sp->receive_only = FALSE;
    list = g_slist_append(list, sp);
    sp++;

    sp->status = GG_STATUS_BLOCKED;
    sp->description = g_strdup(_("Blocked"));
    sp->image = g_strdup("invisible.png");
    sp->receive_only = TRUE;
    list = g_slist_append(list, sp);
    sp++;

    return list;
}

void start_plugin()
{
	
	print_debug("%s : start_plugin\n",GGadu_PLUGIN_NAME);

	p = g_new0(GGaduProtocol, 1);
	p->display_name = g_strdup("Gadu-Gadu");
	p->img_filename = g_strdup("Gadu-Gadu.png");
	p->statuslist = status_init();
	p->offline_status = GG_STATUS_NOT_AVAIL;
	
	signal_emit(GGadu_PLUGIN_NAME, "gui register protocol", p, "main-gui");
	
	/** try to register menu for this plugin in GUI **/
	menu_pluginmenu = build_plugin_menu();
	
	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_pluginmenu, "main-gui");

	register_signal(handler,"change status");
	register_signal(handler,"change status descr");
	register_signal(handler,"send message");
	register_signal(handler,"add user");
	register_signal(handler,"change user");
	register_signal(handler,"update config");
	register_signal(handler,"search");
	register_signal(handler,"exit");
	register_signal(handler,"add user search");
	register_signal(handler,"get current status");
	
	load_contacts("ISO-8859-2");

	signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");

	test();

	register_userlist_menu();

	if (config_var_get(handler, "autoconnect") && !connected)
				gadu_gadu_login(
					(config_var_check(handler,"reason")) ? config_var_get(handler,"reason") : _("no reason")
					, 
					(config_var_check(handler,"status")) ? (gint)config_var_get(handler,"status") : GG_STATUS_AVAIL
				);

}

void my_signal_receive(gpointer name, gpointer signal_ptr) 
{
	GGaduSignal *signal = (GGaduSignal *)signal_ptr;

        print_debug("%s : receive signal %s\n",GGadu_PLUGIN_NAME,(gchar *)signal->name);

	if (!ggadu_strcasecmp(signal->name,"exit")) { 
	    //exit_signal_handler(); 
	    return;
	}
	
	if (!ggadu_strcasecmp(signal->name,"add user")) { 
	    GGaduContact *k = g_new0(GGaduContact,1);
	    GSList *kvlist = (GSList *)signal->data;

	    while (kvlist) 
	    {
		GGaduKeyValue *kv = (GGaduKeyValue *)kvlist->data;
		
		switch ((gint)kv->key) 
		{
		    case GGADU_ID: {
			gint i = 0;
			gchar *tmp = (gchar *)kv->value;
			
			for (i=0; i < strlen(tmp); i++)
			    if (g_ascii_isdigit( (tmp)[i] ) == FALSE) {
				signal_emit(GGadu_PLUGIN_NAME, "gui show warning",g_strdup(_("Not valid GG#")),"main-gui");
				print_debug("This is not valid uin\n");
				return;
			    }
			
			k->id = g_strdup(tmp);
			g_free(tmp);

			}
			break;

		    case GGADU_NICK:
	    		k->nick		= g_strdup((gchar *)kv->value);
			g_free(kv->value);
			break;
			
		    case GGADU_FIRST_NAME:
			k->first_name	= g_strdup((gchar *)kv->value);
			g_free(kv->value);
			break;
			
		    case GGADU_LAST_NAME:
			k->last_name	= g_strdup((gchar *)kv->value);
			g_free(kv->value);
			break;

		    case GGADU_MOBILE:
			k->mobile	= g_strdup((gchar *)kv->value);
			g_free(kv->value);
			break;
		}
		
		g_free(kv->description);
		kvlist=kvlist->next;
	    }
	    
	    // initial status for added person 
	    k->status	= GG_STATUS_NOT_AVAIL;
	    
	    g_slist_free(kvlist);

	    userlist = g_slist_append(userlist,k);
	    signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
	    save_addressbook_file(userlist);

	    if ((connected == TRUE) && (session != NULL) & (k != NULL) && (k->id != NULL))
		gg_add_notify(session,atoi(k->id));

	    return;
	}

	if (!ggadu_strcasecmp(signal->name,"add user search")) { 
	    GGaduContact *k = signal->data;

	    /* initial status for added person */
	    k->status	= GG_STATUS_NOT_AVAIL;
	    
	    userlist = g_slist_append(userlist,k);
	    signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
	    save_addressbook_file(userlist);

	    if (connected && session)
		gg_add_notify(session,atoi(k->id));
	    
	    return;
	}


	if (!ggadu_strcasecmp(signal->name,"change user")) 
	{ 
	    GGaduContact *k = g_new0(GGaduContact,1);
	    GSList *kvlist = (GSList *)signal->data;
	    GSList *ulisttmp = userlist;
	    gint i = 0;

	    while (kvlist) 
	    {
		GGaduKeyValue *kv = (GGaduKeyValue *)kvlist->data;
		
		switch ((gint)kv->key) 
		{
		    case GGADU_ID:
			k->id = g_strdup((gchar *)kv->value);
			g_free(kv->value);
			
			for (i=0; i < strlen(k->id); i++)
			    if (g_ascii_isdigit( (k->id)[i] ) == FALSE)
				return;
			    
			break;

		    case GGADU_NICK:
			k->nick		= g_strdup((gchar *)kv->value);
			g_free(kv->value);
			break;
			
		    case GGADU_FIRST_NAME:
			k->first_name	= g_strdup((gchar *)kv->value);
			g_free(kv->value);
			break;
			
		    case GGADU_LAST_NAME:
			k->last_name	= g_strdup((gchar *)kv->value);
			g_free(kv->value);
			break;

		    case GGADU_MOBILE:
			k->mobile	= g_strdup((gchar *)kv->value);
			g_free(kv->value);
			break;
		}
		g_free(kv->description);
		kvlist=kvlist->next;
	    }

	    g_slist_free(kvlist);
	    
	    /* odnajduje id do zmiany */
	    while (ulisttmp)
	    {
		GGaduContact *ktmp = (GGaduContact *)ulisttmp->data;
		
		if (!ggadu_strcasecmp(ktmp->id, k->id)) {
		    /* zmiana danych */
		    ktmp->id = k->id;
		    ktmp->first_name = k->first_name;
		    ktmp->last_name = k->last_name;
		    ktmp->nick = k->nick;
		    ktmp->mobile = k->mobile;
		    break;
		}
		ulisttmp = ulisttmp->next;
	    }

	    save_addressbook_file(userlist);
	    signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
	    return;
	}

	
	if (!ggadu_strcasecmp(signal->name,"change status")) 
	{ 
	    GGaduStatusPrototype *sp = signal->data;
	    
	    if (!connected) {
	    
		connect_count = 0;
		if ((sp != NULL) && (sp->status != GG_STATUS_NOT_AVAIL) &&
		    (sp->status != GG_STATUS_NOT_AVAIL_DESCR)) {
		    gadu_gadu_login(NULL,sp->status);
		}

		
	    } else if (connected && sp) {
	    
		if (sp->status != GG_STATUS_NOT_AVAIL) {
		
		    if (sp->status == GG_STATUS_AVAIL_DESCR ||
			sp->status == GG_STATUS_BUSY_DESCR ||
			sp->status == GG_STATUS_NOT_AVAIL_DESCR ||
			sp->status == GG_STATUS_INVISIBLE_DESCR) 
		    {
			GGaduDialog *d = ggadu_dialog_new();
		        GGaduKeyValue *kv = NULL;

			d->title = g_strdup(_("Enter status description"));
			d->callback_signal = g_strdup("change status descr");
    
		        kv = g_new0(GGaduKeyValue,1);
			kv->description	= g_strdup(_("Description:"));
			kv->type	= VAR_STR;
			to_utf8("ISO-8859-2",config_var_get(handler,"reason"),kv->value);
//			kv->value	= config_var_get(handler,"reason");

			d->optlist = g_slist_append(d->optlist, kv);
			d->user_data = sp;
			signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");
			
		    } else {
			gint _status = sp->status;
			if (config_var_get(handler, "private")) 
			    _status |= GG_STATUS_FRIENDS_MASK;

			if (gg_change_status(session, _status) == -1) {
			    signal_emit(GGadu_PLUGIN_NAME, "gui show warning",g_strdup(_("Unable to change status")),"main-gui");
			    print_debug("zjebka podczas change_status %d\n",sp->status);
			} else {
			    signal_emit(GGadu_PLUGIN_NAME,"gui status changed",(gpointer)sp->status,"main-gui");
			}
		    }
		    
		} else {

		    /* nie czekamy az serwer powie good bye tylko sami sie rozlaczamy*/
		    ggadu_gadu_gadu_disconnect();
		}
	    }
	    	    
	    return;
	}

	if (!ggadu_strcasecmp(signal->name,"change status descr")) 
	{ 
	    GGaduDialog *d = signal->data;
	    GGaduStatusPrototype *sp = d->user_data;
	    
	    if (d->response == GGADU_OK) {
	        if (connected && sp) {
	
	    	    GGaduKeyValue *kv = NULL;
		    if (d->optlist) {
			gchar *desc_utf = NULL, *desc_cp = NULL;
			gint _status = sp->status;
		    
			if (config_var_get(handler, "private")) 
			    _status |= GG_STATUS_FRIENDS_MASK;

			kv = (GGaduKeyValue *)d->optlist->data;
		    
			print_debug(" %d %d\n ",GG_STATUS_INVISIBLE_DESCR,sp->status);
		    
			desc_utf = kv->value;
			from_utf8("CP1250",desc_utf,desc_cp);
			config_var_set(handler, "reason", desc_cp);
			if (!gg_change_status_descr(session, _status, desc_cp))
			    signal_emit(GGadu_PLUGIN_NAME,"gui status changed",(gpointer)sp->status,"main-gui");
			g_free(desc_cp);
		    }
		
		    if (sp->status == GG_STATUS_NOT_AVAIL_DESCR) {
			ggadu_gadu_gadu_disconnect();
		    }

		}
	    }
	    GGaduDialog_free(d);
	    return;
	}
	

	if (!ggadu_strcasecmp(signal->name,"update config")) 
	{ 
	    GGaduDialog *d = signal->data;
	    GSList *tmplist = d->optlist;
	    
	    if (d->response == GGADU_OK) {
		while (tmplist) 
		{
		    GGaduKeyValue *kv = (GGaduKeyValue *)tmplist->data;
		
		    switch (kv->key) 
		    {
			case GGADU_GADU_GADU_CONFIG_ID:
				    print_debug("changing var setting uin to %d\n", kv->value);
				    config_var_set(handler, "uin", kv->value);
				    break;
			case GGADU_GADU_GADU_CONFIG_PASSWORD:
				    print_debug("changing var setting password to %s\n", kv->value);
				    config_var_set(handler, "password", kv->value);
				    break;
			case GGADU_GADU_GADU_CONFIG_SERVER:
				    print_debug("changing var setting server to %s\n", kv->value);
				    config_var_set(handler, "server", kv->value);
				    break;
			case GGADU_GADU_GADU_CONFIG_SOUND_CHAT_FILE:
				    print_debug("changing var setting sound_chat_file to %s\n", kv->value);
				    config_var_set(handler, "sound_chat_file", kv->value);
				    break;
			case GGADU_GADU_GADU_CONFIG_SOUND_MSG_FILE:
				    print_debug("changing var setting sound_msg_file to %s\n", kv->value);
				    config_var_set(handler, "sound_msg_file", kv->value);
				    break;
			case GGADU_GADU_GADU_CONFIG_SOUND_APP_FILE:
				    print_debug("changing var setting sound_app_file to %s\n", kv->value);
				    config_var_set(handler, "sound_app_file", kv->value);
				    break;
			case GGADU_GADU_GADU_CONFIG_HISTORY:
				    print_debug("changing var setting log to %d\n", kv->value);
				    config_var_set(handler, "log", kv->value);
				    break;
			case GGADU_GADU_GADU_CONFIG_AUTOCONNECT:
				    print_debug("changing var setting autoconnect to %d\n", kv->value);
				    config_var_set(handler, "autoconnect", kv->value);
				    break;
			case GGADU_GADU_GADU_CONFIG_AUTOCONNECT_STATUS: {
						/* change string description to int value depending on current locales so it cannot be hardcoded eh.. */
						GSList *statuslist_tmp = p->statuslist;
						gint val = -1;
						
						while (statuslist_tmp) {
								GGaduStatusPrototype *sp = (GGaduStatusPrototype *)statuslist_tmp->data;
								if (!g_strcasecmp(sp->description,kv->value)) {
										val = sp->status;
								}
								statuslist_tmp = statuslist_tmp->next;
						}
						
				    print_debug("changing var setting status to %d\n", val);
				    config_var_set(handler, "status", (gpointer)val);
						}
				    break;
			case GGADU_GADU_GADU_CONFIG_REASON:
				    {
				    gchar *utf = NULL;
				    print_debug("changing derault reason %s\n", kv->value);
				    from_utf8("ISO-8859-2",kv->value,utf);
				    config_var_set(handler, "reason", utf);
				    }
				    break;
			case GGADU_GADU_GADU_CONFIG_FRIENDS_MASK:
				    print_debug("changing var setting private to %d\n", kv->value);
				    config_var_set(handler, "private", kv->value);
				    break;
		    }
		    tmplist = tmplist->next;
		}
		config_save(handler);
	    }
	    GGaduDialog_free(d);
	    return;
	}
	
	if (!ggadu_strcasecmp(signal->name,"send message")) 
	{ 
	    GGaduMsg *msg = signal->data;

	    if (connected && msg) {
		gchar *message = NULL;

		from_utf8("CP1250",msg->message,message);
		message = insert_cr(message);
		    
		print_debug("%s\n",message);
			
		msg->time = g_strtod(get_timestamp(0),NULL);
		
		if ((msg->class == GGADU_CLASS_CONFERENCE) && (msg->recipients != NULL)) {
		    gint   i = 0;
		    uin_t  *recipients = g_malloc( g_slist_length(msg->recipients) * sizeof(uin_t) );
		    GSList *tmp = msg->recipients;
		    
		    while (tmp)
		    {
			if (tmp->data != NULL) /* ? */
			    recipients[i++] = atoi( (gchar *)tmp->data );
			tmp = tmp->next;
		    }
		    
		    if (gg_send_message_confer(session, GG_CLASS_CHAT, g_slist_length(msg->recipients), recipients, message) == -1) {
			signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Unable send message")),"main-gui");
			print_debug("zjebka podczas send message %s\n",msg->id);
		    } else if (config_var_get(handler, "log")) {
			GSList *tmp = msg->recipients;

			while (tmp)
			{
			    gchar *line = g_strdup_printf(_(":: Me (%s)::\n%s\n\n"),get_timestamp(0),msg->message);
			    ggadu_gg_save_history((gchar *)tmp->data,line);
			    g_free(line);
			    tmp = tmp->next;
			}

		    }

		} else {
		    gint class = GG_CLASS_MSG;
			
		    if (msg->class == GGADU_CLASS_CHAT)
			class = GG_CLASS_CHAT;
			
		    if ((msg->id == NULL) || (gg_send_message(session, class, atoi(msg->id), message) == -1)) {
			signal_emit(GGadu_PLUGIN_NAME, "gui show warning",g_strdup(_("Unable send message")),"main-gui");
			print_debug("zjebka podczas send message %s\n",msg->id);
		    } else if (config_var_get(handler, "log")) {
			gchar *line = g_strdup_printf(_(":: Me (%s) ::\n%s\n\n"),get_timestamp(0),msg->message);
			ggadu_gg_save_history(msg->id,line);
			g_free(line);
		    }
		}
		    
		g_free(message);
		/* i wiele wiele innych */
	    }
	}	

        if (!ggadu_strcasecmp(signal->name,"search")) 
        {	 
            GGaduDialog *d = signal->data;
    	    GSList *tmplist = d->optlist;
	    gg_pubdir50_t req;
	    
	    if ((d->response == GGADU_OK) || (d->response == GGADU_NONE))  {

		if (!(req = gg_pubdir50_new(GG_PUBDIR50_SEARCH))) {
		    GGaduDialog_free(d);
	    	    return;
		}
		
		while (tmplist) 
		{
    		    GGaduKeyValue *kv = (GGaduKeyValue *)tmplist->data;
	
	    	    switch (kv->key) 
	    	    {
			case GGADU_SEARCH_FIRSTNAME:
		    	    if (kv->value && *(gchar*)kv->value) {
				gchar *first_name;
				to_cp("UTF-8", kv->value, first_name);
				gg_pubdir50_add(req, GG_PUBDIR50_FIRSTNAME, first_name);
				g_free(first_name);
			    }
			    break;
			case GGADU_SEARCH_LASTNAME:
		    	    if (kv->value && *(gchar*)kv->value) {
				gchar *last_name;
				to_cp("UTF-8", kv->value, last_name);
				gg_pubdir50_add(req, GG_PUBDIR50_LASTNAME, last_name);
				g_free(last_name);
			    }
			    break;
			case GGADU_SEARCH_NICKNAME:
		    	    if (kv->value && *(gchar*)kv->value) {
				gchar *nick_name;
				to_cp("UTF-8", kv->value, nick_name);
				gg_pubdir50_add(req, GG_PUBDIR50_NICKNAME, nick_name);
				g_free(nick_name);
			    }
			    break;
			case GGADU_SEARCH_CITY:
		    	    if (kv->value && *(gchar*)kv->value) {
				gchar *city;
				to_cp("UTF-8", kv->value, city);
				gg_pubdir50_add(req, GG_PUBDIR50_CITY, city);
				g_free(city);
			    }
			    break;
			case GGADU_SEARCH_ACTIVE:
			    if (kv->value)
			        gg_pubdir50_add(req, GG_PUBDIR50_ACTIVE, GG_PUBDIR50_ACTIVE_TRUE);
			    break;
			case GGADU_SEARCH_ID:
			    {
    				if (kv->value) 
			    	    gg_pubdir50_add(req, GG_PUBDIR50_UIN, kv->value);
			    }
			    break;
		        }
			tmplist = tmplist->next;
		    }
		    
		    if (gg_pubdir50(session, req) == -1) {
			signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Error! Cannot perform search!")),"main-gui");
		    }
		
		    gg_pubdir50_free(req);
		}
		GGaduDialog_free(d);

	        return;
	    }

        if (!ggadu_strcasecmp(signal->name,"get current status")) {
	    if (session) 
		signal->data_return = (gpointer) ((session->status & GG_STATUS_FRIENDS_MASK) ? session->status ^ GG_STATUS_FRIENDS_MASK : session->status);
	    else
		signal->data_return = (gpointer) GG_STATUS_NOT_AVAIL;
	}
}

void destroy_plugin() 
{
    config_save(handler);
    print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
    if (menu_pluginmenu)
    {
      signal_emit (GGadu_PLUGIN_NAME, "gui unregister menu", menu_pluginmenu, "main-gui");
      ggadu_menu_free (menu_pluginmenu);
    }
    signal_emit (GGadu_PLUGIN_NAME, "gui unregister userlist menu", NULL, "main-gui");
    signal_emit (GGadu_PLUGIN_NAME, "gui unregister protocol", p, "main-gui");
}


void load_contacts(gchar *encoding)
{
	FILE *fp;
	GGaduContact *k = NULL;
	gchar *line;
	gchar *path;
		
	path = g_build_filename(this_configdir, "userlist", NULL);

	fp = fopen(path, "r");

	if (!fp) {
	    g_free(path);
	    g_warning(_("I still cannot open contacts files! Exiting..."));
	    return;
	}
	
	g_free(path);
	
	line = g_malloc0(1024);

	while (fgets(line, 1023, fp)) 
	{
		gchar *buf = NULL;
		gchar **l;
		gchar *first_name, *last_name, *nick, *comment, *mobile, *group, *uin;
		
		if (line[0] == '#' || !strcmp(g_strstrip(line), ""))
			continue;

		to_utf8(encoding,line,buf);
		
		l = g_strsplit(buf, ";", 11);
		if (!l[0]) /* ZONK */ 
		    continue;
		first_name = l[0];
		last_name = l[1];
		nick = l[2];
		mobile = l[4];
		group = l[5];
		uin = l[6];
		comment = l[7];

		if ((!uin || !*uin) && (!mobile || !*mobile))
			continue;

		k = g_new0(GGaduContact, 1);
		
		print_debug("%s\n", uin);    
		k->id = uin ? g_strdup(uin) : g_strdup("");
		k->first_name = g_strdup(first_name);
		k->last_name = g_strdup(last_name);
		k->nick = (strlen(nick) == 0) ? g_strconcat(first_name," ",last_name,NULL) : g_strdup(nick);
		k->comment = g_strdup(comment);
		k->mobile = g_strdup(mobile);
		k->group = g_strdup(group);
		k->status = GG_STATUS_NOT_AVAIL;

		g_strfreev(l);

		userlist = g_slist_append(userlist, k);
	}

	g_free(line);
	fclose(fp);
}

void save_addressbook_file(gpointer userlist) 
{
	GIOChannel *ch = NULL;
	gchar *path = NULL;
    
	path = g_build_filename(this_configdir, "userlist", NULL);
	print_debug("path is %s\n", path);
	ch = g_io_channel_new_file(path,"w",NULL);
    
	if (ch) {	
		gchar *temp = userlist_dump(userlist);
		if (g_io_channel_set_encoding(ch,"ISO-8859-2",NULL) != G_IO_STATUS_ERROR) {
			if (temp != NULL)
				g_io_channel_write_chars(ch,temp,-1,NULL,NULL);	    
		}
		g_free(temp);
		g_io_channel_shutdown(ch,TRUE,NULL);
	}

	g_free(path);
}
