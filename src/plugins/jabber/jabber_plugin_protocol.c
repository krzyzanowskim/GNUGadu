/*
** Jabber Plugin for GNU Gadu 2 2002-2003 Marcin Krzyzanowski <krzak@linux.net.pl>
**
** Parts of code from :
** JabberX (Jabber Client)
** Copyright (c) 1999-2001 Gurer Ozen <palpa@jabber.org>
**
** This code is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License.
*/
#include <iksemel.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "support.h"


#include "jabber_plugin.h"
#include "jabber_plugin_protocol.h"
#include "jabber_plugin_roster.h"

gpointer jabber_login(gpointer data) 
{

	if (!jabber_session) {
		jabber_session = (struct netdata *)g_new0(struct netdata,1);
		jabber_session->parser = iks_jabber_new(jabber_session, j_on_packet);
		jabber_session->status = JABBER_STATUS_AVAILABLE;
    
		if (!jabber_session->parser) {
			print_debug("Something went bad witn iksemel!\n");
			return NULL;
		}
	}
    
	if (!jabber_session->id) {

		if (!config_var_get(jabber_handler,"jid")) {
			signal_emit("jabber", "gui show warning",g_strdup(_("Please check configuration file")),"main-gui");	    
			return NULL;
		}

		jabber_session->id = iks_id_new(NULL,g_strdup(config_var_get(jabber_handler,"jid")));
		jabber_session->id->resource = g_strdup("GNU Gadu 2");
		jabber_session->state = NET_OFF;
	}

	print_debug("loguje sie to jabber_plugin %s %s\n",config_var_get(jabber_handler, "jid"), config_var_get(jabber_handler, "password"));

	if (!iks_connect_tcp(jabber_session->parser, jabber_session->id->server,0)) {
		print_debug("Cannot connect!\n");
		return NULL;
	}
    
	jabber_session->state = NET_CONNECT;
    
	if (updatewatch(jabber_session)==FALSE) print_debug("ooops, updatewatch() failed !!\n");

	return NULL;
}


void jabber_presence(struct netdata *session, gint status, gchar *desc)
{
	iks *x = iks_make_pres(IKS_TYPE_AVAILABLE, status, NULL, desc);
		
	if (jabber_session && connected) 
			iks_send(jabber_session->parser, x);
	
	iks_delete(x);
}


void message_parse(ikspak *pak)
{
	iks *x = NULL;

	x = iks_find(pak->x, "body");

	if(x) {
		GGaduMsg *msg = g_new0(GGaduMsg,1);
	
		msg->id = g_strdup(iks_id_printx(pak->from,id_print));
		msg->message = g_strdup(iks_cdata(iks_child(x)));
		msg->class = GGADU_CLASS_CHAT;
	    
		signal_emit("jabber", "gui msg receive",msg,"main-gui");
	    
		iks_delete(pak->x);
	}
}


void j_handle_iq(ikspak *pak) 
{
	gchar *id = iks_find_attrib(pak->x, "id");
    
	if(iks_strcmp(id, "auth") == 0) {

		if(pak->subtype == IKS_TYPE_RESULT) {
			print_debug("Authorized, fetching roster...\n");
			iks *x = iks_make_iq(IKS_TYPE_GET, IKS_NS_ROSTER);

			jabber_session->state = NET_ON;
			connected = TRUE;

			print_debug("Send GET_ROSTER\n");
			iks_send(jabber_session->parser, x);
			iks_delete(x);
				
		} else {
				
			iks *x = iks_find(pak->x, "error");

			if (x) print_debug("Error %s: %s.\n", iks_find_attrib(x, "code"), iks_cdata(iks_child(x)));

			signal_emit("jabber", "gui show warning",g_strdup(_("Authorisation failed")),"main-gui");
			/* FIX ME: switch to NET_OFF state */
		}
		
	} else if(iks_strcmp(pak->ns, IKS_NS_ROSTER) == 0) {
		print_debug("IKS_NS_ROSTER\n");
		roster_update(pak);
	}
}


void j_on_packet(void *udata, ikspak *pak) 
{
	print_debug("ON_PACKET\n");
    
	switch(pak->type) {
		case IKS_PAK_STREAM: {
			iks *x = iks_make_auth(jabber_session->id,config_var_get(jabber_handler,"password"),iks_find_attrib(pak->x,"id"));
	    
			if (!x) break;
	    
			iks_insert_attrib(x, "id", "auth");
			iks_send(jabber_session->parser, x);
			iks_delete(x);
	    
			jabber_session->state = NET_STREAM;
	    
			print_debug("Connected, logging in... jabber\n");
	    break;
		}
		case IKS_PAK_MESSAGE:
			print_debug("MESSAGE\n");
			message_parse(pak);
		break;
		case IKS_PAK_PRESENCE:
			print_debug("PRESENCE\n");
			roster_update_presence(pak);
		break;
		case IKS_PAK_IQ:
			print_debug("IQ\n");
			j_handle_iq(pak);
		break;
		case IKS_PAK_S10N:
			print_debug("S10N\n");
			presence_parse(pak);
		break;
		case IKS_PAK_ERROR:
		print_debug("ERROR\n");
		break;
		case IKS_PAK_UNKNOWN:
			print_debug("UNKNOWN\n");
		break;
	}
    
}
