/* $Id: jabber_cb.c,v 1.20 2004/01/07 23:50:11 thrulliq Exp $ */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include "ggadu_conf.h"
#include "jabber_cb.h"
#include "jabber_protocol.h"

extern jabber_data_type jabber_data;

void connection_auth_cb (LmConnection * connection, gboolean success, gint * status)
{
	if (!success)
	{
		print_debug ("jabber: Authentication failed.\n");
    		signal_emit ("jabber", "gui show message", g_strdup(_("Jabber authentication failed")), "main-gui");
		signal_emit ("jabber", "gui disconnected", NULL, "main-gui");
		return;
	}

	jabber_data.connected = 2;

	print_debug ("jabber: Authentication succeeded. Changing status...\n");

	jabber_change_status (*(gint *) & status);

	print_debug ("jabber: Fetching roster.\n");

	jabber_fetch_roster ();
}

void connection_open_result_cb (LmConnection * connection, gboolean success, gint * status)
{
	gchar *jid = NULL;

	if (!success)
	{
		print_debug ("jabber: Connection failed.\n");
		signal_emit ("jabber", "gui disconnected", NULL, "main-gui");
		return;
	}

	jabber_data.connected = 1;
	print_debug ("jabber: Connection succeeded. Authenticating... (%p)\n", status);

	jid = g_strdup (ggadu_config_var_get (jabber_handler, "jid"));
	strchr (jid, '@')[0] = '\0';

	if (!lm_connection_authenticate
	    (connection, jid, ggadu_config_var_get (jabber_handler, "password"),
	     ggadu_config_var_get (jabber_handler, "resource") ? ggadu_config_var_get (jabber_handler, "resource") : "GNU Gadu 2",
	     (LmResultFunction) connection_auth_cb, status, NULL, NULL))
	{
		print_debug ("jabber: lm_connection_authenticate() failed.\n");
    		signal_emit ("jabber", "gui show message", g_strdup(_("Jabber authentication failed")), "main-gui");
	}

	g_free (jid);
}

LmHandlerResult presence_cb (LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			     gpointer user_data)
{
	gchar *jid;
	GSList *list = jabber_data.userlist;
	GGaduContact *k = NULL;
	LmMessageNode *status;
	gchar *descr = NULL;
	gchar *show;
	LmMessageNode *node;

	jid = (gchar *) lm_message_node_get_attribute (message->node, "from");

	print_debug ("\n%s", lm_message_node_to_string (message->node));

	if (strchr (jid, '/'))
		strchr (jid, '/')[0] = '\0';

	if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_SUBSCRIBE)
	{
		GGaduDialog *d = ggadu_dialog_new ();
		gchar *msg = g_strdup_printf (_("Person : %s\nwants to subscribe your presence"), jid);

		ggadu_dialog_set_title (d, _("Subscription request confirmation"));
		ggadu_dialog_set_type (d, GGADU_DIALOG_YES_NO);
		ggadu_dialog_callback_signal (d, "jabber subscribe");
		ggadu_dialog_add_entry (&(d->optlist), 0, msg, VAR_NULL, NULL, VAR_FLAG_NONE);
		d->user_data = g_strdup (jid);
		signal_emit ("jabber", "gui show dialog", d, "main-gui");

		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
	else if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_SUBSCRIBED)
	{
		gchar *msg = g_strdup_printf (_("From %s\nYou are now authorized"), jid);
		signal_emit ("jabber", "gui show message", msg, "main-gui");
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
    else if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_UNSUBSCRIBED)
	{
		gchar *msg = g_strdup_printf (_("From %s\nYour authorization has been removed!"), jid);
		signal_emit ("jabber", "gui show message", msg, "main-gui");
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
/*	else if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_UNSUBSCRIBE)
	{
		gchar *msg = g_strdup_printf (_("%s unsubscribed you from presence"), jid);
		signal_emit ("jabber", "gui show message", msg, "main-gui");
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	} */


    status = lm_message_node_get_child (message->node, "status");
	if (status)
		descr = (gchar *) lm_message_node_get_value (status);

	while (list)
	{
		k = (GGaduContact *) list->data;
		if (!ggadu_strcasecmp (k->id, jid))
		{
			gint oldstatus = k->status;
			gchar *olddescr = k->status_descr;
            
			if (k->status_descr)
				g_free (k->status_descr);
            
			k->status_descr = NULL;

			switch (lm_message_get_sub_type (message))
			{
			default:
			case LM_MESSAGE_SUB_TYPE_AVAILABLE:
				node = lm_message_node_get_child (message->node, "show");
				show = NULL;
				if (node)
					show = (gchar *) lm_message_node_get_value (node);

				if (show)
				{
					if (!strcmp (show, "away"))
						k->status = JABBER_STATUS_AWAY;
					else if (!strcmp (show, "xa"))
						k->status = JABBER_STATUS_XA;
					else if (!strcmp (show, "dnd"))
						k->status = JABBER_STATUS_DND;
					else if (!strcmp (show, "chat"))
						k->status = JABBER_STATUS_CHAT;
					else
						k->status = JABBER_STATUS_AVAILABLE;
				}
				else
					k->status = JABBER_STATUS_AVAILABLE;

				if (descr)
					k->status_descr = g_strdup (descr);

				break;
			case LM_MESSAGE_SUB_TYPE_ERROR:
                k->status = JABBER_STATUS_ERROR;
                k->status_descr = g_strdup(lm_message_node_get_value (lm_message_node_get_child (message->node, "error")));
                break;
			case LM_MESSAGE_SUB_TYPE_UNAVAILABLE:
				k->status = JABBER_STATUS_UNAVAILABLE;

				if (descr)
					k->status_descr = g_strdup (descr);

				break;
			}

			if ((k->status != oldstatus) || (olddescr != k->status_descr))
				ggadu_repo_change_value ("jabber", k->id, k, REPO_VALUE_DC);
		}
		list = list->next;
	}

	return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
}

LmHandlerResult iq_cb (LmMessageHandler * handler, LmConnection * connection, LmMessage * message, gpointer user_data)
{
	GSList *list = jabber_data.actions;
	gchar *type;
	gchar *id;

	id = (gchar *) lm_message_node_get_attribute (message->node, "id");
	type = (gchar *) lm_message_node_get_attribute (message->node, "type");

	/* first we check if we have waiting actions for the message
	 * if there is one, then run the associated function instead of the code
	 * below. */

	while (list)
	{
		waiting_action *action = (waiting_action *) list->data;
		if (!strcmp (type, action->type) && !strcmp (id, action->id))
		{
			action->func (connection, message, action->data);
			jabber_data.actions = g_slist_remove (jabber_data.actions, action);
			g_free (action->id);
			g_free (action->type);
			g_free (action);
			return LM_HANDLER_RESULT_REMOVE_MESSAGE;
		}
		list = list->next;
	}

	return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
}

LmHandlerResult iq_version_cb (LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			       gpointer user_data)
{
	LmMessageNode *node;
	LmMessage *m;
    gchar *from;

	if (!(node = lm_message_node_get_child (message->node, "query")))
	{
		print_debug ("jabber: weird roster.\n%s", lm_message_node_to_string (message->node));
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
	}

	if (strcmp (lm_message_node_get_attribute (node, "xmlns"), "jabber:iq:version"))
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;

	from = (gchar *) lm_message_node_get_attribute (message->node, "from");

	m = lm_message_new_with_sub_type (from, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_RESULT);

	lm_message_node_set_attribute (m->node, "id", lm_message_node_get_attribute (message->node, "id"));

	node = lm_message_node_add_child (m->node, "query", NULL);
	lm_message_node_set_attribute (node, "xmlns", "jabber:iq:version");

	lm_message_node_add_child (node, "name", "GNU Gadu");
	lm_message_node_add_child (node, "version", VERSION);

	print_debug ("%s", lm_message_node_to_string (m->node));
	lm_connection_send (connection, m, NULL);

	lm_message_unref (m);

	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

LmHandlerResult iq_roster_cb (LmMessageHandler * handler, LmConnection * connection, LmMessage * message, gpointer data)
{
	LmMessageNode *node;
	LmMessageNode *child;
	int first_time = 0;
	int first_seen = 1;

	if (!jabber_data.userlist)
		first_time = 1;

	print_debug ("%s", lm_message_node_to_string (message->node));

	if (lm_message_get_sub_type (message) != LM_MESSAGE_SUB_TYPE_SET &&
	    lm_message_get_sub_type (message) != LM_MESSAGE_SUB_TYPE_RESULT)
	{
		print_debug ("%s", lm_message_node_get_attribute (message->node, "type"));
		print_debug ("xml: %s", lm_message_node_to_string (message->node));
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
	}

	if (!(node = lm_message_node_get_child (message->node, "query")))
	{
		print_debug ("jabber: weird roster.");
        lm_message_node_unref(node);
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
	}


	if (strcmp (lm_message_node_get_attribute (node, "xmlns"), "jabber:iq:roster")) {
        lm_message_node_unref(node);
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
    }

	child = lm_message_node_get_child (node, "item");

	while (child)
	{
		gchar *jid, *name, *subs;
		GGaduContact *k = NULL;

		jid = (gchar *) lm_message_node_get_attribute (child, "jid");
		name = (gchar *) lm_message_node_get_attribute (child, "name");
		subs = (gchar *) lm_message_node_get_attribute (child, "subscription");

		if (!jid)
			continue;

		if (strchr (jid, '/'))
			strchr (jid, '/')[0] = '\0';

		print_debug ("jabber: roster: %s -> %s with %s", jid, name, subs);

        /* REMOVE */
		if (subs && (!strcmp (subs, "remove")))
		{
            return LM_HANDLER_RESULT_REMOVE_MESSAGE;
        }

        /* co to kurwa jest ? tu sprawdzac raczej czy id=roster_1 bo to jest lista kontaktow ? weeeeeee*/
		if (jabber_data.userlist)
		{
			GSList *list = jabber_data.userlist;
			while (list)
			{
				k = (GGaduContact *) list->data;
				if (!ggadu_strcasecmp (k->id, jid))
				{
					first_seen = 0;
					if (k->nick)
						g_free (k->nick);
					break;
				}
				k = NULL;
                list = list->next;
			}
		}

		if (!k)
		{
			k = g_new0 (GGaduContact, 1);
			k->id = g_strdup (jid);
			jabber_data.userlist = g_slist_append (jabber_data.userlist, k);
		}
    
		k->nick = g_strdup (name ? name : jid);

         /* pokazuj wszystkie */
			if (first_seen)
			{
/*				if (!strcmp (subs, "to"))
					k->status = JABBER_STATUS_WAIT_SUBSCRIBE;
				else
*/
					k->status = JABBER_STATUS_UNAVAILABLE;
			}

			if (!g_slist_find (jabber_data.userlist, k))
				jabber_data.userlist = g_slist_append (jabber_data.userlist, k);

			if (!ggadu_repo_add_value ("jabber", k->id, k, REPO_VALUE_CONTACT))
				ggadu_repo_change_value ("jabber", k->id, k, REPO_VALUE_DC);

		child = child->next;
	}

	signal_emit ("jabber", "gui send userlist", jabber_data.userlist, "main-gui");

    /* pytanie o status kolesi ktorzy znalezli sie na liscie */
    if (first_time)
	{
		GSList *list;

		list = jabber_data.userlist;
		while (list)
		{
			LmMessage *m;
			GGaduContact *k = (GGaduContact *) list->data;

			m = lm_message_new_with_sub_type (k->id, LM_MESSAGE_TYPE_PRESENCE, LM_MESSAGE_SUB_TYPE_PROBE);
			lm_connection_send (connection, m, NULL);
			lm_message_unref (m);

			list = list->next;
		}
	}

    lm_message_node_unref(node);
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

LmHandlerResult message_cb (LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			    gpointer user_data)
{
	gchar *jid;
	GGaduMsg *msg;
	LmMessageNode *body;

	jid = (gchar *) lm_message_node_get_attribute (message->node, "from");

	if (strchr (jid, '/'))
		strchr (jid, '/')[0] = '\0';

	if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_ERROR)
	{
		print_debug ("jabber: Error message.\n");
		print_debug ("The message is:\n%s", lm_message_node_to_string (message->node));
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}

	body = lm_message_node_find_child (message->node, "body");
	if (!body)
	{
		print_debug ("jabber: Message from %s without a body.\n", jid);
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}

	msg = g_new0 (GGaduMsg, 1);
	msg->id = g_strdup (jid);
	msg->message = g_strdup (lm_message_node_get_value (body));

	signal_emit ("jabber", "gui msg receive", msg, "main-gui");

	if (ggadu_config_var_get (jabber_handler, "log"))
	{
		gchar *line = g_strdup_printf ("\n:: %s (%s) ::\n%s\n", msg->id, get_timestamp (0), msg->message);
		ggadu_jabber_save_history (msg->id, line);
		g_free (line);
	}

	lm_message_unref (message);

	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}
