/* $Id: tlen_plugin.c,v 1.2 2003/03/23 11:51:34 zapal Exp $ */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <libtlen/libtlen.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "menu.h"
#include "support.h"
#include "tlen_plugin.h"
#include "dialog.h"

GGaduPlugin *handler;

struct tlen_session *session = NULL;

static GSList *userlist = NULL;

static GSList *search_results = NULL;

GIOChannel *source_chan = NULL;
gboolean connected = FALSE;

gchar *this_configdir = NULL;

GGaduMenu *menu_tlenmenu;

gchar *description = NULL;

gint watch = 0;

gpointer loginstatus;

GGadu_PLUGIN_INIT("tlen", GGADU_PLUGIN_TYPE_PROTOCOL);


/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 						PRIVATE STUFF
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
 
gint user_in_userlist(gpointer ula, gpointer cona)
{
    GSList *ul = ula;
    GGaduContact *k1;
    GGaduContact *k2 = cona;
    
    while (ul) {
	k1 = ul->data;
	if (!ggadu_strcasecmp(k1->id, k2->id)) return 1;
	ul = ul->next;
    }
    
    return 0;
}
 
gboolean ping(gpointer data)
{
    if (!connected) return FALSE;
    tlen_ping(session);
    print_debug("TLEN PING sent!\n");
    return TRUE;
}

void set_userlist_status(gchar *id, gint status, gchar *status_descr)
{
    GSList *slistmp = userlist;
    
    while (slistmp) {
	GGaduContact *k = slistmp->data;
	if (k != NULL) {
	    print_debug("set_userlist_status : id = %s, k->id = %s, status %d\n",id,k->id,status);
	    if (!ggadu_strcasecmp(id, k->id)) {
		k->status = status;
		if (k->status_descr) {
		    g_free(k->status_descr);
		    k->status_descr = NULL;
		}
		if (status_descr)
		    ggadu_convert("CP1250","UTF-8", status_descr, k->status_descr);
	        break;
	    }
	}
	slistmp = slistmp->next;
    }
}

void handle_search_item(struct tlen_pubdir *item)
{
	GGaduContact *k = g_new0(GGaduContact, 1);
	
	const gchar * id;
	const gchar * first_name;
	const gchar * last_name;
	const gchar * nick;
	int status = item->status;
	
	// konwersja na UTF8
	to_utf8("ISO-8859-2", item->id, id);
	to_utf8("ISO-8859-2", item->firstname, first_name);
	to_utf8("ISO-8859-2", item->lastname, last_name);
	to_utf8("ISO-8859-2", item->nick, nick);
	
	k->id = g_strdup((id) ? id : "?");
	k->first_name = (first_name) ? g_strdup(first_name) : NULL;
	k->last_name = (last_name) ? g_strdup(last_name) : NULL;
	k->nick = (nick) ? g_strdup(nick) : NULL;
	k->status = (status) ? status : TLEN_STATUS_UNAVAILABLE;

	search_results = g_slist_append(search_results, k);
}

gboolean updatewatch(struct tlen_session *sess)
{
    static int fd = 0;
    static guint tag = 0;

    if ((sess->fd != fd) || (sess->state != 0))
    {
	if (tag)
	{
	    if (g_source_remove(tag) == TRUE)
	    {
		g_io_channel_unref(source_chan);
	    } else return FALSE;
	}
	
	if ((source_chan = g_io_channel_unix_new(sess->fd)) == NULL) return FALSE;
	
	{
	    GIOCondition cond;
	    switch (sess->check)
	    {
		case (TLEN_CHECK_READ | TLEN_CHECK_WRITE):
		    cond = G_IO_IN | G_IO_OUT;
		    break;
		case TLEN_CHECK_READ:
		    cond = G_IO_IN;
		    break;
		case TLEN_CHECK_WRITE:
		    cond = G_IO_OUT;
		    break;
	    }
	    // sprawdzanie wyst±pienia warunków b³êdu
	    cond |= G_IO_ERR;
	    cond |= G_IO_HUP;
	    
	    if ((tag = g_io_add_watch(source_chan, cond, test_chan, NULL)) == 0)
	    {
		g_io_channel_unref(source_chan);
		return FALSE;
	    }
	}
    }
    return TRUE;
}

gboolean test_chan(GIOChannel *source, GIOCondition condition, gpointer data)
{
    struct tlen_event *e;
    GGaduContact *k;
    GGaduNotify *notify;
    GGaduMsg *msg;
    
/*    if (condition & G_IO_ERR || condition & G_IO_HUP) {
	connected = FALSE;
	tlen_presence(session,TLEN_STATUS_UNAVAILABLE,""); 
	return FALSE;
    }
*/    
    tlen_watch_fd(session);

    if (session->error) {
	print_debug("Because of libtlen error, connection is terminating;");
	switch (session->error) {
	    case TLEN_ERROR_UNAUTHORIZED:
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Unauthorized")), "main-gui");
		print_debug("libtlen error: Unauthorized\n");
		break;
	    
	    case TLEN_ERROR_BADRESPONSE:
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Bad response from server")), "main-gui");
		print_debug("libtlen error: Bad response from server\n");
		break;
	    
	    case TLEN_ERROR_MALLOC:
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Memory allocation error")), "main-gui");
		print_debug("libtlen error: Memory allocation error\n");
		break;
	    
	    case TLEN_ERROR_NETWORK:
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Network error")), "main-gui");
		print_debug("libtlen error: Network error\n");
		break;
	    
	    case TLEN_ERROR_OTHER:
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Unknown error")), "main-gui");
		print_debug("libtlen error: Unknown error\n");
		break;
	}	
	if (updatewatch(session)==FALSE) print_debug("ooops, updatewatch() failed !!\n");
        tlen_presence(session, TLEN_STATUS_UNAVAILABLE, NULL);
        connected = FALSE;
        tlen_freesession(session);
	session = FALSE;
        signal_emit(GGadu_PLUGIN_NAME, "gui disconnected", NULL, "main-gui");
	return FALSE;	
    }
    
    while ((e = tlen_getevent(session)) != NULL) {
	print_debug("%d",e->type);
	switch (e->type) {	
	    case TLEN_EVENT_AUTHORIZED:
		tlen_getroster(session);
		connected = TRUE;
	    
		/* pingpong */
		g_timeout_add(100000, ping, NULL);
		break;
	    
	    case TLEN_EVENT_GOTROSTERITEM:

		if (e->roster->jid == NULL) {
		    print_debug("%s \t\t B£¡D PODCZAS GOTROSTERITEM!\n", GGadu_PLUGIN_NAME);
		    signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Error while GETROSTERITEM")), "main-gui");
		    break;
		    }

		k = g_new0(GGaduContact, 1);
		k->id = g_strdup(e->roster->jid);
		if (e->roster->name) k->nick = g_strdup(e->roster->name); else k->nick = g_strdup(e->roster->jid);
		
		k->status = TLEN_STATUS_UNAVAILABLE;
		if (!user_in_userlist(userlist,k))
		    userlist = g_slist_append(userlist, k);
		break;
	    
	    case TLEN_EVENT_ENDROSTER:
		tlen_presence(session, (int)loginstatus, "");
		signal_emit(GGadu_PLUGIN_NAME, "gui status changed", NULL, "main-gui");
		signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
		break;
	    
	    case TLEN_EVENT_SUBSCRIBE:
		k = g_new0(GGaduContact, 1);
		k->id = g_strdup(e->subscribe->jid);
		tlen_accept_subscribe(session,k->id);
		signal_emit(GGadu_PLUGIN_NAME, "auth request", k, "main-gui");    
		break;
	    
	    case TLEN_EVENT_SUBSCRIBED:
		k = g_new0(GGaduContact, 1);
		k->id = g_strdup(e->subscribe->jid);
		signal_emit(GGadu_PLUGIN_NAME, "auth request accepted", k, "main-gui");    
		break;
		
	    case TLEN_EVENT_UNSUBSCRIBE:
		k = g_new0(GGaduContact, 1);
		k->id = g_strdup(e->subscribe->jid);
		tlen_accept_unsubscribe(session,k->id);
		signal_emit(GGadu_PLUGIN_NAME, "unauth request", k, "main-gui");    
		break;
	    
	    case TLEN_EVENT_UNSUBSCRIBED:
		k = g_new0(GGaduContact, 1);
		k->id = g_strdup(e->subscribe->jid);
		signal_emit(GGadu_PLUGIN_NAME, "unauth request accepted", k, "main-gui");    
		break;
	    
	    case TLEN_EVENT_MESSAGE:
		print_debug("masz wiadomo¶æ!\n");
		print_debug("od: %d\n", e->message->from);
		print_debug("tre¶æ: %s\n", e->message->body);
		
		msg = g_new0(GGaduMsg,1);
		
		msg->id = g_strdup_printf("%s",e->message->from);
		
		to_utf8("ISO-8859-2",e->message->body,msg->message);
		
		msg->class = e->message->type;
		signal_emit(GGadu_PLUGIN_NAME, "gui msg receive",msg,"main-gui");
		
		break;
	    
	    case TLEN_EVENT_PRESENCE:
	    	notify  = g_new0(GGaduNotify,1);
		notify->id = g_strdup_printf("%s",e->presence->from);
		notify->status = e->presence->status;


		print_debug("STATUS IN EVENT: %d\n",e->presence->status);
		set_userlist_status(notify->id, notify->status, e->presence->description);
		signal_emit(GGadu_PLUGIN_NAME,"gui notify",notify,"main-gui");
		break;
		
		case TLEN_EVENT_GOTSEARCHITEM:
			handle_search_item(e->pubdir);
		break;
		
		case TLEN_EVENT_ENDSEARCH:
			if (search_results) {
			    signal_emit(GGadu_PLUGIN_NAME, "gui show search results", search_results, "main-gui");
			    search_results = NULL;
			} else 
			    signal_emit(GGadu_PLUGIN_NAME, "gui show message", g_strdup(_("No users have been found!")), "main-gui");
		break;
	}
	tlen_freeevent(e);
	
    }
    if (updatewatch(session)==FALSE) print_debug("ooops, updatewatch() failed !!\n");
    return TRUE;
}

gpointer login(gpointer data) 
{
    gchar *login, *password;
#ifndef DEBUG
    tlen_setdebug(0);
#else
    tlen_setdebug(1);
#endif

    if (!session) {
	session = tlen_init();
    } else {
        if (connected) {
	    tlen_presence(session,TLEN_STATUS_UNAVAILABLE,""); 
	    g_io_channel_shutdown(source_chan,TRUE,NULL);    
	}
	tlen_freesession(session);
	session = tlen_init();
    }

    login = config_var_get(handler, "login");
    password = config_var_get(handler, "password");

    if ((!login || !*login) || (!password || !*password)) {
	user_preferences_action(NULL);
	// warning pokazujemy pozniej, bo jest modalny!
	signal_emit(GGadu_PLUGIN_NAME, "gui show warning",g_strdup(_("You have to enter your login and password first!")),"main-gui");
	signal_emit(GGadu_PLUGIN_NAME, "gui disconnected",NULL,"main-gui");
	return NULL;
    } 
    
    print_debug("loguje sie to tlen_plugin %s %s\n",login, password);

    
    tlen_set_auth(session,login, password);
    tlen_set_hub_blocking(session,0);
    tlen_login(session);
    if (updatewatch(session)==FALSE) print_debug("ooops, updatewatch() failed !!\n");
    
    loginstatus = data;

    return NULL;
}

void wyjdz_signal_handler() {
    show_error(_("Program is about to leave"));
}

gpointer user_info_action(gpointer user_data)                                   
{                                                                               
    GGaduContact *k = user_data;                                                
    if (k) {                                                                    
        print_debug(", -----------------\n");                                   
        print_debug("| id     :  %s\n",k->id);                                  
        print_debug("| nick   :  %s\n",k->nick);                                
        print_debug("| status :  %d\n",k->status);                              
        print_debug("` -----------------\n");                                   
    }                                                                           
    return NULL;                                                                
}   

gpointer user_chat_action(gpointer user_data)                                   
{                                                                               
    GSList *users = (GSList *)user_data;
    
    while (users) 
    {
	GGaduContact *k = (GGaduContact *)users->data;
        GGaduMsg *msg = g_new0(GGaduMsg,1);                                         

        msg->id = k->id;
        msg->message = NULL;
        msg->class = TLEN_CHAT;
		                                                                                    
        signal_emit(GGadu_PLUGIN_NAME, "gui msg receive",msg,"main-gui");

	users = users->next;
    }

    return NULL;                                                                
} 

gpointer user_remove_user_action(gpointer user_data)                            
{                                                                               
    GSList *users = (GSList *)user_data;
    
    while (users) 
    {
	GGaduContact *k = (GGaduContact *)users->data;
                                    
        userlist = g_slist_remove(userlist,k);
        tlen_request_unsubscribe (session, k->id);
        tlen_removecontact (session, k->id);

        GGaduContact_free(k);

	users = users->next;
    }

    signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");  
    
    return NULL;                                                                
}                                                                               
			                                                                                
gpointer user_add_user_action(gpointer user_data)                               
{        
    GSList *optlist   = NULL;
    GGaduKeyValue *kv = NULL;
    
    kv = g_new0(GGaduKeyValue,1);
    kv->key		= TLEN_TLEN_UIN;
    kv->description	= g_strdup("Tlen ID");
    kv->type		= VAR_INT;
    
    optlist = g_slist_append(optlist,kv);

    kv = g_new0(GGaduKeyValue,1);
    kv->key		= TLEN_TLEN_NICK;
    kv->description	= g_strdup(_("Nick"));
    kv->type		= VAR_STR;
    
    optlist = g_slist_append(optlist,kv);

    kv = g_new0(GGaduKeyValue,1);
    kv->key		= TLEN_TLEN_GROUP;
    kv->description	= g_strdup(_("Group"));
    kv->type		= VAR_STR;
    
    optlist = g_slist_append(optlist,kv);
                                                                       
    signal_emit(GGadu_PLUGIN_NAME, "gui add user window", optlist, "main-gui");
    
    return NULL;                                                                
}       

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 						PLUGIN STUFF
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

GGaduPlugin *initialize_plugin(gpointer conf_ptr) {
    print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);
    
    GGadu_PLUGIN_ACTIVATE(conf_ptr); /* wazne zeby wywolac to makro w tym miejscu */

    handler = (GGaduPlugin *)register_plugin(GGadu_PLUGIN_NAME,"Tlen protocol");

    register_signal_receiver((GGaduPlugin *)handler, (signal_func_ptr)my_signal_receive);

    if (g_getenv("CONFIG_DIR"))
	this_configdir = g_build_filename(g_get_home_dir(),g_getenv("CONFIG_DIR"),"tlen",NULL);
     else
	this_configdir = g_build_filename(g_get_home_dir(),".tlen",NULL);

    mkdir(this_configdir, 0700);
    
    set_config_file_name((GGaduPlugin *)handler,g_build_filename(this_configdir,"config",NULL));
    
    config_var_add(handler, "login",    VAR_STR);
    config_var_add(handler, "password", VAR_STR);
    config_var_add(handler, "autoconnect", VAR_BOOL);
    
    config_read(handler);

    return handler;
}

GSList *status_init() 
{
    GSList *list = NULL;
    GGaduStatusPrototype *sp;
    
    sp = g_new0(GGaduStatusPrototype, 8);

    sp->status = TLEN_STATUS_AVAILABLE;
    sp->description = g_strdup(_("Available"));
    sp->image = g_strdup("tlen-online.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = TLEN_STATUS_AWAY;
    sp->description = g_strdup(_("Away"));
    sp->image = g_strdup("tlen-away.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = TLEN_STATUS_CHATTY;
    sp->description = g_strdup(_("Chatty"));
    sp->image = g_strdup("tlen-chat.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = TLEN_STATUS_EXT_AWAY;
    sp->description = g_strdup(_("Very busy"));
    sp->image = g_strdup("tlen-beback.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = TLEN_STATUS_DND;
    sp->description = g_strdup(_("DND"));
    sp->image = g_strdup("tlen-occupied.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = TLEN_STATUS_INVISIBLE;
    sp->description = g_strdup(_("Invisible"));
    sp->image = g_strdup("tlen-invisible.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = TLEN_STATUS_UNAVAILABLE;
    sp->description = g_strdup(_("Unavailable"));
    sp->image = g_strdup("tlen-offline.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = TLEN_STATUS_DESC;
    sp->description = g_strdup(_("Set description ..."));
    sp->image = g_strdup("tlen-desc.png");
    list = g_slist_append(list, sp);
    sp++;

    return list;
}

void free_search_results()
{
    GSList *tmplist = search_results;
    
    while (tmplist) {
	if (tmplist->data != NULL)
	    GGaduContact_free((GGaduContact *)tmplist->data);
	tmplist = tmplist->next;
    }
    g_slist_free(search_results);
    search_results = NULL;
}

gpointer search_action(gpointer user_data)
{
    GGaduDialog *d = NULL;
    
    if (!connected) {
	signal_emit(GGadu_PLUGIN_NAME, "gui show warning",g_strdup(_("You have to be connected to perform searching!")),"main-gui");
	return NULL;
    }
    
//    if (search_results)
	//free_search_results();
    
    d = ggadu_dialog_new();
    
    d->title = g_strdup(_("Tlen search")); 
    d->callback_signal = g_strdup("search");

    ggadu_dialog_add_entry(&(d->optlist), GGADU_SEARCH_FIRSTNAME, _("First name:"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SEARCH_LASTNAME, _("Last name:"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SEARCH_NICKNAME, _("Nick:"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SEARCH_CITY, _("City:"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SEARCH_ID, _("@tlen.pl"), VAR_STR, NULL, VAR_FLAG_NONE);
    
    signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");
        
    return NULL;
}

void register_userlist_menu() 
{
    GGaduMenu *umenu    = ggadu_menu_create();                                  
    GGaduMenu *listmenu = NULL;                                                 
	                                                                                    
    ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("Chat"),user_chat_action,NULL));
			                                                                                
    listmenu = ggadu_menu_new_item(_("List"),NULL,NULL);                        
    ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Remove"),user_remove_user_action,NULL));
    ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Add"),user_add_user_action,NULL));
		                                                                                    
    ggadu_menu_add_submenu(umenu, listmenu );                                   
                                                        
    ggadu_menu_print(umenu,NULL);                                               

    signal_emit(GGadu_PLUGIN_NAME, "gui register userlist menu", umenu, "main-gui");
} 

gpointer user_preferences_action(gpointer user_data) 
{
    GGaduDialog *d = ggadu_dialog_new();
    
    ggadu_dialog_set_title(d,_("Tlen plugin configuration"));
    ggadu_dialog_callback_signal(d,"update config");
        
    ggadu_dialog_add_entry(&(d->optlist), TLEN_TLEN_UIN, _("Tlen login"), VAR_STR, config_var_get(handler, "login"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), TLEN_TLEN_PASSWORD, _("Password"), VAR_STR, config_var_get(handler, "password"), VAR_FLAG_PASSWORD);
    ggadu_dialog_add_entry(&(d->optlist), TLEN_TLEN_AUTOCONNECT, _("Auto connect on startup"), VAR_BOOL, config_var_get(handler, "autoconnect"), VAR_FLAG_NONE);
    
    signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");
        
    return NULL;
}


GGaduMenu *build_tlen_menu() 
{
    GGaduMenu *root = ggadu_menu_create();
    GGaduMenu *item_tl = ggadu_menu_add_item(root,"_Tlen",NULL,NULL);
    
    ggadu_menu_add_submenu(item_tl, ggadu_menu_new_item(_("Add Contact"),user_add_user_action,NULL));
    ggadu_menu_add_submenu(item_tl, ggadu_menu_new_item(_("Preferences"),user_preferences_action,NULL));
    ggadu_menu_add_submenu(item_tl, ggadu_menu_new_item(_("Search for friends"),search_action,NULL));
    
    return root;
}


void start_plugin()
{
	GGaduProtocol *p;

        print_debug("%s : start_plugin\n",GGadu_PLUGIN_NAME);

	p = g_new0(GGaduProtocol, 1);
	p->display_name = g_strdup("Tlen");
	p->img_filename = g_strdup("Tlen.png");
	p->statuslist = status_init();
	p->offline_status = TLEN_STATUS_UNAVAILABLE;

	handler->protocol = p;
		
	signal_emit(GGadu_PLUGIN_NAME, "gui register protocol", p, "main-gui");
        
        register_signal(handler,"change status");
        register_signal(handler,"change status descr");
        register_signal(handler,"send message");
        register_signal(handler,"add user");
        register_signal(handler,"change user");
        register_signal(handler,"update config");
        register_signal(handler,"search");
        register_signal(handler,"add user search");
        register_signal(handler,"get current status");

	menu_tlenmenu = build_tlen_menu();
	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_tlenmenu, "main-gui");
	
	register_userlist_menu();

	if (config_var_get(handler, "autoconnect") && !connected)
	    login((gpointer)TLEN_STATUS_AVAILABLE);
}

void my_signal_receive(gpointer name, gpointer signal_ptr) {
	GGaduSignal *signal = (GGaduSignal *)signal_ptr;
        print_debug("%s : receive signal %s\n",GGadu_PLUGIN_NAME,(gchar *)signal->name);

	if (!ggadu_strcasecmp(signal->name,"wyjdz")) { 
	    wyjdz_signal_handler(); 
	}
	
	if (!ggadu_strcasecmp(signal->name,"change status")) { 
	    GGaduStatusPrototype *sp = signal->data;
	    
	    if (connected && sp)
	    {
		if (sp->status == TLEN_STATUS_UNAVAILABLE)
		{
		    g_source_remove(watch);
		    tlen_presence(session, TLEN_STATUS_UNAVAILABLE, NULL);
		    connected = FALSE;
		    tlen_freesession(session);
		    session = FALSE;
		    g_io_channel_shutdown(source_chan, FALSE, NULL);
		    signal_emit(GGadu_PLUGIN_NAME, "gui disconnected", NULL, "main-gui");
		    signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", NULL, "main-gui");
		}
		else if (sp->status == TLEN_STATUS_DESC)
		{    
		    GGaduDialog *d = ggadu_dialog_new();
		    GGaduKeyValue *kv = NULL; 
		    GGaduStatusPrototype *status;
		    GSList *tmp = NULL;
		    
		    /* Wyszukiwanie StatusPrototype :-D */

		    tmp = handler->protocol->statuslist;
		    
		    if (tmp != NULL)
		    {
			while (tmp)
			{
			    GGaduStatusPrototype *sp = tmp->data;
			
			    if ((sp) && (sp->status == session->status))
			    {
				status = sp;
				break;
			    }
			    
			    tmp = tmp->next;
			}
		    }
		    else
		    {
			print_debug("Something went wrong, statuslist is NULL!!!\n");
		    }
		    /* end */
		        
		    ggadu_dialog_set_title(d,_("Enter status description"));
		    ggadu_dialog_callback_signal(d,"change status descr");
		    
		    kv = g_new0(GGaduKeyValue,1);
		    kv->description = g_strdup(_("Description:"));
		    kv->type = VAR_STR;
		    
		    d->optlist = g_slist_append(d->optlist, kv);
		    d->user_data = status;
		    signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");
		    signal_emit(GGadu_PLUGIN_NAME, "gui change icon", status, "main-gui");    
		}
		else
		{
		    tlen_presence(session, sp->status, description);
		}
	    }
	    else if (sp && sp->status != TLEN_STATUS_UNAVAILABLE && sp->status != TLEN_STATUS_DESC) 
		login((gpointer)sp->status);
	    return;
	}


	if (!ggadu_strcasecmp(signal->name,"update config")) 
	{ 
	    GGaduDialog *d = signal->data;
	    GSList *tmplist = d->optlist;
	    
	    while (tmplist) 
	    {
		GGaduKeyValue *kv = (GGaduKeyValue *)tmplist->data;
		
		switch (kv->key) 
		{
		    case TLEN_TLEN_UIN:
				    print_debug("changing var setting uin to %s\n", kv->value);
				    config_var_set(handler, "login", kv->value);
				    break;
		    case TLEN_TLEN_PASSWORD:
				    print_debug("changing var setting password to %s\n", kv->value);
				    config_var_set(handler, "password", kv->value);
				    break;
		    case TLEN_TLEN_AUTOCONNECT:
				    print_debug("changing var setting autoconnect to %d\n", kv->value);
				    config_var_set(handler, "autoconnect", kv->value);
				    break;
		}
		tmplist = tmplist->next;
	    }
	    
	    config_save(handler);
	    
	    GGaduDialog_free(d);
	    
	    return;
	}
	
	if (!ggadu_strcasecmp(signal->name,"change status descr"))
	{
	    GGaduDialog *d = signal->data;
	    GGaduStatusPrototype *sp = d->user_data;
	    
	    if (connected == FALSE)
	    {
		login((gpointer)TLEN_STATUS_AVAILABLE);
	    }
	    else if (connected && sp)
	    {
		GGaduKeyValue *kv = NULL;
		if (d->optlist)
		{
		    kv = (GGaduKeyValue *)d->optlist->data;
		    tlen_presence(session, sp->status, kv->value);
		    description = g_strdup(kv->value);
		}
	    }
	    GGaduDialog_free(d);
	    return;
	}

	if (!ggadu_strcasecmp(signal->name,"add user")) { 

	    GGaduContact *k = g_new0(GGaduContact,1);
	    GSList *kvlist = (GSList *)signal->data;

	    while (kvlist) {
		GGaduKeyValue *kv = (GGaduKeyValue *)kvlist->data;
		
		switch ((gint)kv->key) {
		
		    case TLEN_TLEN_UIN:
			k->id = g_strdup((gchar *)kv->value);
			g_free(kv->value);
			break;

		    case TLEN_TLEN_NICK:
			k->nick		= g_strdup((gchar *)kv->value);
			g_free(kv->value);
			break;
			
		    case TLEN_TLEN_GROUP:
			k->group	= g_strdup((gchar *)kv->value);
			g_free(kv->value);
			break;
		}
		
		g_free(kv->description);
		kvlist=kvlist->next;
	    }
	    
	    g_slist_free(kvlist);
	    
	    userlist = g_slist_append(userlist,k);
	    
            tlen_addcontact(session,k->nick,k->id,k->group);

	    tlen_request_subscribe(session, k->id);
	    
	    signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
	    
	}


	if (!ggadu_strcasecmp(signal->name,"send message")) {
		
	    GGaduMsg *msg = signal->data;
	    if (connected && msg) {
		gchar *message = NULL;
		
		from_utf8("ISO-8859-2",msg->message,message);
		
		if (msg->class == GGADU_CLASS_CHAT)
		    msg->class = TLEN_CHAT;
		else
		    msg->class = TLEN_MESSAGE;
		    
		switch (msg->class)
		{
		    case TLEN_CHAT:
			if (!tlen_sendmsg(session, msg->id, message, TLEN_CHAT))
			    print_debug("zjebka podczas send message %s\n",msg->id, TLEN_CHAT);
			break;
		    case TLEN_MESSAGE:
			if (!tlen_sendmsg(session, msg->id, message, TLEN_MESSAGE))
			    print_debug("zjebka podczas send message %s\n",msg->id, TLEN_MESSAGE);

			break;
		}
		
		g_free(message);
	    }
	}

	if (!ggadu_strcasecmp(signal->name,"search")) {
            GGaduDialog *d = signal->data;
    	    GSList *tmplist = d->optlist;
		
	    struct tlen_pubdir *req;

	    if (!(req = tlen_new_pubdir())) {
	    	GGaduDialog_free(d);
		return;
	    }
			
	    while (tmplist) {
		GGaduKeyValue *kv = (GGaduKeyValue *)tmplist->data;
				
		switch (kv->key) {  
			case GGADU_SEARCH_FIRSTNAME:
		    	    	    if (kv->value && *(gchar*)kv->value)
					req->firstname = g_strdup(kv->value);
			    	break;
			case GGADU_SEARCH_LASTNAME:
		    	    	    if (kv->value && *(gchar*)kv->value)
					req->lastname = g_strdup(kv->value);
			    	break;
			case GGADU_SEARCH_NICKNAME:
		    	    	    if (kv->value && *(gchar*)kv->value)
					req->nick = g_strdup(kv->value);
			    	break;
			case GGADU_SEARCH_CITY:
		    	    	    if (kv->value && *(gchar*)kv->value)
					req->city = g_strdup(kv->value);
				break;
			case GGADU_SEARCH_ID: {
    				    if (kv->value) {
			    		req->id = g_strdup(kv->value);
				    }
			    	}
				break;					
			}
				
			tmplist = tmplist->next;
		}

		if (!(tlen_search(session, req)))
	    	    signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Error! Cannot perform search!")),"main-gui");
					
		GGaduDialog_free(d);
		tlen_free_pubdir(req);
	}
	
	if (!ggadu_strcasecmp(signal->name, "get current status")) {
	    if (session) 
		signal->data_return = (gpointer) session->status;
	    else
		signal->data_return = (gpointer) TLEN_STATUS_UNAVAILABLE;
	}
}



void destroy_plugin() {
    print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
    if (menu_tlenmenu)
    {
      signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", menu_tlenmenu, "main-gui");
    }

}

