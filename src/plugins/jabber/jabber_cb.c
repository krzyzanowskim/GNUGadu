/* $Id: jabber_cb.c,v 1.42 2004/05/21 08:58:38 krzyzak Exp $ */

/* 
 * Jabber plugin for GNU Gadu 2 
 * 
 * Copyright (C) 2003-2004 GNU Gadu Team 
 * 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version. 
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include "ggadu_conf.h"
#include "jabber_cb.h"
#include "jabber_protocol.h"

extern jabber_data_type jabber_data;

void jabber_disconnect_cb(LmConnection * connection, LmDisconnectReason reason, gpointer user_data)
{
	if (reason == LM_DISCONNECT_REASON_OK)
	{
		signal_emit_from_thread("jabber", "gui show message", g_strdup(_("Jabber disconnected")), "main-gui");
	} else if (reason == LM_DISCONNECT_REASON_PING_TIME_OUT) {
		signal_emit_from_thread("jabber", "gui show message", g_strdup(_("Connection to the Jabber server timed out")), "main-gui");
	} else if (reason == LM_DISCONNECT_REASON_HUP) {
		signal_emit_from_thread("jabber", "gui show message", g_strdup(_("Connection hung up")), "main-gui");
	} else if (reason == LM_DISCONNECT_REASON_ERROR) {
		signal_emit_from_thread("jabber", "gui show message", g_strdup(_("Generic error somewhere in the transport layer")), "main-gui");
	} else if (reason == LM_DISCONNECT_REASON_UNKNOWN) {
		signal_emit_from_thread("jabber", "gui show message", g_strdup(_("An unknown Jabber error")), "main-gui");
	}

	lm_connection_unregister_message_handler(connection, iq_handler, LM_MESSAGE_TYPE_IQ);
	iq_handler = NULL;
	lm_connection_unregister_message_handler(connection, iq_roster_handler, LM_MESSAGE_TYPE_IQ);
	iq_roster_handler = NULL;
	lm_connection_unregister_message_handler(connection, iq_version_handler, LM_MESSAGE_TYPE_IQ);
	iq_version_handler = NULL;
	lm_connection_unregister_message_handler(connection, presence_handler, LM_MESSAGE_TYPE_PRESENCE);
	presence_handler = NULL;
	lm_connection_unregister_message_handler(connection, message_handler, LM_MESSAGE_TYPE_MESSAGE);
	message_handler = NULL;
	
	jabber_data.status = JABBER_STATUS_UNAVAILABLE;
	signal_emit_from_thread("jabber", "gui disconnected", NULL, "main-gui");

}

static LmHandlerResult register_register_handler(LmMessageHandler * handler, LmConnection * connection, LmMessage * msg,
						GGaduJabberRegister * data)
{
	LmMessageSubType sub_type;
	LmMessageNode *node;

	sub_type = lm_message_get_sub_type(msg);
	switch (sub_type)
	{
	case LM_MESSAGE_SUB_TYPE_RESULT:
		signal_emit("jabber", "gui show message", g_strdup_printf(_("Account:\n%s@%s\ncreated"),data->username,data->server), "main-gui");
		if (data->update_config)
		{
			ggadu_config_var_set(jabber_handler,"jid",g_strdup_printf("%s@%s",data->username,data->server));
			ggadu_config_var_set(jabber_handler,"password",g_strdup(data->password));
			ggadu_config_save(jabber_handler);
		}
		break;
	case LM_MESSAGE_SUB_TYPE_ERROR:
	default:
		node = lm_message_node_find_child(msg->node, "error");
		signal_emit("jabber", "gui show warning", g_strdup(lm_message_node_get_value(node)), "main-gui");
		break;
	}
	g_free(data->username);
	g_free(data->password);
	g_free(data->server);
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

void jabber_register_account_cb(LmConnection * connection, gboolean result, GGaduJabberRegister * gjr)
{
	LmMessage *msg = NULL;
	LmMessageNode *node = NULL;
	LmMessageHandler *handler = NULL;
	gchar *jid = g_strdup_printf("%s@%s", gjr->username, gjr->server);

	msg = lm_message_new_with_sub_type(jid, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
	node = lm_message_node_add_child(msg->node, "query", NULL);

	lm_message_node_set_attribute(node, "xmlns", "jabber:iq:register");
	lm_message_node_add_child(node, "username", gjr->username);
	lm_message_node_add_child(node, "password", gjr->password);

	handler = lm_message_handler_new((LmHandleMessageFunction) register_register_handler, gjr, NULL);
	lm_connection_send_with_reply(connection, msg, handler, NULL);

	lm_message_unref(msg);
	g_free(jid);
}

void connection_auth_cb(LmConnection * connection, gboolean success, gpointer status)
{
	if (!success)
	{
		lm_connection_close(connection, NULL);
		return;
	}

	print_debug("jabber: Authentication succeeded. Changing status...\n");
	jabber_fetch_roster(status);
}

void connection_open_result_cb(LmConnection * connection, gboolean success, gint * status)
{
	gchar *jid = NULL;
	gchar *tmp = NULL;

	if (!success)
	{
		jabber_disconnect_cb(jabber_data.connection,LM_DISCONNECT_REASON_OK, NULL);
		return;
	}

	jid = g_strdup(ggadu_config_var_get(jabber_handler, "jid"));
	if ((tmp = g_strstr_len(jid, strlen(jid), "@")) != NULL)
		tmp[0] = '\0';

	print_debug("jabber: Connection open succeeded. Authenticating... (status %p, jid %s, server %s)\n", status, jid,lm_connection_get_server(connection));
	
	if (!lm_connection_authenticate
	    (connection, jid, ggadu_config_var_get(jabber_handler, "password"),
	     ggadu_config_var_get(jabber_handler, "resource") ? ggadu_config_var_get(jabber_handler,
										     "resource") : "GNU Gadu 2",
	     (LmResultFunction) connection_auth_cb, status, NULL, NULL))
	{
		print_debug("jabber: lm_connection_authenticate() failed.");
		signal_emit("jabber", "gui show message", g_strdup(_("Jabber authentication failed")), "main-gui");
	} else {
/*		signal_emit("jabber", "gui status changed", (gpointer) status, "main-gui");
		jabber_change_status((int)status);
*/
	}
/*      jabber_change_status(status); */

	g_free(jid);
}

LmHandlerResult presence_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			    gpointer user_data)
{
	gchar *jid;
	GGaduContact *k = NULL;
	LmMessageNode *status;
	gchar *descr = NULL;
	gchar *show;
	LmMessageNode *node;
	GSList *list;

	jid = (gchar *) lm_message_node_get_attribute(message->node, "from");

	print_debug("%s", lm_message_node_to_string(message->node));

	if (strchr(jid, '/'))
		strchr(jid, '/')[0] = '\0';

	if (lm_message_get_sub_type(message) == LM_MESSAGE_SUB_TYPE_SUBSCRIBE)
	{
		GGaduDialog *dialog = ggadu_dialog_new_full(GGADU_DIALOG_YES_NO,_("Subscription request confirmation"),"jabber subscribe", g_strdup(jid));
		gchar *msg = g_strdup_printf(_("Person : %s\nwants to subscribe your presence"), jid);

		ggadu_dialog_add_entry(dialog, 0, msg, VAR_NULL, NULL, VAR_FLAG_NONE);
		signal_emit("jabber", "gui show dialog", dialog, "main-gui");

		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
	else if (lm_message_get_sub_type(message) == LM_MESSAGE_SUB_TYPE_SUBSCRIBED)
	{
		gchar *msg = g_strdup_printf(_("From %s\nYou are now authorized"), jid);
		signal_emit("jabber", "gui show message", msg, "main-gui");
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
	else if (lm_message_get_sub_type(message) == LM_MESSAGE_SUB_TYPE_UNSUBSCRIBED)
	{
		gchar *msg = g_strdup_printf(_("From %s\nYour authorization has been removed!"), jid);
		signal_emit("jabber", "gui show message", msg, "main-gui");
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
/*
    else if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_UNSUBSCRIBE)
    {
	gchar *msg = g_strdup_printf (_("%s unsubscribed you from presence"), jid);
	signal_emit ("jabber", "gui show message", msg, "main-gui");
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
    }
*/
	status = lm_message_node_get_child(message->node, "status");

	if (status)
		descr = ggadu_strchomp((gchar *) lm_message_node_get_value(status));

	list = ggadu_repo_get_as_slist("jabber", REPO_VALUE_CONTACT);
	while (list)
	{
		k = (GGaduContact *) list->data;
		if (!ggadu_strcasecmp(k->id, jid))
		{
			gint oldstatus = k->status;
			gchar *olddescr = k->status_descr;

			if (k->status_descr)
				g_free(k->status_descr);

			k->status_descr = NULL;

			switch (lm_message_get_sub_type(message))
			{
			default:
			case LM_MESSAGE_SUB_TYPE_AVAILABLE:
				node = lm_message_node_get_child(message->node, "show");
				show = NULL;
				if (node)
					show = (gchar *) lm_message_node_get_value(node);

				if (show)
				{
					if (!strcmp(show, "away"))
						k->status = JABBER_STATUS_AWAY;
					else if (!strcmp(show, "xa"))
						k->status = JABBER_STATUS_XA;
					else if (!strcmp(show, "dnd"))
						k->status = JABBER_STATUS_DND;
					else if (!strcmp(show, "chat"))
						k->status = JABBER_STATUS_CHAT;
					else
						k->status = JABBER_STATUS_AVAILABLE;
				}
				else
					k->status = JABBER_STATUS_AVAILABLE;

				if (descr)
					k->status_descr = g_strdup(descr);

				break;
			case LM_MESSAGE_SUB_TYPE_ERROR:
				k->status = JABBER_STATUS_ERROR;
				k->status_descr =
					g_strdup(lm_message_node_get_value
						 (lm_message_node_get_child(message->node, "error")));
				break;
			case LM_MESSAGE_SUB_TYPE_UNAVAILABLE:
				k->status = JABBER_STATUS_UNAVAILABLE;

				if (descr)
					k->status_descr = g_strdup(descr);

				break;
			}

			if ((k->status != oldstatus) || (olddescr != k->status_descr))
			{
				//k->status_descr = g_strstrip(k->status_descr);
				ggadu_repo_change_value("jabber", ggadu_repo_key_from_string(k->id), k, REPO_VALUE_DC);
			}

		}
		list = list->next;
	}
	g_slist_free(list);
	return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
}

LmHandlerResult iq_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message, gpointer user_data)
{
	GSList *list = jabber_data.actions;
	gchar *type;
	gchar *id;

	id = (gchar *) lm_message_node_get_attribute(message->node, "id");
	type = (gchar *) lm_message_node_get_attribute(message->node, "type");

	/* first we check if we have waiting actions for the message
	 * if there is one, then run the associated function instead of the code
	 * below. */

	while (list)
	{
		waiting_action *action = (waiting_action *) list->data;
		if (!strcmp(type, action->type) && !strcmp(id, action->id))
		{
			action->func(connection, message, action->data);
			jabber_data.actions = g_slist_remove(jabber_data.actions, action);
			g_free(action->id);
			g_free(action->type);
			g_free(action);
			return LM_HANDLER_RESULT_REMOVE_MESSAGE;
		}
		list = list->next;
	}

	return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
}

LmHandlerResult iq_version_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			      gpointer user_data)
{
	LmMessageNode *node;
	LmMessage *m;
	gchar *from;

	if (!(node = lm_message_node_get_child(message->node, "query")))
	{
		print_debug("jabber : weird roster : %s", lm_message_node_to_string(message->node));
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
	}

	if (strcmp(lm_message_node_get_attribute(node, "xmlns"), "jabber:iq:version"))
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;

	from = (gchar *) lm_message_node_get_attribute(message->node, "from");

	m = lm_message_new_with_sub_type(from, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_RESULT);

	lm_message_node_set_attribute(m->node, "id", lm_message_node_get_attribute(message->node, "id"));

	node = lm_message_node_add_child(m->node, "query", NULL);
	lm_message_node_set_attribute(node, "xmlns", "jabber:iq:version");

	lm_message_node_add_child(node, "name", "GNU Gadu");
	lm_message_node_add_child(node, "version", VERSION);

	lm_connection_send(connection, m, NULL);

	lm_message_unref(m);

	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

LmHandlerResult iq_roster_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message, gpointer data)
{
	LmMessageNode *node;
	LmMessageNode *child;
	int first_time = 0;
	int first_seen = 1;
	GSList *list = ggadu_repo_get_as_slist("jabber", REPO_VALUE_CONTACT);

	if (!list)
		first_time = 1;

	print_debug("%s", lm_message_node_to_string(message->node));

	/* verbose error reporting */
	if (lm_message_get_sub_type(message) == LM_MESSAGE_SUB_TYPE_ERROR)
	{
		if (!(node = lm_message_node_get_child(message->node, "error")))
		{
			print_debug("jabber: weird roster.");
			lm_message_node_unref(node);
			g_slist_free(list);
			return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
		}
		signal_emit("jabber", "gui show warning",
			    g_strdup_printf(_("Error: %s (code %s)"), lm_message_node_get_value(node),
					    lm_message_node_get_attribute(node, "code")), "main-gui");
		lm_message_node_unref(node);
		g_slist_free(list);
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
	}

	if (lm_message_get_sub_type(message) != LM_MESSAGE_SUB_TYPE_SET &&
	    lm_message_get_sub_type(message) != LM_MESSAGE_SUB_TYPE_RESULT)
	{
		print_debug("Type : %s", lm_message_node_get_attribute(message->node, "type"));
		g_slist_free(list);
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
	}

	if (!(node = lm_message_node_get_child(message->node, "query")))
	{
		print_debug("jabber: weird roster.");
		lm_message_node_unref(node);
		g_slist_free(list);
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
	}

	if (strcmp(lm_message_node_get_attribute(node, "xmlns"), "jabber:iq:roster"))
	{
		lm_message_node_unref(node);
		g_slist_free(list);
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
	}

	child = lm_message_node_get_child(node, "item");

	while (child)
	{
		gchar *jid, *name, *subs;
		GGaduContact *k = NULL;

		jid = (gchar *) lm_message_node_get_attribute(child, "jid");
		name = (gchar *) lm_message_node_get_attribute(child, "name");
		subs = (gchar *) lm_message_node_get_attribute(child, "subscription");

		if (!jid)
			continue;

		if (strchr(jid, '/'))
			strchr(jid, '/')[0] = '\0';

		print_debug("jabber: roster: jid= %s ,name= %s ,subscription= %s", jid, name, subs);

		/* filter out transports entries if there is no '@' in jid */		
		if (strchr(jid,'@') == NULL)
		{
			child = child->next;
			continue;
		}

		if (subs && (!strcmp(subs, "remove")))
		{
			g_slist_free(list);
			return LM_HANDLER_RESULT_REMOVE_MESSAGE;
		}

		/* hm... ?  weeeeeee */
		if (list)
		{
			GSList *list2 = list;
			while (list2)
			{
				k = (GGaduContact *) list2->data;
				if (!ggadu_strcasecmp(k->id, jid))
				{
					first_seen = 0;
					if (k->nick)
						g_free(k->nick);
					break;
				}
				k = NULL;
				list2 = list2->next;
			}
		}

		if (!k)
		{
			k = g_new0(GGaduContact, 1);
			k->id = g_strdup(jid);
		}

		k->nick = g_strdup(name ? name : jid);

		/* show all */
		if (first_seen)
			k->status = JABBER_STATUS_UNAVAILABLE;

		if (!ggadu_repo_add_value("jabber", ggadu_repo_key_from_string(k->id), k, REPO_VALUE_CONTACT))
			ggadu_repo_change_value("jabber", ggadu_repo_key_from_string(k->id), k, REPO_VALUE_DC);

		child = child->next;
	}

	signal_emit("jabber", "gui send userlist", NULL, "main-gui");

	/* ZONK ask everybowy from the list about their status, do we need it ? */
	if (first_time)
	{
		GSList *list3 = ggadu_repo_get_as_slist("jabber", REPO_VALUE_CONTACT);

		while (list3)
		{
			LmMessage *m;
			GGaduContact *k = (GGaduContact *) list3->data;

			m = lm_message_new_with_sub_type(k->id, LM_MESSAGE_TYPE_PRESENCE, LM_MESSAGE_SUB_TYPE_PROBE);
			lm_connection_send(connection, m, NULL);
			lm_message_unref(m);

			list3 = list3->next;
		}
		g_slist_free(list3);
	}

	g_slist_free(list);
	lm_message_node_unref(node);
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

LmHandlerResult message_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			   gpointer user_data)
{
	gchar *jid;
	GGaduMsg *msg;
	LmMessageNode *body;

	jid = (gchar *) lm_message_node_get_attribute(message->node, "from");

	if (strchr(jid, '/'))
		strchr(jid, '/')[0] = '\0';

	if (lm_message_get_sub_type(message) == LM_MESSAGE_SUB_TYPE_ERROR)
	{
		print_debug("jabber: Error message : \n%s", lm_message_node_to_string(message->node));
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}

	body = lm_message_node_find_child(message->node, "body");
	if (!body)
	{
		print_debug("jabber: Message from %s without a body.", jid);
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}

	msg = g_new0(GGaduMsg, 1);
	msg->id = g_strdup(jid);
	msg->message = g_strdup(lm_message_node_get_value(body));

	signal_emit("jabber", "gui msg receive", msg, "main-gui");

	if (ggadu_config_var_get(jabber_handler, "log"))
	{
		gchar *line = g_strdup_printf("\n:: %s (%s) ::\n%s\n", msg->id, get_timestamp(0), msg->message);
		ggadu_jabber_save_history(msg->id, line);
		g_free(line);
	}

	lm_message_unref(message);

	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}
