/* $Id: jabber_cb.c,v 1.87 2005/03/04 22:07:05 mkobierzycki Exp $ */

/* 
 * Jabber plugin for GNU Gadu 2 
 * 
 * Copyright (C) 2003-2005 GNU Gadu Team 
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
#include <stdlib.h>
#include "ggadu_conf.h"
#include "jabber_cb.h"
#include "jabber_login.h"
#include "jabber_protocol.h"

extern jabber_data_type jabber_data;

void 	jabber_disconnect_cb(LmConnection * connection, LmDisconnectReason reason, gpointer user_data)
{
	static GStaticMutex connect_mutex = G_STATIC_MUTEX_INIT;
	static gint reconnect_count = 0;
	static gchar *error_message;
	g_static_mutex_lock(&connect_mutex);
	
	if (iq_handler)
	    lm_connection_unregister_message_handler(connection, iq_handler, LM_MESSAGE_TYPE_IQ);
	    
	iq_handler = NULL;

	if (iq_roster_handler)
	    lm_connection_unregister_message_handler(connection, iq_roster_handler, LM_MESSAGE_TYPE_IQ);

	iq_roster_handler = NULL;

	if (iq_version_handler)
	    lm_connection_unregister_message_handler(connection, iq_version_handler, LM_MESSAGE_TYPE_IQ);

	iq_version_handler = NULL;
	
	if (iq_vcard_handler)	
	    lm_connection_unregister_message_handler(connection, iq_vcard_handler, LM_MESSAGE_TYPE_IQ);

	iq_vcard_handler = NULL;

	if (iq_account_data_handler)
	    lm_connection_unregister_message_handler(connection, iq_account_data_handler, LM_MESSAGE_TYPE_IQ);

	iq_account_data_handler = NULL;

	if (presence_handler)
	    lm_connection_unregister_message_handler(connection, presence_handler, LM_MESSAGE_TYPE_PRESENCE);

	presence_handler = NULL;

	if (message_handler)
	    lm_connection_unregister_message_handler(connection, message_handler, LM_MESSAGE_TYPE_MESSAGE);

	message_handler = NULL;
	
	if ((reconnect_count < 3) && (reason != LM_DISCONNECT_REASON_OK))
	{
	    reconnect_count++;
	    g_thread_create(jabber_login_connect, (gpointer) jabber_data.status, FALSE, NULL);
	    g_static_mutex_unlock(&connect_mutex);
	    return;
	}
	
	reconnect_count = 0;
	jabber_data.status = JABBER_STATUS_UNAVAILABLE;

	if (reason == LM_DISCONNECT_REASON_OK)
	{
		error_message = _("Jabber disconnected");
	} else if (reason == LM_DISCONNECT_REASON_PING_TIME_OUT) {
		error_message = _("Connection to the Jabber server timed out");
	} else if (reason == LM_DISCONNECT_REASON_HUP) {
		error_message = _("Connection hung up");
	} else if (reason == LM_DISCONNECT_REASON_ERROR) {
		error_message = _("Generic error somewhere in the transport layer");
	} else if (reason == LM_DISCONNECT_REASON_UNKNOWN) {
		error_message = _("An unknown Jabber error");
	}
	
	lm_connection_close(connection,NULL);

	/* Just remove all contacts from repo. */
	if(ggadu_repo_del("jabber"))
	    ggadu_repo_add("jabber");

	signal_emit_from_thread("jabber", "gui send userlist", NULL, "main-gui");
	signal_emit_from_thread("jabber", "gui show message", g_strdup(error_message), "main-gui");
	signal_emit_from_thread("jabber", "gui disconnected", NULL, "main-gui");

	print_debug("jabber_disconnect_cb 10");
	g_static_mutex_unlock(&connect_mutex);
}

LmHandlerResult register_register_handler(LmMessageHandler * handler, LmConnection * connection, LmMessage * msg,
					  GGaduJabberRegister *data)
{
	LmMessageSubType sub_type;

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
		signal_emit("jabber", "gui show warning", g_strdup(_("Username not available")), "main-gui");
		break;
	}
	g_free(data->username);
	g_free(data->password);
	g_free(data->server);
	g_free(data);
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

static LmHandlerResult register_get_fields_handler(LmMessageHandler *handler, LmConnection * connection, LmMessage *msg,
						   gpointer *data)
{
    LmMessageNode *node;
    GGaduDialog *dialog = ggadu_dialog_new_full(GGADU_DIALOG_CONFIG, _("Register Jabber account"), "register account",
		                                (gpointer) connection);

    node=lm_message_node_find_child(msg->node, "query");
    if(lm_message_node_find_child(node, "username"))
	    ggadu_dialog_add_entry(dialog, GGADU_JABBER_USERNAME, _("Username"), VAR_STR, NULL, VAR_FLAG_NONE);
    if(lm_message_node_find_child(node, "password"))
	    ggadu_dialog_add_entry(dialog, GGADU_JABBER_PASSWORD, _("Password"), VAR_STR, NULL, VAR_FLAG_NONE);
    if(lm_message_node_find_child(node, "name"))
	    ggadu_dialog_add_entry(dialog, GGADU_JABBER_FN, _("Full name"), VAR_STR, NULL, VAR_FLAG_NONE);
    if(lm_message_node_find_child(node, "email"))
	    ggadu_dialog_add_entry(dialog, GGADU_JABBER_USERID, _("E-mail"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(dialog, GGADU_JABBER_UPDATE_CONFIG, _("Update settings on success?"), VAR_BOOL, FALSE,
		   	   VAR_FLAG_NONE);
	    
    signal_emit("jabber", "gui show dialog", dialog, "main-gui");
    return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

void jabber_register_account_cb(LmConnection * connection, gboolean result, gpointer data)
{
    LmMessage *msg;
    LmMessageNode *node;
    LmMessageHandler *handler;

    msg = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);
    lm_message_node_set_attribute(msg->node, "id", "reg1");
    node = lm_message_node_add_child(msg->node, "query", NULL);
    lm_message_node_set_attribute(node, "xmlns", "jabber:iq:register");

    handler = lm_message_handler_new((LmHandleMessageFunction) register_get_fields_handler, NULL, NULL);
    lm_connection_send_with_reply(connection, msg, handler, NULL);
    lm_message_unref(msg);


    return;
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
	jabber_services_discovery_action(NULL);
}

void connection_open_result_cb(LmConnection * connection, gboolean success, gint * status)
{
	gchar *jid = NULL;
	gchar *tmp = NULL;

	if (!success)
	{
//dupa		jabber_disconnect_cb(jabber_data.connection,LM_DISCONNECT_REASON_OK, NULL);
		return;
	}

	jid = g_strdup(ggadu_config_var_get(jabber_handler, "jid"));
	if ((tmp = g_strstr_len(jid, strlen(jid), "@")) != NULL)
		tmp[0] = '\0';

	print_debug("jabber: Connection open succeeded. Authenticating... (status %p, jid %s, server %s)\n", status, jid,lm_connection_get_server(connection));
	
	if (!lm_connection_authenticate
	    (connection, jid, ggadu_config_var_get(jabber_handler, "password"),
	     ggadu_config_var_get(jabber_handler, "resource") ? ggadu_config_var_get(jabber_handler, "resource") :
	     JABBER_DEFAULT_RESOURCE, (LmResultFunction) connection_auth_cb, status, NULL, NULL))
	{
		print_debug("jabber: lm_connection_authenticate() failed.");
		signal_emit("jabber", "gui show message", g_strdup(_("Jabber authentication failed")), "main-gui");
	} else {
/*		signal_emit("jabber", "gui status changed", (gpointer) status, "main-gui");
		jabber_change_status((int)status);
*/
	}

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
	GSList *temp;
	gchar **tab;
	gchar *res;

	jid = (gchar *) lm_message_node_get_attribute(message->node, "from");
	tab = g_strsplit(jid, "/", 2);
	res = tab[1];

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

	status = lm_message_node_get_child(message->node, "status");

	if (status)
		descr = ggadu_strchomp((gchar *) lm_message_node_get_value(status));

	list = ggadu_repo_get_as_slist("jabber", REPO_VALUE_CONTACT);
	temp = list;
	while (temp)
	{
		k = (GGaduContact *) temp->data;
		if (!ggadu_strcasecmp(k->id, jid))
		{
			gint oldstatus = k->status;
			gchar *olddescr = k->status_descr;

			if (k->status_descr)
				g_free(k->status_descr);

			k->status_descr = NULL;
			k->resource = g_strdup(res);

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

			if(g_slist_find(p->offline_status, (gpointer) oldstatus) &&
		           (g_slist_find(p->online_status, (gpointer) k->status) ||
			    g_slist_find(p->away_status, (gpointer) k->status)))
			{
			    jabber_get_version(k);
			}

			if ((k->status != oldstatus) || (olddescr != k->status_descr))
			{
				ggadu_repo_change_value("jabber", ggadu_repo_key_from_string(k->id), k, REPO_VALUE_DC);
			}

		}
		temp = temp->next;
	}

	g_slist_free(list);
	g_strfreev(tab);
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

	print_debug("jabber : %s", lm_message_node_to_string(message->node));

	if (!(node = lm_message_node_get_child(message->node, "query")))
	{
		print_debug("jabber : weird roster : %s", lm_message_node_to_string(message->node));
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
	}

	if (strcmp(lm_message_node_get_attribute(node, "xmlns"), "jabber:iq:version"))
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;

	from = (gchar *) lm_message_node_get_attribute(message->node, "from");

	if(lm_message_get_sub_type(message) == LM_MESSAGE_SUB_TYPE_GET)
	{
		m = lm_message_new_with_sub_type(from, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_RESULT);

		lm_message_node_set_attribute(m->node, "id", lm_message_node_get_attribute(message->node, "id"));

		node = lm_message_node_add_child(m->node, "query", NULL);
		lm_message_node_set_attribute(node, "xmlns", "jabber:iq:version");

		lm_message_node_add_child(node, "name", "GNU Gadu");
		lm_message_node_add_child(node, "version", VERSION);
		if(strlen(OS_NAME))
		    lm_message_node_add_child(node, "os", OS_NAME);

		lm_connection_send(connection, m, NULL);

		lm_message_unref(m);
	}

	if(lm_message_get_sub_type(message) == LM_MESSAGE_SUB_TYPE_RESULT)
	{
		LmMessageNode *node;
		GSList *temp = jabber_data.software;
		gchar *jid = (gchar *) lm_message_node_get_attribute(message->node, "from");

		if(strchr(jid, '/'))
		    strchr(jid, '/')[0] = 0;

		while(temp)
		{
		    if(!ggadu_strcmp(((GGaduJabberSoftware*) temp->data)->jid, jid))
		        break;

		    temp = temp->next;
		}

		node = lm_message_node_find_child(message->node, "name");
		if(((GGaduJabberSoftware*) temp->data)->client)
		{
		    g_free(((GGaduJabberSoftware*) temp->data)->client);
		    ((GGaduJabberSoftware*) temp->data)->client = NULL;
		}
		if(node && lm_message_node_get_value(node))
		    ((GGaduJabberSoftware*) temp->data)->client = g_strdup(lm_message_node_get_value(node));

		node = lm_message_node_find_child(message->node, "version");
		if(((GGaduJabberSoftware*) temp->data)->version)
		{
		    g_free(((GGaduJabberSoftware*) temp->data)->version);
		    ((GGaduJabberSoftware*) temp->data)->version = NULL;
		}
		if(node && lm_message_node_get_value(node))
		    ((GGaduJabberSoftware*) temp->data)->version = g_strdup(lm_message_node_get_value(node));

		node = lm_message_node_find_child(message->node, "os");
		if(((GGaduJabberSoftware*) temp->data)->os)
		{
		    g_free(((GGaduJabberSoftware*) temp->data)->os);
		    ((GGaduJabberSoftware*) temp->data)->os = NULL;
		}
		if(node && lm_message_node_get_value(node))
		    ((GGaduJabberSoftware*) temp->data)->os = g_strdup(lm_message_node_get_value(node));
	}
	
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

LmHandlerResult iq_vcard_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			      gpointer user_data)
{
	print_debug("jabber : %s", lm_message_node_to_string(message->node));
	
	if(!lm_message_node_get_attribute(message->node, "id"))
	        return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;

        if(!strcmp(lm_message_node_get_attribute(message->node, "id"), "v1"))
	{
	    if(lm_message_node_find_child(message->node, "vCard"))
	    {
		LmMessageNode *node;
		GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("Personal info:"), "user edit vcard");
		gchar **tab = NULL;

		node = lm_message_node_find_child(message->node, "GIVEN");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_GIVEN, _("First name"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_NONE);
		node = lm_message_node_find_child(message->node, "FAMILY");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_FAMILY, _("Last name"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_NONE);
		node = lm_message_node_find_child(message->node, "FN");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_FN, _("Full name"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_INSENSITIVE);
		node = lm_message_node_find_child(message->node, "NICKNAME");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_NICKNAME, _("Nick"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_NONE);
		node = lm_message_node_find_child(message->node, "URL");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_URL, _("Homepage"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_NONE);

		node = lm_message_node_find_child(message->node, "BDAY");
		if(node && lm_message_node_get_value(node))
                	tab = g_strsplit(lm_message_node_get_value(node), "-", 3);
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_BDAY, _("Birthday"), VAR_INT,
				       tab ? (gpointer) atoi(tab[2]) : NULL, VAR_FLAG_NONE);
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_BMONTH, _("Month"), VAR_INT,
				       tab ? (gpointer) atoi(tab[1]) : NULL, VAR_FLAG_NONE);
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_BYEAR, _("Year"), VAR_INT,
				       tab ? (gpointer) atoi(tab[0]) : NULL, VAR_FLAG_NONE);
		if(tab)
			g_strfreev(tab);

		node = lm_message_node_find_child(message->node, "ORGNAME");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_ORGNAME, _("Organization"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_NONE);
		node = lm_message_node_find_child(message->node, "NUMBER");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_NUMBER, _("Telephone number"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_NONE);
		node = lm_message_node_find_child(message->node, "LOCALITY");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_LOCALITY, _("Locality"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_NONE);
		node = lm_message_node_find_child(message->node, "CTRY");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_CTRY, _("Country"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_NONE);
		node = lm_message_node_find_child(message->node, "USERID");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_USERID, _("E-mail"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_NONE);

		signal_emit("jabber", "gui show dialog", dialog, "main-gui");
	    }
	    
	    return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}

        if(!strcmp(lm_message_node_get_attribute(message->node, "id"), "v3"))
	{
		LmMessageNode *node;
		gchar *string = g_strdup_printf(_("%s's personal info:"),lm_message_node_get_attribute(message->node, "from"));
		GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_YES_NO, string, "user show vcard");

		ggadu_dialog_set_flags(dialog, GGADU_DIALOG_FLAG_ONLY_OK);
		g_free(string);

		node = lm_message_node_find_child(message->node, "GIVEN");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_GIVEN, _("First name"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_INSENSITIVE);
		node = lm_message_node_find_child(message->node, "FAMILY");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_FAMILY, _("Last name"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_INSENSITIVE);
		node = lm_message_node_find_child(message->node, "FN");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_FN, _("Full name"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_INSENSITIVE);
		node = lm_message_node_find_child(message->node, "NICKNAME");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_NICKNAME, _("Nick"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_INSENSITIVE);
		node = lm_message_node_find_child(message->node, "URL");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_URL, _("Homepage"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_INSENSITIVE);
		node = lm_message_node_find_child(message->node, "BDAY");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_BDAY, _("Birth date"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_INSENSITIVE);
		node = lm_message_node_find_child(message->node, "ORGNAME");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_ORGNAME, _("Organization"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_INSENSITIVE);
		node = lm_message_node_find_child(message->node, "NUMBER");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_NUMBER, _("Telephone number"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_INSENSITIVE);
		node = lm_message_node_find_child(message->node, "LOCALITY");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_LOCALITY, _("Locality"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_INSENSITIVE);
		node = lm_message_node_find_child(message->node, "CTRY");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_CTRY, _("Country"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_INSENSITIVE);
		node = lm_message_node_find_child(message->node, "USERID");
                ggadu_dialog_add_entry(dialog, GGADU_JABBER_USERID, _("E-mail"), VAR_STR,
				       node ? (gpointer) lm_message_node_get_value(node) : NULL,
				       VAR_FLAG_INSENSITIVE);

		signal_emit("jabber", "gui show dialog", dialog, "main-gui");

                return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}

	return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
}

LmHandlerResult iq_account_data_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			      gpointer user_data)
{
	if(!lm_message_node_get_attribute(message->node, "id"))
	        return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;

	if(!strcmp(lm_message_node_get_attribute(message->node, "id"), "change1"))
	{
		if(lm_message_get_sub_type(message) == LM_MESSAGE_SUB_TYPE_RESULT)
		{
			signal_emit("jabber", "gui show message", g_strdup(_("Password successfully changed")), "main-gui");
			ggadu_config_save(jabber_handler);
		}

		if(lm_message_get_sub_type(message) == LM_MESSAGE_SUB_TYPE_ERROR)
		{
			if(lm_message_node_find_child(message->node, "not-authorized"))
				signal_emit("jabber", "gui show warning",
					    g_strdup(_("User is not authorized")), "main-gui");
			if(lm_message_node_find_child(message->node, "not-allowed"))
				signal_emit("jabber", "gui show warning",
					    g_strdup(_("Password change is not allowed by the server")), "main-gui");
			if(lm_message_node_find_child(message->node, "unexpected-request"))
				signal_emit("jabber", "gui show warning",
					    g_strdup(_("You are not registered with the server")), "main-gui");

			ggadu_config_read(jabber_handler);
		}

		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}

	if(!strcmp(lm_message_node_get_attribute(message->node, "id"), "unreg1"))
	{
		if(lm_message_get_sub_type(message) == LM_MESSAGE_SUB_TYPE_RESULT)
		{
			signal_emit("jabber", "gui show message", g_strdup(_("Account successfully removed")), "main-gui");
			ggadu_config_var_set(jabber_handler, "jid", NULL);
			ggadu_config_var_set(jabber_handler, "password", NULL);
			ggadu_config_var_set(jabber_handler, "log", 0);
			ggadu_config_var_set(jabber_handler, "only_friends", 0);
			ggadu_config_var_set(jabber_handler, "autoconnect", 0);
			ggadu_config_var_set(jabber_handler, "use_ssl", 0);
			ggadu_config_save(jabber_handler);
			
			lm_connection_close(jabber_data.connection, NULL);
		}

		if(lm_message_get_sub_type(message) == LM_MESSAGE_SUB_TYPE_ERROR)
		{
			if(lm_message_node_find_child(message->node, "forbidden"))
				signal_emit("jabber", "gui show warning",
					    g_strdup(_("You are not allowed to remove this account")), "main-gui");
			if(lm_message_node_find_child(message->node, "registration-required"))
				signal_emit("jabber", "gui show warning",
					    g_strdup(_("This account was not registered")), "main-gui");
			if(lm_message_node_find_child(message->node, "unexpected-request"))
				signal_emit("jabber", "gui show warning",
					    g_strdup(_("You are not registered with this server")), "main-gui");
		}

		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
	return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
}

LmHandlerResult iq_roster_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message, gpointer data)
{
	LmMessageNode *node = NULL;
	LmMessageNode *child = NULL;
	int first_time = 0;
	int first_seen = 1;
	GSList *list = ggadu_repo_get_as_slist("jabber", REPO_VALUE_CONTACT);

	if (!list)
		first_time = 1;
		
	if (!message)
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;

	if (message && message->node) 
	    print_debug("%s", lm_message_node_to_string(message->node));

	/* verbose error reporting */
	if (lm_message_get_sub_type(message) == LM_MESSAGE_SUB_TYPE_ERROR)
	{
		if (!(node = lm_message_node_get_child(message->node, "error")))
		{
			print_debug("jabber: weird roster.");
/*			lm_message_node_unref(node);
			g_slist_free(list);
			return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
*/			
		}
/*
		signal_emit("jabber", "gui show warning",
			    g_strdup_printf(_("Error: %s (code %s)"), lm_message_node_get_value(node),
					    lm_message_node_get_attribute(node, "code")), "main-gui");
*/
		print_debug("Error: %s (code %s)",lm_message_node_get_value(node),lm_message_node_get_attribute(node, "code"));
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
		/* lm_message_node_unref(node); */
		g_slist_free(list);
		return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
	}

	if (g_ascii_strcasecmp(lm_message_node_get_attribute(node, "xmlns"), "jabber:iq:roster"))
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
			
		if (!strcmp(subs,"from"))
			k->status = JABBER_STATUS_AUTH_FROM;

		if (!strcmp(subs,"none"))
			k->status = JABBER_STATUS_NOAUTH;

		if (!ggadu_repo_add_value("jabber", ggadu_repo_key_from_string(k->id), k, REPO_VALUE_CONTACT))
			ggadu_repo_change_value("jabber", ggadu_repo_key_from_string(k->id), k, REPO_VALUE_DC);

		child = child->next;
	}

	signal_emit("jabber", "gui send userlist", NULL, "main-gui");

	/* ZONK ask everybowy from the list about their status, do we need it ? */
	if (first_time)
	{
		GSList *list3 = ggadu_repo_get_as_slist("jabber", REPO_VALUE_CONTACT);
		GSList *temp = list3;

		while (temp)
		{
			LmMessage *m;
			GGaduContact *k = (GGaduContact *) temp->data;

			jabber_data.software = g_slist_prepend(jabber_data.software, NULL);
			(jabber_data.software)->data = g_new0(GGaduJabberSoftware, 1);
			((GGaduJabberSoftware *) (jabber_data.software)->data)->jid = g_strdup(k->id);

			m = lm_message_new_with_sub_type(k->id, LM_MESSAGE_TYPE_PRESENCE, LM_MESSAGE_SUB_TYPE_PROBE);
			lm_connection_send(connection, m, NULL);
			lm_message_unref(m);

			temp = temp->next;
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

 	if(ggadu_config_var_get(jabber_handler, "only_friends"))
 	{
 	    GSList *roster = ggadu_repo_get_as_slist("jabber", REPO_VALUE_CONTACT);
	    GSList *temp = roster;
 	    gboolean handle_it = FALSE;
 
 	    while(temp)
 	    {
                GGaduContact *k = (GGaduContact *) temp->data;
 		if(!ggadu_strcasecmp(jid, k->id) || lm_message_get_type(message)!=LM_MESSAGE_TYPE_MESSAGE)
 		{
 		    handle_it=TRUE;
 		    break;
 		}
                temp = temp->next;
 	    }
 
 	    g_slist_free(roster);
	    
 	    if(!handle_it) 
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
 	}

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
	msg->time = time(NULL);

	signal_emit("jabber", "gui msg receive", msg, "main-gui");

	ggadu_jabber_save_history(GGADU_HISTORY_TYPE_RECEIVE,msg, msg->id);

	lm_message_unref(message);

	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}
