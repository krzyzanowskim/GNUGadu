#include <iksemel.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "support.h"

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
	if (!ggadu_strcasecmp(k1->id, k2->id)) return 1;
	ul = ul->next;
    }
    
    return 0;
}

void set_userlist_status(gchar *id, gint status, gchar *status_descr)
{
    GSList *slistmp = userlist;
    
    while (slistmp) {
	GGaduContact *k = slistmp->data;
	if (k != NULL) 
	{
	    print_debug("set_userlist_status : id = %s, k->id = %s, status %d\n",id,k->id,status);
	    if (!ggadu_strcasecmp(id, k->id)) {
		k->status = status;
		if (k->status_descr) {
		    g_free(k->status_descr);
		    k->status_descr = NULL;
		}
		if (status_descr)
		    k->status_descr = g_strdup(status_descr);
//		    ggadu_convert("ISO-8859-2", "UTF-8", status_descr, k->status_descr);
	        break;
	    }
	}
	slistmp = slistmp->next;
    }
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
	if(pak->subtype == IKS_TYPE_ERROR)
	{
		print_debug("IKS_TYPE_ERROR\n");
		return;
	}

	print_debug("STATUS IN JABBER: %d\n",pak->show);

	notify = g_new0(GGaduNotify,1);
	notify->id = g_strdup(iks_id_printx(pak->from,id_print));
	notify->status = pak->show;
	
	set_userlist_status(notify->id, notify->status, iks_find_cdata(pak->x,"status"));
	signal_emit("jabber","gui notify",notify,"main-gui");
}


void roster_update_item(iks *x, gint type) 
{
    GGaduContact *k = NULL;

    if (type == J_ROSTER_REMOVE)
    {
	gchar *jid = iks_find_attrib(x, "jid");
	GSList *l = userlist;
	while (l)
	{
	        GGaduContact *k = (GGaduContact *)l->data;
		
	        if (!g_strcasecmp(k->id,jid))
	        {
			g_slist_remove(userlist,k);
			remove_roster(jid);
			GGaduContact_free(k);
			userlist_changed = TRUE;
			return;
	        }
		
	l = l->next;
	}

    return;	
    }
	
    if (type == J_ROSTER_TO || J_ROSTER_FROM || J_ROSTER_BOTH)
    {
	    gchar *id = NULL;
	    
	    id = iks_find_attrib(x, "jid");
	    if(!id) return;

	    k = g_new0(GGaduContact, 1);
	    k->id = g_strdup(id);
	    k->nick = g_strdup(id); 
	    k->status = JABBER_STATUS_UNAVAILABLE;
	    
	    if (!user_in_userlist(userlist,k)) 
	    {
		userlist_changed = TRUE;
	        userlist = g_slist_append(userlist, k);
	    } else GGaduContact_free(k);

    return;
    }
}

void roster_update(ikspak *pak)
{
	iks *x = NULL;

	switch(pak->subtype)
	{
	case IKS_TYPE_RESULT:
		print_debug("got roster\n");
		x = iks_make_pres(0, 0, NULL, NULL);
		iks_send(jabber_session->parser, x);
		iks_delete(x);

	case IKS_TYPE_SET:
		x = iks_child(pak->query);
		
		userlist_changed = FALSE;
		
		while(x)
		{
			if(iks_strcmp(iks_name(x), "item") == 0)
			{
			    gint type = J_ROSTER_NONE;

			    if (!g_strcasecmp(iks_find_attrib(x, "subscription"),"none"))
				type = J_ROSTER_NONE;
			    else if (!g_strcasecmp(iks_find_attrib(x, "subscription"),"to"))
				type = J_ROSTER_TO;
			    else if (!g_strcasecmp(iks_find_attrib(x, "subscription"),"from"))
				type = J_ROSTER_FROM;
			    else if (!g_strcasecmp(iks_find_attrib(x, "subscription"),"both"))
				type = J_ROSTER_BOTH;
			    else if (!g_strcasecmp(iks_find_attrib(x, "subscription"),"remove"))
				type = J_ROSTER_REMOVE;
			    
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

	signal_emit("jabber", "gui status changed", NULL, "main-gui");
}


void presence_parse(ikspak *pak)
{
	iks *x;
	char *errnum;

	switch(pak->subtype)
	{
		case IKS_TYPE_ERROR:
			print_debug("jabber reports error");
			x = iks_find(pak->x, "error");
			if(x)
			{
				errnum = iks_find_attrib(x, "code");
				if(errnum)
					print_debug("[%s]: ", errnum);
				else
					print_debug(": ");

				print_debug("%s\n", iks_cdata(iks_child(x)));
			}
			else
			{
				print_debug(": No error message!\n");
			}
			break;

		case IKS_TYPE_SUBSCRIBE:
			print_debug("wants to subscribe your presence.\n");
			
			x = iks_make_pres(IKS_TYPE_SUBSCRIBED, 0, iks_id_printx(pak->from,id_print), NULL);
			iks_send(jabber_session->parser, x);
			iks_delete(x);

			x = iks_make_pres(IKS_TYPE_SUBSCRIBE, 0, iks_id_printx(pak->from,id_print), NULL);
			iks_send(jabber_session->parser, x);
			iks_delete(x);

			break;

		case IKS_TYPE_SUBSCRIBED:
			print_debug("accepts your subscription.\n");
			break;

		case IKS_TYPE_UNSUBSCRIBE:

			x = iks_make_pres(IKS_TYPE_UNSUBSCRIBE, 0, iks_id_printx(pak->from,id_print), NULL);
			iks_send(jabber_session->parser, x);
			iks_delete(x);

			print_debug("wants to unsubscribe from your presence.\n");
			break;

		case IKS_TYPE_UNSUBSCRIBED:
			/* WTF ? */
			x = iks_make_pres(IKS_TYPE_UNSUBSCRIBED, 0, iks_id_printx(pak->from,id_print), NULL);
			iks_send(jabber_session->parser, x);
			iks_delete(x);

			print_debug("unsubscribes you from her presence.\n");
			break;
		default:
			print_debug("(unknown presence code)\n");
	}
}
