/* $Id: jabber_plugin_roster.c,v 1.12 2003/06/09 18:24:35 shaster Exp $ */

#include <iksemel.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "support.h"
#include "dialog.h"
#include "repo.h"

#include "jabber_plugin.h"
#include "jabber_plugin_protocol.h"
#include "jabber_plugin_roster.h"

gboolean userlist_changed = FALSE;

gint user_in_userlist(gpointer ula, gpointer cona) 
{
	GSList *ul = ula;
	GGaduContact *k1;
	GGaduContact *k2 = cona;
    
	while (ul) {
		k1 = ul->data;
			
		if (!ggadu_strcasecmp(k1->id, k2->id))
				return 1;
		
		ul = ul->next;
	}
	
	return 0;
}

void remove_roster(char *jid)
{
	iks *x,*y,*z;

	x = iks_make_iq(IKS_TYPE_SET, "jabber:iq:roster");
	z = iks_find(x,"query");
	y = iks_insert(z,"item");
	iks_insert_attrib(y, "jid", jid);
	iks_insert_attrib(y, "subscription", "remove");
	iks_send(jabber_session->parser, x);
	iks_delete(x);
}


void roster_update_presence(ikspak *pak)
{
	GGaduNotify *notify;
	GSList *l = userlist;

	if (!pak) return;

	if(pak->subtype == IKS_TYPE_ERROR) {
		print_debug("IKS_TYPE_ERROR\n");
		return;
	}

	print_debug("STATUS IN JABBER: %d\n",pak->show);

	notify = g_new0(GGaduNotify,1);
	notify->id = g_strdup(iks_id_printx(pak->from,id_print));
	notify->status = pak->show;
	
	set_userlist_status(notify, g_strdup(iks_find_cdata(pak->x,"status")), userlist);

	while (l) {
	  GGaduContact *k = (GGaduContact *)l->data;

	  if (!g_strcasecmp(k->id, notify->id)) {
	    ggadu_repo_change_value ("jabber", k->id, k, REPO_VALUE_DC);
	  }
	  l = l->next;
	}
	
//	signal_emit("jabber","gui notify",notify,"main-gui");
}


void roster_update_item(iks *x, gint type) 
{
	GGaduContact *k = NULL;

	if (!x) return;
	
	if (type == J_ROSTER_REMOVE) {
				gchar *jid = iks_find_attrib(x, "jid");
				GSList *l = userlist;
		
				while (l) {
					GGaduContact *k = (GGaduContact *)l->data;
		
					if (!g_strcasecmp(k->id,jid)) {
						g_slist_remove(userlist,k);
						ggadu_repo_del_value ("jabber", k->id);
						remove_roster(jid);
						GGaduContact_free(k);
						userlist_changed = TRUE;
						return;
					}

				l = l->next;
				}
		}
		
//	if (type == J_ROSTER_TO || J_ROSTER_FROM || J_ROSTER_BOTH	) {
	if ((type == J_ROSTER_BOTH)	 || (type == J_ROSTER_NONE)) {
				gchar *id = iks_find_attrib(x, "jid");
		
				if(!id) return;

				k = g_new0(GGaduContact, 1);
				k->id = g_strdup(id);
				k->nick = g_strdup(id); 
				k->status = (type == J_ROSTER_BOTH) ? JABBER_STATUS_UNAVAILABLE : JABBER_STATUS_WAIT_SUBSCRIBE;
	    
				if (!user_in_userlist(userlist,k)) {
					userlist_changed = TRUE;
					userlist = g_slist_append(userlist, k);
					ggadu_repo_add_value ("jabber", k->id, k, REPO_VALUE_CONTACT);
				} else 
					GGaduContact_free(k);
		}

}


void roster_update(ikspak *pak)
{
	iks *x = NULL;

	switch(pak->subtype) {	
		case IKS_TYPE_RESULT:
			print_debug("got roster\n");
			x = iks_make_pres(0, 0, NULL, NULL);
			iks_send(jabber_session->parser, x);
			iks_delete(x);

		case IKS_TYPE_SET:
			x = iks_child(pak->query);
			userlist_changed = FALSE;
		
			while(x) {
					
				if(iks_strcmp(iks_name(x), "item") == 0) {
					gint type = J_ROSTER_NONE;

					if (!g_strcasecmp(iks_find_attrib(x, "subscription"),"none")) type = J_ROSTER_NONE;
					else if (!g_strcasecmp(iks_find_attrib(x, "subscription"),"to")) type = J_ROSTER_TO;
					else if (!g_strcasecmp(iks_find_attrib(x, "subscription"),"from")) type = J_ROSTER_FROM;
					else if (!g_strcasecmp(iks_find_attrib(x, "subscription"),"both")) type = J_ROSTER_BOTH;
					else if (!g_strcasecmp(iks_find_attrib(x, "subscription"),"remove")) type = J_ROSTER_REMOVE;
			    
					print_debug("type = %d %s\n",type,iks_find_attrib(x, "subscription"));
					roster_update_item(x,type);
				}
				
			x = iks_next(x);
			}
		
		iks_delete(pak->x);
		
		if (userlist_changed)
		    signal_emit("jabber", "gui send userlist", userlist, "main-gui");
		    
		break;
		
		default:
			print_debug("parse roster failed\n");
			iks_delete(pak->x);
		}

	signal_emit("jabber", "gui status changed", (gpointer)jabber_session->status, "main-gui");
}


void presence_parse(ikspak *pak)
{
	iks *x = NULL;
	iksid *id = NULL;
	gchar *from = NULL;
	char *errnum;

	switch(pak->subtype) {
			
		case IKS_TYPE_ERROR:
			print_debug("jabber reports error");
			x = iks_find(pak->x, "error");
		
			if(x) {
				errnum = iks_find_attrib(x, "code");
				if(errnum)
					print_debug("[%s]: ", errnum);
				else
					print_debug(": ");

				print_debug("%s\n", iks_cdata(iks_child(x)));
				
			} else print_debug(": No error message!\n");
		break;

		case IKS_TYPE_SUBSCRIBE: {
			GGaduDialog *d = ggadu_dialog_new();
//			GGaduNotify *notify = g_new0(GGaduNotify,1);

			id = pak->from;

//			notify->id = g_strdup_printf("%s@%s",id->user,id->server);
//			notify->status = JABBER_STATUS_WAIT_SUBSCRIBE;
		
//			set_userlist_status(notify->id, notify->status, NULL, userlist);
		
//			signal_emit("jabber","gui notify",notify,"main-gui");
			
			from = g_strdup_printf(_("User : %s@%s \nwants to SUBSCRIBE your presence"),id->user,id->server);
			
			ggadu_dialog_set_title(d, _("Confirmation"));
			ggadu_dialog_set_type(d, GGADU_DIALOG_YES_NO);       
			ggadu_dialog_callback_signal(d,"jabber subscribe");
			ggadu_dialog_add_entry(&(d->optlist), 0, from, VAR_NULL, NULL, VAR_FLAG_NONE);
			d->user_data = pak->from;
					
			signal_emit("jabber", "gui show dialog", d, "main-gui");
					
			}
			break;

		case IKS_TYPE_SUBSCRIBED: {
			id = pak->from;
			from = g_strdup_printf(_("%s accepts your subscription"),id->user);
			signal_emit("jabber", "gui show message",from,"main-gui");
			}
			break;

		case IKS_TYPE_UNSUBSCRIBE: {
			GGaduDialog *d = ggadu_dialog_new();

			id = pak->from;
			from = g_strdup_printf(_("User : %s@%s \nwants to UNSUBSCRIBE your presence"),id->user,id->server);

			ggadu_dialog_set_title(d, _("Confirmation"));
			ggadu_dialog_set_type(d, GGADU_DIALOG_YES_NO);       
			ggadu_dialog_callback_signal(d,"jabber unsubscribe");
			ggadu_dialog_add_entry(&(d->optlist), 0, from, VAR_NULL, NULL, VAR_FLAG_NONE);
			d->user_data = pak->from;
					
			signal_emit("jabber", "gui show dialog", d, "main-gui");

			print_debug("wants to unsubscribe from your presence.\n");
			}
			break;

		case IKS_TYPE_UNSUBSCRIBED:
			/* WTF ? */
			id = pak->from;
			from = g_strdup_printf(_("%s unsubscribe you from presence"),id->user);
			signal_emit("jabber", "gui show message",from,"main-gui");
		
			x = iks_make_pres(IKS_TYPE_UNSUBSCRIBED, 0, iks_id_printx(pak->from,id_print), NULL);
			iks_send(jabber_session->parser, x);
			iks_delete(x);

			break;
		default:
			print_debug("(unknown presence code)\n");
	}
}
