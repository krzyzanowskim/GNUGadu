/* $Id: icq_plugin.c,v 1.1 2003/04/09 16:07:22 thrulliq Exp $ */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "icq.h"
#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "support.h"
#include "dialog.h"
#include "menu.h"

#include "icq_plugin.h"

static GGaduPlugin *handler;
static GGaduProtocol *p;
static GGaduMenu *menu;

gboolean connected = FALSE;

gchar *this_configdir = NULL;

static GSList *userlist = NULL;

icq_Link *icqlink = NULL;

GIOChannel *chan = NULL;
gint watch = 0;
gint icq_timeout = 0;
gint status = 0;

GGadu_PLUGIN_INIT("icq", GGADU_PLUGIN_TYPE_PROTOCOL);


void login(gint status)
{
    print_debug("login\n");
    if (icq_Connect(icqlink, "icq.mirabilis.com", 4000) <= 0){
    	print_debug("Co jest?\n");
    }
    icq_Login(icqlink, status);
}

static void logged_in(icq_Link *icqlink)
{
    print_debug("Logged in\n");
    connected = TRUE;
    signal_emit(GGadu_PLUGIN_NAME, "gui status changed", (gpointer)status, "main-gui");
    icq_ChangeStatus(icqlink, status);
    icq_SendContactList(icqlink);
}

static void wrong_password(icq_Link *icqlink)
{
    print_debug("wrong password\n");
    signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Wrong password!")), "main-gui");
}

static void wrong_uin(icq_Link *icqlink)
{
    print_debug("wrong uin\n");
    signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Wrong UIN!")), "main-gui");
}

static void disconnected(icq_Link *icqlink)
{
    print_debug("disconnected\n");
    signal_emit(GGadu_PLUGIN_NAME, "gui disconnected", NULL, "main-gui");
    g_source_remove(watch);
    watch = 0;
    if (connected) {
        connected = FALSE;
	login(status);
    }
}

static void log_event(icq_Link *icqlink, gint time, guchar level, gchar *msg)
{
    print_debug("Log event: %s\n", msg);
}

gboolean icq_timeout_callback(gpointer user_data)
{
    print_debug("timeout!\n");
    icq_HandleTimeout();
    icq_timeout = 0;
    return FALSE;
}

static void set_timeout(glong interval)
{
    print_debug("Timeout: %ld\n", interval);
    if (interval > 0) {
        if (icq_timeout)
    	    g_source_remove(icq_timeout);
        icq_timeout = g_timeout_add(interval * 1000, icq_timeout_callback, NULL);
    } else if (icq_timeout) {
        g_source_remove(icq_timeout);
        icq_timeout = 0;
    }
}

static void request_notify(icq_Link *icqlink, guint id, gint type, gint len, gpointer data)
{
    print_debug("request notify %d\n", id);
    if (type == ICQ_NOTIFY_FAILED) {
        signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Communication error occured while connecting to ICQ server")), "main-gui");
    }
}

static void user_online(icq_Link *icqlink, gulong uin, gulong status, gulong ip, gushort port, gulong real_ip, guchar tcp_flag)
{
    print_debug("user_online: %d\n", uin);
}

static void user_offline(icq_Link *icqlink, gulong uin)
{
    print_debug("user offline: %d\n", uin);
}

static void recv_message(icq_Link *icqlink, gulong uin, guchar hour, guchar minute, guchar day, guchar month, gushort year, gchar *message)
{
    GGaduMsg *msg;
//    struct tm t;
    
    print_debug("recv message: %d %s\n", uin, message);
    msg = g_new0(GGaduMsg,1);
    msg->id = g_strdup_printf("%ld", uin);
    to_utf8("ISO-8859-1", message, msg->message);
    msg->class = GGADU_CLASS_CHAT;
    
/*
    memset(&t, 0, sizeof(struct tm));
    
    t.tm_min 	= minute;
    t.tm_hour 	= hour;
    t.tm_mday	= day;
    t.tm_mon	= month;
    t.tm_year 	= year;
    
    msg->time = mktime(&t);
*/    
    signal_emit(GGadu_PLUGIN_NAME, "gui msg receive", msg, "main-gui");
}

static void recv_url(icq_Link *icqlink, gulong uin, guchar hour, guchar minute, guchar day, guchar month, gushort year, gchar *url, gchar *descr)
{
    print_debug("recv url: %d %s %s\n", uin, url, descr);
}

static void recv_mail_express(icq_Link *icqlink, guchar hour, guchar minute, guchar day, guchar month, gushort year, gchar *nick, gchar *email, gchar *msg)
{
    print_debug("recv mail express: %s %s %s\n", nick, email, msg);
}

static void recv_added(icq_Link *icqlink, gulong uin, guchar hour, guchar minute, guchar day, guchar month, gushort year, gchar *nick, gchar *first, gchar *last, gchar *email)
{
    print_debug("recv added: %d %s\n", uin, nick);
}

static void recv_web_pager(icq_Link *icqlink, guchar hour, guchar minute, guchar day, guchar month, gushort year, gchar *nick, gchar *email, gchar *msg)
{
    print_debug("recv web pager: %s %s %s\n", email, nick, msg);
}

static void user_status_update(icq_Link *icqlink, gulong uin, gulong status)
{
    print_debug("status update: %d\n", uin);
}

static void info_reply(icq_Link *icqlink, gulong uin, gchar *nick, gchar *first, gchar *last, gchar *email, gchar auth)
{
    print_debug("info reply: %d %s %s\n", uin, nick, email);
}

static void ext_info_reply(icq_Link *icqlink, gulong uin, gchar *city, gushort countr_code, gchar countr_stat, gchar *state, gushort age, gchar gender, gchar *phone, gchar *hp, gchar *about)
{
    print_debug("info reply: %d %s %s\n", uin, state, city);
}

static void recv_chat_req(icq_Link *icqlink, gulong uin, guchar hour, guchar minute, guchar day, guchar month, gushort year, gchar *descr, gulong seq)
{
    print_debug("recv chat req: %d %s\n", uin, descr);
}

static void recv_file_req(icq_Link *icqlink, gulong uin, guchar hour, guchar minute, guchar day, guchar month, gushort year, gchar *descr, gchar *filename, gulong filesize, gulong seq)
{
    print_debug("recv file req: %d %s\n", uin, filename);
}

static void recv_auth_req(icq_Link *icqlink, gulong uin, guchar hour, guchar minute, guchar day, guchar month, gushort year, gchar *nick, gchar *first, gchar *last, gchar *email, gchar *reason)
{
    print_debug("recv auth req: %d %s\n", uin, nick);
}

gboolean test_chan(GIOChannel *source, GIOCondition cond, gpointer data)
{
    print_debug("icq socket ready\n");
    if (cond & G_IO_IN)
    	icq_HandleReadySocket(g_io_channel_unix_get_fd (source), ICQ_SOCKET_READ);
    if (cond & G_IO_OUT)
    	icq_HandleReadySocket(g_io_channel_unix_get_fd (source), ICQ_SOCKET_WRITE);
    return TRUE;
}

void icq_sock_notify(int socket, int type, int status)
{
    if (status) {
	gint cond;
	if (type == ICQ_SOCKET_READ)
	    cond = G_IO_IN;
	else
	    cond = G_IO_OUT;
	
	if ((chan = g_io_channel_unix_new(socket)) != NULL)
	  if ((watch = g_io_add_watch(chan, cond, test_chan, NULL)) == 0)
		g_io_channel_unref(chan);
    } else {
	g_source_remove(watch);
	g_io_channel_unref(chan);
	watch = 0;
    }
}


/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 						PLUGIN STUFF
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

void my_signal_receive(gpointer name, gpointer signal_ptr) 
{
	GGaduSignal *signal = (GGaduSignal *)signal_ptr;
        print_debug("%s : receive signal %s\n",GGadu_PLUGIN_NAME,(gchar *)signal->name);
        
	if (!ggadu_strcasecmp(signal->name,"add user")) { 
	    GGaduContact *k = g_new0(GGaduContact,1);
	    GSList *kvlist = (GSList *)signal->data;

	    while (kvlist) 
	    {
		GGaduKeyValue *kv = (GGaduKeyValue *)kvlist->data;
		
		switch ((gint)kv->key) 
		{
		    case ICQ_UIN: {
			gint i = 0;
			gchar *tmp = (gchar *)kv->value;
			
			for (i=0; i < strlen(tmp); i++)
			    if (g_ascii_isdigit( (tmp)[i] ) == FALSE) {
				signal_emit(GGadu_PLUGIN_NAME, "gui show warning",g_strdup(_("Not valid UIN")),"main-gui");
				print_debug("This is not valid uin\n");
				return;
			    }
			
			k->id = g_strdup(tmp);
			g_free(tmp);

			}
			break;

		    case ICQ_NICK:
	    		k->nick		= g_strdup((gchar *)kv->value);
			g_free(kv->value);
			break;
		}
		
		g_free(kv->description);
		kvlist=kvlist->next;
	    }
	    
	    // initial status for added person 
	    k->status	= STATUS_OFFLINE;
	    
	    g_slist_free(kvlist);

	    userlist = g_slist_append(userlist,k);
	    signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
	    save_addressbook_file(userlist);
	    
	    icq_ContactAdd(icqlink, atoi(k->id));

	    if (connected) 
		icq_SendContactList(icqlink);
		
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
		    case ICQ_UIN:
			k->id = g_strdup((gchar *)kv->value);
			g_free(kv->value);
			
			for (i=0; i < strlen(k->id); i++)
			    if (g_ascii_isdigit( (k->id)[i] ) == FALSE)
				return;
			    
			break;

		    case ICQ_NICK:
			k->nick		= g_strdup((gchar *)kv->value);
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
      	if (!ggadu_strcasecmp(signal->name,"change status")) { 
	    GGaduStatusPrototype *sp = signal->data;
	    
	    if (sp->status == STATUS_OFFLINE) {
		if (connected) {
		    g_source_remove(watch);
		    g_io_channel_shutdown(chan, FALSE, NULL);
		    watch = 0;
		    signal_emit(GGadu_PLUGIN_NAME, "gui disconnected", NULL, "main-gui");
		    connected = FALSE;
		    icq_Logout(icqlink);
		    icq_Disconnect(icqlink);
		}
	    } else {
		if (!connected) {
		    login(sp->status);
		} else {
		    icq_ChangeStatus(icqlink, sp->status);
		    signal_emit(GGadu_PLUGIN_NAME, "gui status changed", (gpointer)sp->status, "main-gui");
		}
	    }
	    status = sp->status;
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
			case ICQ_UIN:
				    print_debug("changing var setting uin to %s\n", kv->value);
				    config_var_set(handler, "uin", kv->value);
				    break;
			case ICQ_PASSWORD:
				    print_debug("changing var setting password to %s\n", kv->value);
				    config_var_set(handler, "password", kv->value);
				    break;
			case ICQ_NICK:
				    print_debug("changing var setting nick to %d\n", kv->value);
				    config_var_set(handler, "nick", kv->value);
				    break;
		    }
		    tmplist = tmplist->next;
		}
	        config_save(handler);
	    }
	    GGaduDialog_free(d);
	    return;
	}

	if (!ggadu_strcasecmp(signal->name, "get current status")) {
		signal->data_return = (gpointer) icqlink->icq_Status;
		return;
	}

	if (!ggadu_strcasecmp(signal->name,"send message")) 
	{ 
	    GGaduMsg *msg = signal->data;

	    if (connected && msg) {
		gchar *message = NULL;
		gint uin = atoi(msg->id);
		from_utf8("ISO-8859-1", msg->message, message);
		print_debug("message to %d\n", uin);
		icq_SendMessage(icqlink, uin, message, ICQ_SEND_DIRECT);
		g_free(message);
    	    }
	}	
}

GGaduPlugin *initialize_plugin(gpointer conf_ptr) 
{
    print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);

    GGadu_PLUGIN_ACTIVATE(conf_ptr); /* wazne zeby wywolac to makro w tym miejscu */

    handler = (GGaduPlugin *)register_plugin(GGadu_PLUGIN_NAME,"ICQ protocol");

    register_signal_receiver((GGaduPlugin *)handler, (signal_func_ptr)my_signal_receive);

    if (g_getenv("CONFIG_DIR"))
	this_configdir = g_build_filename(g_get_home_dir(),g_getenv("CONFIG_DIR"),"gg2",NULL);
     else
	this_configdir = g_build_filename(g_get_home_dir(),".gg2",NULL);

    mkdir(this_configdir, 0700);
    
    set_config_file_name((GGaduPlugin *)handler,g_build_filename(this_configdir,"icq",NULL));
    
    config_var_add(handler, "uin",    VAR_INT);
    config_var_add(handler, "password", VAR_STR);
    config_var_add(handler, "nick", VAR_STR);
    
    config_read(handler);
	
    register_signal(handler,"update config");
    register_signal(handler,"change status");
    register_signal(handler,"get current status");
    register_signal(handler,"send message");
    register_signal(handler,"add user");
    register_signal(handler,"change user");
    
    return handler;
}

GSList *status_init() 
{
    GSList *list = NULL;
    GGaduStatusPrototype *sp;
    
    sp = g_new0(GGaduStatusPrototype, 9);

    sp->status = STATUS_ONLINE;
    sp->description = g_strdup(_("Online"));
    sp->image = g_strdup("icq-online.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = STATUS_AWAY;
    sp->description = g_strdup(_("Away"));
    sp->image = g_strdup("icq-away.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = STATUS_DND;
    sp->description = g_strdup(_("Do not disturb"));
    sp->image = g_strdup("icq-dnd.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = STATUS_NA;
    sp->description = g_strdup(_("Not available"));
    sp->image = g_strdup("icq-na.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = STATUS_OCCUPIED;
    sp->description = g_strdup(_("Occupied"));
    sp->image = g_strdup("icq-occupied.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = STATUS_INVISIBLE;
    sp->description = g_strdup(_("Invisible"));
    sp->image = g_strdup("icq-invisible.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = STATUS_FREE_CHAT;
    sp->description = g_strdup(_("Free chat"));
    sp->image = g_strdup("icq-freechat.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = (gint) STATUS_OFFLINE;
    sp->description = g_strdup(_("Unavailable"));
    sp->image = g_strdup("icq-offline.png");
    list = g_slist_append(list, sp);
    
    print_debug("STATUS_OFFLINE %d\n", (gpointer)sp->status);
    return list;
}

gpointer user_preferences_action(gpointer user_data) 
{
    GGaduDialog *d = ggadu_dialog_new();
    
    ggadu_dialog_set_title(d, _("ICQ plugin configuration"));
    ggadu_dialog_callback_signal(d, "update config");
    ggadu_dialog_set_type(d, GGADU_DIALOG_CONFIG);

    ggadu_dialog_add_entry(&(d->optlist), ICQ_UIN, _("Uin"), VAR_INT, config_var_get(handler, "uin"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), ICQ_PASSWORD, _("Password"), VAR_STR, config_var_get(handler, "password"), VAR_FLAG_PASSWORD);
    ggadu_dialog_add_entry(&(d->optlist), ICQ_NICK, _("Nick"), VAR_STR, config_var_get(handler, "nick"), VAR_FLAG_NONE);
    
    signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");

    return NULL;
}

void init_icq()
{
    gchar *password = (gchar *)config_var_get(handler, "password");
    gchar *nick = (gchar *)config_var_get(handler, "nick");
    
    icqlink = icq_ICQLINKNew((gint)config_var_get(handler, "uin"), (password) ? password : "", (nick) ? nick : "", TRUE);

    /* setting callbacks */
    
    icqlink->icq_Log = log_event;
    icqlink->icq_Logged = logged_in;
    icqlink->icq_WrongPassword = wrong_password;
    icqlink->icq_InvalidUIN = wrong_uin;
    icqlink->icq_Disconnected = disconnected;
    icqlink->icq_RequestNotify = request_notify;
    icqlink->icq_UserOnline = user_online;
    icqlink->icq_UserOffline = user_offline;
    icqlink->icq_RecvMessage = recv_message;
    icqlink->icq_RecvAdded = recv_added;
    icqlink->icq_RecvURL = recv_url;
    icqlink->icq_RecvWebPager = recv_web_pager;
    icqlink->icq_RecvMailExpress = recv_mail_express;
    icqlink->icq_UserStatusUpdate = user_status_update;
    icqlink->icq_InfoReply = info_reply;
    icqlink->icq_ExtInfoReply = ext_info_reply;
    icqlink->icq_RecvAuthReq = recv_auth_req;
    icqlink->icq_RecvChatReq = recv_chat_req;
    icqlink->icq_RecvFileReq = recv_file_req;
    
    icq_SocketNotify = icq_sock_notify;
    icq_SetTimeout = set_timeout;    
    icq_ContactClear(icqlink);    
}


gpointer user_chat_action(gpointer user_data)
{
    GSList	*users = (GSList *)user_data;
    GGaduMsg    *msg   = g_new0(GGaduMsg,1);

    if (!user_data) return NULL;
    
    GGaduContact *k    = (GGaduContact *)users->data;
    msg->class = GGADU_CLASS_CHAT;
    msg->id = k->id;

    msg->message = NULL;
    signal_emit(GGadu_PLUGIN_NAME, "gui msg receive",msg,"main-gui");
    return NULL;
}

gpointer user_remove_user_action(gpointer user_data)
{
	GSList *users = (GSList *)user_data;

	while (users) 
	{
		GGaduContact *k = (GGaduContact *)users->data;
		userlist = g_slist_remove(userlist,k);

		icq_ContactRemove(icqlink, atoi(k->id));
		
		GGaduContact_free(k);
		users = users->next;
	}

	if ((user_data) && (userlist)) {
		signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
		save_addressbook_file(userlist);
	}
	
	if (connected)
	    icq_SendContactList(icqlink);
	return NULL;
}

gpointer user_change_user_action(gpointer user_data)
{
    GSList	 *optlist  	= NULL;
    GSList 	 *users		= (GSList *)user_data;
    GGaduContact *k		= (GGaduContact *)users->data;

    ggadu_dialog_add_entry(&optlist, ICQ_UIN, "UIN", VAR_STR, k->id, VAR_FLAG_INSENSITIVE); 
    ggadu_dialog_add_entry(&optlist, ICQ_NICK, _("Nick"), VAR_STR, k->nick, VAR_FLAG_SENSITIVE); 

    /* wywoluje okienko zmiany usera */
    signal_emit(GGadu_PLUGIN_NAME, "gui change user window", optlist, "main-gui");

    return NULL;
    
}

gpointer user_add_user_action(gpointer user_data)
{
    GSList *optlist   = NULL;

    ggadu_dialog_add_entry(&optlist, ICQ_UIN, "UIN", VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&optlist, ICQ_NICK, _("Nick"), VAR_STR, NULL, VAR_FLAG_NONE);

    /* wywoluje okienko dodawania usera */
    signal_emit(GGadu_PLUGIN_NAME, "gui add user window", optlist, "main-gui");
    
    return NULL;
}

void register_userlist_menu() 
{
	GGaduMenu *umenu 	= ggadu_menu_create();
	GGaduMenu *listmenu = NULL;

	ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("Chat"),user_chat_action,NULL)   );
    
	listmenu = ggadu_menu_new_item(_("Contact"),NULL,NULL);
	ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Add"),user_add_user_action,NULL)   );
	ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Remove"),user_remove_user_action,NULL)   );
	ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Info"),user_change_user_action,NULL)   );

	ggadu_menu_add_submenu(umenu, listmenu );

	ggadu_menu_print(umenu,NULL);

	signal_emit(GGadu_PLUGIN_NAME, "gui register userlist menu", umenu, "main-gui");
}

GGaduMenu *build_icq_menu() 
{
    GGaduMenu *root = ggadu_menu_create();
    GGaduMenu *item = ggadu_menu_add_item(root,"_ICQ",NULL,NULL);
    
    ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Preferences"),user_preferences_action,NULL));
    ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Add Contact"),user_add_user_action,NULL));
   
    return root;
}


void start_plugin()
{
    init_icq();
    
    p = g_new0(GGaduProtocol, 1);
    p->display_name = g_strdup("ICQ");
    p->img_filename = g_strdup("icq.png");
    p->statuslist = status_init();
    p->offline_status = STATUS_OFFLINE;
	
    signal_emit(GGadu_PLUGIN_NAME, "gui register protocol", p, "main-gui");
	
    menu = build_icq_menu();
    signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu, "main-gui");
	
    load_contacts("ISO-8859-2");

    signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");

    register_userlist_menu();
}

void destroy_plugin() {
    print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
    if (menu) {
      signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", menu, "main-gui");
      ggadu_menu_free (menu);
    }
//    signal_emit(GGadu_PLUGIN_NAME, "gui unregister userlist menu", NULL, "main-gui");
    signal_emit(GGadu_PLUGIN_NAME, "gui unregister protocol", p, "main-gui");

    icq_Logout(icqlink);
    icq_Disconnect(icqlink);
    icq_LinkDelete(icqlink);
    g_free(this_configdir);
}

/* contacts file i/o */

gchar *userlist_dump(GSList *list)
{
	gchar *dump = NULL;
	GSList *tmplist = list;
    
	while (tmplist)
	{
		gchar	 *line		= NULL;
		GGaduContact *k		= (GGaduContact *)tmplist->data;    
	    
		line = g_strdup_printf("%s;%s\n",k->id,k->nick);
	    
		if (!dump) 
			dump = g_strdup(line);
		else {
			gchar *tmp = dump;
			dump = g_strjoin(NULL, dump, line, NULL);
			g_free(tmp);
		}
	    
		g_free(line);
		tmplist = tmplist->next;
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

void load_contacts(gchar *encoding)
{
	FILE *fp;
	GGaduContact *k = NULL;
	gchar *line;
	gchar *path;
		
	path = g_build_filename(this_configdir, "icq_userlist", NULL);

	fp = fopen(path, "r");

	g_free(path);

	if (!fp) {
	    g_warning(_("I still cannot open contacts files! Exiting..."));
	    return;
	}
	
	
	line = g_malloc0(1024);

	while (fgets(line, 1023, fp)) 
	{
		gchar *buf = NULL;
		gchar **l;
		gchar *uin, *nick;
		
		if (line[0] == '#' || !line[0])
			continue;

		to_utf8(encoding,line,buf);
		
		l = g_strsplit(buf, ";", 3);
		if (!l[0]) /* ZONK */ 
		    continue;
		uin = l[0];
		nick = l[1];

		if ((!uin || !*uin))
			continue;

		k = g_new0(GGaduContact, 1);
		
		print_debug("%s\n", uin);    
		k->id = g_strdup(uin);
		k->nick = (!nick || !*nick) ? g_strdup(uin) : g_strdup(nick);
		k->status = STATUS_OFFLINE;

		g_strfreev(l);

		userlist = g_slist_append(userlist, k);
		icq_ContactAdd(icqlink, atoi(k->id));
	}

	g_free(line);
	fclose(fp);
}

void save_addressbook_file(gpointer userlist) 
{
	GIOChannel *ch = NULL;
	gchar *path = NULL;
    
	path = g_build_filename(this_configdir, "icq_userlist", NULL);
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
		g_io_channel_unref(ch);
	}

	g_free(path);
}

