/* $Id: jabber_protocol.c,v 1.39 2004/12/29 13:32:42 krzyzak Exp $ */

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

#include <loudmouth/loudmouth.h>
#include <string.h>

#include "jabber_protocol.h"
#include "jabber_plugin.h"
#include "jabber_login.h"
#include "jabber_cb.h"

extern jabber_data_type jabber_data;

waiting_action *action_queue_add(gchar * id, gchar * type, gpointer action_callback, gchar * data, gboolean str_dup)
{
	waiting_action *action;
	action = g_new0(waiting_action, 1);
	action->id = g_strdup(id);
	action->type = g_strdup(type);
	action->func = action_callback;
	action->data = data ? str_dup ? g_strdup(data) : data : NULL;

	jabber_data.actions = g_slist_append(jabber_data.actions, action);
	return action;
}

void action_queue_del(waiting_action * action)
{
	jabber_data.actions = g_slist_remove(jabber_data.actions, action);
	g_free(action->data);
	g_free(action);
}

void action_roster_add_result(LmConnection * connection, LmMessage * message, gpointer id)
{
	signal_emit("jabber", "gui show message", g_strdup(_("Roster list updated")), "main-gui");
}

void action_roster_fetch_result(LmConnection * connection, LmMessage * message, gpointer user_data)
{
	GGaduStatusPrototype *sp = ggadu_find_status_prototype(p,(gint) user_data);
	jabber_change_status(sp);
	iq_roster_cb(NULL, connection, message, user_data);
}

void action_roster_remove_result(LmConnection * connection, LmMessage * message, gpointer data)
{
	ggadu_repo_del_value("jabber", ggadu_repo_key_from_string((gchar *) data));
	/* GGaduContact_free(k); */
	signal_emit("jabber", "gui send userlist", NULL, "main-gui");
	signal_emit("jabber", "gui show message", g_strdup(_("Contact removed from roster")), "main-gui");
}

void jabber_change_status(GGaduStatusPrototype *sp)
{
	enum states status;
	LmMessage *m = NULL;
	gchar *show = NULL;
	gchar *show_away = "away";
	gchar *show_xa = "xa";
	gchar *show_dnd = "dnd";
	gchar *show_chat = "chat";
	
	print_debug("jabber_change_status start");
	
	if (!sp)
	    return;
	
	status = sp->status;
	
	if ((status == jabber_data.status))
		return;
		
	if (status == JABBER_STATUS_UNAVAILABLE)
	{
		lm_connection_close(jabber_data.connection, NULL);
		return;
	}

	/* connect if switched to any other than unavailable */	
	if ((jabber_data.status == JABBER_STATUS_UNAVAILABLE) && (status != JABBER_STATUS_UNAVAILABLE) &&
	    (status != JABBER_STATUS_DESCR) && (!jabber_data.connection || !lm_connection_is_open(jabber_data.connection)
	    || !lm_connection_is_authenticated(jabber_data.connection)))
	{
		g_thread_create(jabber_login_connect, (gpointer) status, FALSE, NULL);
		return;
	}

	/* Just a simple esthetic functionality */
	if((jabber_data.status == JABBER_STATUS_UNAVAILABLE) && (status == JABBER_STATUS_DESCR))
		signal_emit("jabber", "gui status changed", (gpointer) JABBER_STATUS_UNAVAILABLE, "main-gui");
	
	if (jabber_data.connection && !lm_connection_is_authenticated(jabber_data.connection))
	{
		print_debug("You are not yet authenticated!");
		return;
	}
	
	m = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_PRESENCE,
					 (status == JABBER_STATUS_UNAVAILABLE) ? 
					 LM_MESSAGE_SUB_TYPE_UNAVAILABLE : LM_MESSAGE_SUB_TYPE_AVAILABLE);

	switch (status == JABBER_STATUS_DESCR ? jabber_data.status : status)
	{
	case JABBER_STATUS_AWAY:
		show = show_away;
		break;
	case JABBER_STATUS_XA:
		show = show_xa;
		break;
	case JABBER_STATUS_DND:
		show = show_dnd;
		break;
	case JABBER_STATUS_CHAT:
		show = show_chat;
		break;
	case JABBER_STATUS_AVAILABLE:
		show = NULL;
		break;
	default:
		break;
	}

	if (show)
		lm_message_node_add_child(m->node, "show", show);

	if (jabber_data.description)
		lm_message_node_add_child(m->node, "status", jabber_data.description);

	if (!lm_connection_send(jabber_data.connection, m, NULL))
	{
		print_debug("jabber: Couldn't change status!\n");
	}
	else
	{
		if(status != JABBER_STATUS_DESCR)
		{
			jabber_data.status = status;
			signal_emit("jabber", "gui status changed", (gpointer) status, "main-gui");
		}
	}
	lm_message_unref(m);
	print_debug("jabber_change_status end");
}

void jabber_fetch_roster(gpointer user_data)
{
	LmMessage *m = NULL;
	LmMessageNode *node = NULL;

	print_debug("jabber_fetch_roster a");
	print_debug("jabber: Fetching roster. %s", lm_connection_get_server(jabber_data.connection));

	m = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);
	node = lm_message_node_add_child(m->node, "query", NULL);
	lm_message_node_set_attribute(m->node, "id", "fetch_roster");
	lm_message_node_set_attribute(node, "xmlns", "jabber:iq:roster");

	if (!lm_connection_send(jabber_data.connection, m, NULL))
		print_debug("jabber: Can't fetch roster (lm_connection_send() failed).\n");
	else
		action_queue_add("fetch_roster", "result", action_roster_fetch_result, user_data, FALSE);

	print_debug("jabber_fetch_roster b");
	lm_message_unref(m);
	print_debug("jabber_fetch_roster c");
}

void action_search_form(LmConnection * connection, LmMessage * message, gpointer data)
{
	GGaduDialog *dialog;
	LmMessageNode *node;
	
	dialog =  ggadu_dialog_new_full(GGADU_DIALOG_GENERIC,_("Jabber search: form"),"search", 
			    (gpointer) g_strdup(lm_message_node_get_attribute(message->node, "from")));

	node = lm_message_node_get_child(message->node, "query");
	if (!strcmp(lm_message_node_get_attribute(node, "xmlns"), "jabber:iq:search"))
	{
		/* child_instr = lm_message_node_get_child(node, "instructions"); */   

		if (lm_message_node_get_child(node, "first"))
			ggadu_dialog_add_entry(dialog, GGADU_SEARCH_FIRSTNAME, _("First name:"), VAR_STR, NULL,
					       VAR_FLAG_NONE);
		if (lm_message_node_get_child(node, "last"))
			ggadu_dialog_add_entry(dialog, GGADU_SEARCH_LASTNAME, _("Last name:"), VAR_STR, NULL,
					       VAR_FLAG_NONE);
		if (lm_message_node_get_child(node, "nick"))
			ggadu_dialog_add_entry(dialog, GGADU_SEARCH_NICKNAME, _("Nick:"), VAR_STR, NULL,
					       VAR_FLAG_NONE);
		if (lm_message_node_get_child(node, "email"))
			ggadu_dialog_add_entry(dialog, GGADU_SEARCH_EMAIL, _("Email:"), VAR_STR, NULL,
					       VAR_FLAG_NONE);

		signal_emit("jabber", "gui show dialog", dialog, "main-gui");
	}
}

void action_search_result(LmConnection * connection, LmMessage * message, gpointer data)
{
	LmMessageNode *node;
	LmMessageNode *child;
	GSList *list = NULL;

	node = lm_message_node_get_child(message->node, "query");
	if (!strcmp(lm_message_node_get_attribute(node, "xmlns"), "jabber:iq:search"))
	{
		child = lm_message_node_get_child(node, "item");
		if (!child)
		{
			signal_emit("jabber", "gui show message", g_strdup(_("No users have been found!")), "main-gui");
			return;
		}

		while (child)
		{
			gchar *jid = (gchar *) lm_message_node_get_attribute(child, "jid");
			GGaduContact *k;
			LmMessageNode *child_node;

			k = g_new0(GGaduContact, 1);
			k->id = g_strdup(jid ? jid : "?");


			child_node = lm_message_node_get_child(node, "first");
			if (child_node)
				k->first_name = g_strdup((gchar *) lm_message_node_get_value(child_node));

			child_node = lm_message_node_get_child(node, "last");
			if (child_node)
				k->last_name = g_strdup((gchar *) lm_message_node_get_value(child_node));

			child_node = lm_message_node_get_child(node, "nick");
			if (child_node)
				k->nick = g_strdup((gchar *) lm_message_node_get_value(child_node));

			child_node = lm_message_node_get_child(node, "email");
			if (child_node)
				k->email = g_strdup((gchar *) lm_message_node_get_value(child_node));

			k->status = JABBER_STATUS_UNAVAILABLE;
			list = g_slist_append(list, k);
			child = child->next;
		}

		signal_emit("jabber", "gui show search results", list, "main-gui");
	}
}
