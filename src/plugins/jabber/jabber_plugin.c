/*
** Jabber protocol plugin for GNU Gadu 2
** Marcin Krzyzanowski Copyright (c) 2002-2003
**
** Based on code from:
** JabberX (Jabber Client)
** Copyright (c) 1999-2001 Gurer Ozen <palpa@jabber.org>
**
** This code is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License.
**
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <iksemel.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "menu.h"
#include "support.h"
#include "dialog.h"
#include "repo.h"

#include "jabber_plugin.h"
#include "jabber_plugin_protocol.h"
#include "jabber_plugin_roster.h"

GGaduPlugin *jabber_handler;

extern GGaduConfig *config;

GIOChannel *source_chan;
GSList *userlist;
gint watch = 0;
static guint tag = 0;
gboolean connected = FALSE;

GGaduProtocol *p = NULL;
GGaduMenu *menu_jabbermenu;

struct netdata *jabber_session = NULL;

GGadu_PLUGIN_INIT("jabber", GGADU_PLUGIN_TYPE_PROTOCOL);

void jabber_signal_receive(gpointer name, gpointer signal_ptr) {
	GGaduSignal *signal = (GGaduSignal *)signal_ptr;
		
	print_debug("%s : receive signal %s\n",GGadu_PLUGIN_NAME,(gchar *)signal->name);

	if (!ggadu_strcasecmp(signal->name,"jabber subscribe")) { 
		GGaduDialog *d = signal->data;
		iksid *id = d->user_data;
			
		if ((d->response == GGADU_OK) && (id)) {
					iks *x = NULL;
						
					x = iks_make_pres(IKS_TYPE_SUBSCRIBED, 0, iks_id_printx(id,id_print), NULL);
					iks_send(jabber_session->parser, x);
					iks_delete(x);
						
					x = iks_make_pres(IKS_TYPE_SUBSCRIBE, 0, iks_id_printx(id,id_print), NULL);
					iks_send(jabber_session->parser, x);
					iks_delete(x);
		
		} else if (d->response == GGADU_CANCEL) {
				gchar *jid = g_strdup_printf("%s@%s",id->user,id->server);
				remove_roster(jid);
				g_free(jid);
		}		
		
		GGaduDialog_free(d);
		return;
	}
		
	if (!ggadu_strcasecmp(signal->name,"jabber unsubscribe")) { 
		GGaduDialog *d = signal->data;
			
		if (d->response == GGADU_OK) {
				iksid *id = d->user_data;
				
				if (id) {
					iks *x = NULL;
					x = iks_make_pres(IKS_TYPE_UNSUBSCRIBE, 0, iks_id_printx(id,id_print), NULL);
					iks_send(jabber_session->parser, x);
					iks_delete(x);
				}
		}	    
		
		GGaduDialog_free(d);
		return;
	}

	
	if (!ggadu_strcasecmp(signal->name,"add user")) { 

		GGaduContact *k = g_new0(GGaduContact,1);
		GSList *kvlist = (GSList *)signal->data;
		iks *x = NULL;

		while (kvlist) {
			GGaduKeyValue *kv = (GGaduKeyValue *)kvlist->data;
		
			switch ((gint)kv->key) {
				case GGADU_ID:
					k->id = g_strdup((gchar *)kv->value);
					g_free(kv->value);
				break;
			}
		
		kvlist=kvlist->next;
		}

		g_slist_free(kvlist);
	    
		x = iks_make_pres(IKS_TYPE_SUBSCRIBE, 0, g_strdup(k->id), NULL);
		iks_send(jabber_session->parser, x);
		iks_delete(x);
	    
//ZONK	    GGaduContact_free(k);
	    
	    return;
	}

	if (!ggadu_strcasecmp(signal->name,"change status descr")) { 
		GGaduDialog *d = signal->data;
		GGaduStatusPrototype *sp = d->user_data;
			
		if (d->response == GGADU_OK) {
			if (connected && sp && d && d->optlist) {
				GGaduKeyValue *kv = (GGaduKeyValue *)d->optlist->data;
				jabber_presence(jabber_session, sp->status, kv->value);
				signal_emit(GGadu_PLUGIN_NAME,"gui status changed",(gpointer)sp->status,"main-gui");
			}
		}	    
		
		GGaduDialog_free(d);
		return;
	}

	
	if (!ggadu_strcasecmp(signal->name,"send message")) {
		GGaduMsg *msg = signal->data;
			
		if (msg && connected) {
			iks   *x,*y;
		
			x = iks_new("message");
			iks_insert_attrib(x, "to", g_strdup(msg->id));
			iks_insert_attrib(x, "type", g_strdup("chat"));
				
			y = iks_insert(x, "body");
			iks_insert_cdata(y, g_strdup(msg->message), -1);
			iks_send(jabber_session->parser, x);
			iks_delete(x);
		}
	}

	
	if (!ggadu_strcasecmp(signal->name,"update config")) 
	{ 
		GGaduDialog *d = signal->data;
		GSList *tmplist = d->optlist;
	    
		if (d->response == GGADU_OK) {
				
			while (tmplist) 
			{
				GGaduKeyValue *kv = (GGaduKeyValue *)tmplist->data;
		
				switch (kv->key) {
					case GGADU_JID:
						print_debug("changing var setting jid to %s\n", kv->value);
						config_var_set(jabber_handler, "jid", kv->value);
					break;
				case GGADU_JID_PASSWORD:
					print_debug("changing var setting password to %s\n", kv->value);
					config_var_set(jabber_handler, "password", kv->value);
					break;
				case GGADU_JID_AUTOCONNECT:
					print_debug("changing var setting autoconnect to %d\n", kv->value);
					config_var_set(jabber_handler, "autoconnect", kv->value);
					break;
				}

			tmplist = tmplist->next;
			}
	    
		config_save(jabber_handler);
		}
		
	GGaduDialog_free(d);
	    
	return;
	}



	if (!ggadu_strcasecmp(signal->name,"change status")) { 
	    GGaduStatusPrototype *sp = signal->data;
	    
		if (connected && sp) {
			if ((sp->status == JABBER_STATUS_AWAY) ||
					(sp->status == JABBER_STATUS_DND) ||
					(sp->status == JABBER_STATUS_XA) ||
					(sp->status == JABBER_STATUS_DND) || 
					(sp->status == JABBER_STATUS_CHAT)) {
							
					GGaduDialog *d = g_new0(GGaduDialog, 1);
					GGaduKeyValue *kv = NULL; 

					kv = g_new0(GGaduKeyValue,1);
					kv->description = g_strdup(_("Description:"));
					kv->type = VAR_STR;

					d->title = g_strdup(_("Enter status description"));
					d->callback_signal = g_strdup("change status descr");
					d->optlist = g_slist_append(d->optlist, kv);
					d->user_data = sp;
							
					signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");

			} else if ((sp->status != JABBER_STATUS_AVAILABLE) && (connected)) {
					
				iks_disconnect(jabber_session->parser);
				g_io_channel_shutdown(source_chan, TRUE, NULL);
				connected = FALSE;
				signal_emit(GGadu_PLUGIN_NAME, "gui disconnected", NULL, "main-gui");

			} else if (sp->status == JABBER_STATUS_AVAILABLE) {
				jabber_presence(jabber_session, sp->status, NULL);
				signal_emit(GGadu_PLUGIN_NAME,"gui status changed",(gpointer)sp->status,"main-gui");
			}
			
	} else if (sp && sp->status != JABBER_STATUS_UNAVAILABLE) {
		g_thread_create(jabber_login,(gpointer)sp->status,FALSE,NULL);
	}

	if (jabber_session)
		jabber_session->status = sp->status;

	return;
}
	
	if (!ggadu_strcasecmp(signal->name,"get current status")) { 
	    if (jabber_session) 
		signal->data_return = (gpointer) jabber_session->status;
	    else
		signal->data_return = JABBER_STATUS_UNAVAILABLE;
	}
}

gpointer user_remove_user_action(gpointer user_data)                            
{                                                                               
	GSList *users = (GSList *)user_data;
    
	while (users) {
		GGaduContact *k = (GGaduContact *)users->data;
                                    
		userlist = g_slist_remove(userlist,k);
		ggadu_repo_del_value ("jabber", k->id);

		if (connected && jabber_session && k) {
			iks *x = iks_make_pres(IKS_TYPE_UNSUBSCRIBE, 0, k->id, NULL);
			iks_send(jabber_session->parser, x);
			iks_delete(x);
			remove_roster(k->id);

			GGaduContact_free(k);
		}
	
		users = users->next;
	}

	signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");  
    
	return NULL;                                                                
}                                                                               

gpointer user_add_action(gpointer user_data)
{
	GSList *optlist   = NULL;

	ggadu_dialog_add_entry(&optlist, GGADU_ID, _("Jabber ID (jid)"), VAR_STR, NULL, VAR_FLAG_NONE);
	/* wywoluje okienko dodawania usera */
	signal_emit(GGadu_PLUGIN_NAME, "gui add user window", optlist, "main-gui");

	return NULL;
}


void register_userlist_menu() {                                                 
	GGaduMenu *umenu    = ggadu_menu_create();                                  
	GGaduMenu *listmenu = NULL;                                                 

	listmenu = ggadu_menu_new_item(_("List"),NULL,NULL);                        
	ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Add"),user_add_action,NULL));
	ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Remove"),user_remove_user_action,NULL));
	ggadu_menu_add_submenu(umenu, listmenu );                                   

	signal_emit(GGadu_PLUGIN_NAME, "gui register userlist menu", umenu, "main-gui");
} 


gboolean test_chan(GIOChannel *source, GIOCondition condition, gpointer data)
{

	if (!jabber_session) return FALSE;
			
	iks_recv(jabber_session->parser,0);
		
	if (updatewatch(jabber_session)==FALSE) {
			print_debug("ooops, updatewatch() failed !!\n");
			return FALSE;
	}
	
	return TRUE;
}

gboolean updatewatch(struct netdata *sess)
{
	static int fd = 0;
	GIOCondition cond = G_IO_IN;

	if ((iks_fd(sess->parser) != fd) || (sess->state != NET_OFF)) {

		if (tag) {
				
	    if (g_source_remove(tag) == TRUE)
				g_io_channel_unref(source_chan);
	    else 
				return FALSE;
		}	

		if ((source_chan = g_io_channel_unix_new(iks_fd(sess->parser))) == NULL) 
			return FALSE;
	
		switch (sess->state) {
			case NET_CONNECT:
				cond = G_IO_OUT;
			break;
			default:
				cond = G_IO_IN;
			break;
		}
	
		// sprawdzanie wyst±pienia warunków b³êdu
		cond |= G_IO_ERR;
		cond |= G_IO_HUP;

		if ((tag = g_io_add_watch(source_chan, cond, test_chan, NULL)) == 0) {
			g_io_channel_unref(source_chan);
			return FALSE;
		}
		
	}
	
	return TRUE;
}



GSList *status_init() 
{
    GSList *list = NULL;
    GGaduStatusPrototype *sp = g_new0(GGaduStatusPrototype, 7);
    
		if (!sp) return NULL;
    
    sp->status = JABBER_STATUS_AVAILABLE;
    sp->description = g_strdup(_("Available"));
    sp->image = g_strdup("jabber-online.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = JABBER_STATUS_AWAY;
    sp->description = g_strdup(_("Away"));
    sp->image = g_strdup("jabber-away.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = JABBER_STATUS_XA;
    sp->description = g_strdup(_("XA"));
    sp->image = g_strdup("jabber-xa.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = JABBER_STATUS_DND;
    sp->description = g_strdup(_("DND"));
    sp->image = g_strdup("jabber-dnd.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = JABBER_STATUS_CHAT;
    sp->description = g_strdup(_("Chat"));
    sp->image = g_strdup("jabber-xa.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = JABBER_STATUS_UNAVAILABLE;
    sp->description = g_strdup(_("Unavailable"));
    sp->image = g_strdup("jabber-offline.png");
    list = g_slist_append(list, sp);
    sp++;

    sp->status = JABBER_STATUS_WAIT_SUBSCRIBE;
    sp->description = g_strdup(_("Subscribe"));
    sp->image = g_strdup("jabber-subscribe.png");
		sp->receive_only = TRUE;
    list = g_slist_append(list, sp);

		return list;
}

gpointer user_preferences_action(gpointer user_data)
{
	GGaduDialog *d = g_new0(GGaduDialog, 1);

	d->title = g_strdup(_("Jabber plugin configuration")); 
	d->callback_signal = g_strdup("update config");

	ggadu_dialog_set_type(d, GGADU_DIALOG_CONFIG);
	ggadu_dialog_add_entry(&(d->optlist), GGADU_JID, _("Jabber ID"), VAR_STR, config_var_get(jabber_handler, "jid"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(&(d->optlist), GGADU_JID_PASSWORD, _("Password"), VAR_STR, config_var_get(jabber_handler, "password"), VAR_FLAG_PASSWORD);
	ggadu_dialog_add_entry(&(d->optlist), GGADU_JID_AUTOCONNECT, _("Autoconnect on startup"), VAR_BOOL, config_var_get(jabber_handler, "autoconnect"), VAR_FLAG_NONE);
    
	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");

	return NULL;
}



GGaduMenu *build_jabber_menu()
{
	GGaduMenu *root = ggadu_menu_create();
	GGaduMenu *item_jb = ggadu_menu_add_item(root,"_Jabber",NULL,NULL);
    
	ggadu_menu_add_submenu(item_jb, ggadu_menu_new_item(_("Add Contact"),user_add_action, NULL));
	ggadu_menu_add_submenu(item_jb, ggadu_menu_new_item(_("Preferences"),user_preferences_action, NULL));
    
	return root;
}

void start_plugin()
{
	p = g_new0(GGaduProtocol,1);
	p->display_name = g_strdup("Jabber");
	p->img_filename = g_strdup("jabber.png");
	p->statuslist = status_init();
	p->offline_status = JABBER_STATUS_UNAVAILABLE;
    
	signal_emit(GGadu_PLUGIN_NAME,"gui register protocol", p, "main-gui");
	register_signal(jabber_handler,"change status");
	register_signal(jabber_handler,"change status descr");
	register_signal(jabber_handler,"send message");
	register_signal(jabber_handler,"add user");
	register_signal(jabber_handler,"change user");
	register_signal(jabber_handler,"update config");
	register_signal(jabber_handler,"search");
	register_signal(jabber_handler,"add user search");
	register_signal(jabber_handler,"get current status");
	register_signal(jabber_handler,"jabber subscribe");
	register_signal(jabber_handler,"jabber unsubscribe");

	menu_jabbermenu = build_jabber_menu();
		
	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_jabbermenu, "main-gui");
	
	register_userlist_menu();
    
	if (config_var_get(jabber_handler, "autoconnect") && !connected)
		g_thread_create(jabber_login,(gpointer) JABBER_STATUS_AVAILABLE,FALSE,NULL);

}


GGaduPlugin *initialize_plugin(gpointer conf_ptr) 
{
	GGadu_PLUGIN_ACTIVATE(conf_ptr);

	print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);

	jabber_handler = (GGaduPlugin *)register_plugin(GGadu_PLUGIN_NAME,_("Jabber protocol"));

	register_signal_receiver((GGaduPlugin *)jabber_handler, (signal_func_ptr)jabber_signal_receive);

	mkdir(config->configdir, 0700);
    
	set_config_file_name((GGaduPlugin *)jabber_handler, g_build_filename(config->configdir,"jabber",NULL));
		
	config_var_add(jabber_handler, "jid",    VAR_STR);
	config_var_add(jabber_handler, "password", VAR_STR);
	config_var_add(jabber_handler, "autoconnect", VAR_BOOL);
    
	if (!config_read(jabber_handler)) 
		g_warning(_("Unable to read configuration file for plugin jabber"));
	
	ggadu_repo_add ("jabber");
	
	return jabber_handler;
}


void destroy_plugin() {
	print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
		
	if (menu_jabbermenu) {
		signal_emit (GGadu_PLUGIN_NAME, "gui unregister menu", menu_jabbermenu, "main-gui");
		ggadu_menu_free (menu_jabbermenu);
	}
	
	signal_emit (GGadu_PLUGIN_NAME, "gui unregister userlist menu", NULL, "main-gui");
	signal_emit (GGadu_PLUGIN_NAME, "gui unregister protocol", p, "main-gui");
}
