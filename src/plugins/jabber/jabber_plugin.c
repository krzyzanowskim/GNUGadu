/* $Id: jabber_plugin.c,v 1.85 2004/08/02 11:13:59 krzyzak Exp $ */

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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ggadu_conf.h"
#include "jabber_plugin.h"
#include "jabber_login.h"
#include "jabber_protocol.h"
#include "jabber_cb.h"

GGaduPlugin *jabber_handler;

LmMessageHandler *iq_handler;
LmMessageHandler *iq_roster_handler;
LmMessageHandler *iq_version_handler;
LmMessageHandler *presence_handler;
LmMessageHandler *message_handler;

static GQuark SEARCH_SERVER_SIG;
static GQuark JABBER_SUBSCRIBE_SIG;
static GQuark CHANGE_STATUS_SIG;
static GQuark CHANGE_STATUS_DESCR_SIG;
static GQuark SEND_MESSAGE_SIG;
static GQuark ADD_USER_SIG;
static GQuark UPDATE_CONFIG_SIG;
static GQuark SEARCH_SIG;
static GQuark GET_CURRENT_STATUS_SIG;
static GQuark GET_USER_MENU_SIG;
static GQuark REGISTER_ACCOUNT;
static GQuark USER_REMOVE_SIG;

jabber_data_type jabber_data;

GGaduProtocol *p;
GGaduMenu *jabbermenu;

GGadu_PLUGIN_INIT("jabber", GGADU_PLUGIN_TYPE_PROTOCOL);

gpointer user_view_history_action(gpointer user_data)
{
	gsize length, terminator;
	GIOChannel *ch = NULL;
	gchar *line = NULL;
	gchar *path = NULL;
	GString *hist_buf = g_string_new(NULL);
	GSList *users = (GSList *) user_data;
	GGaduContact *k = (users) ? (GGaduContact *) users->data : NULL;

	if (!k)
	{
		g_string_free(hist_buf, TRUE);
		return NULL;
	}

	path = g_build_filename(config->configdir, "jabber_history", k->id, NULL);
	ch = g_io_channel_new_file(path, "r", NULL);
	g_free(path);

	if (!ch)
	{
		g_string_free(hist_buf, TRUE);
		return NULL;
	}

	g_io_channel_set_encoding(ch, "ISO-8859-2", NULL);

	while (g_io_channel_read_line(ch, &line, &length, &terminator, NULL) != G_IO_STATUS_EOF)
	{
		if (line != NULL)
			g_string_append(hist_buf, line);
	}
	g_io_channel_shutdown(ch, TRUE, NULL);
	g_io_channel_unref(ch);

	signal_emit(GGadu_PLUGIN_NAME, "gui show window with text", hist_buf->str, "main-gui");
	g_string_free(hist_buf, TRUE);

	return NULL;
}


gpointer user_chat_action(gpointer user_data)
{
	GSList *users = (GSList *) user_data;

	if (!users)
		return NULL;

	if (g_slist_length(users) > 1)
	{
		/* TODO: do this, when conference is being handled */
		print_debug("Conferences are not supported yet! Shot zapal for this ;>.\n");
	}
	else
	{
		GGaduMsg *msg = g_new0(GGaduMsg, 1);
		GGaduContact *k = (GGaduContact *) users->data;
		msg->class = GGADU_CLASS_CHAT;
		msg->id = k->id;
		signal_emit("jabber", "gui msg receive", msg, "main-gui");
	}

	return NULL;
}

gpointer user_add_action(gpointer user_data)
{
	GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_GENERIC, _("Add contact"), "add user");
	ggadu_dialog_add_entry(dialog, GGADU_ID, _("Jabber ID (jid)"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_NICK, _("Nickname"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_REQUEST_AUTH_FROM, _("Request authorization from"), VAR_BOOL, (gpointer)TRUE, VAR_FLAG_NONE);

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
	return NULL;
}

gpointer user_edit_action(gpointer user_data)
{
	GSList *user = (GSList *) user_data;
	GGaduDialog *dialog = NULL;
	GGaduContact *k = NULL;

	if (!user)
		return NULL;

	dialog = ggadu_dialog_new(GGADU_DIALOG_GENERIC, _("Edit contact"), "add user");
	k = (GGaduContact *) user->data;
	ggadu_dialog_add_entry(dialog, GGADU_ID, _("Jabber ID (jid)"), VAR_STR, k->id, VAR_FLAG_INSENSITIVE);
	ggadu_dialog_add_entry(dialog, GGADU_NICK, _("Nickname"), VAR_STR, k->nick, VAR_FLAG_NONE);

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
	return NULL;
}

gpointer user_ask_remove_action(gpointer user_data)
{
        GGaduDialog *dialog;
	                                                                                                               
	dialog = ggadu_dialog_new_full(GGADU_DIALOG_YES_NO, _("Are You sure You want to delete selected user(s)?"), 
					"user remove action", user_data);
	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
	return NULL;
}

gpointer user_remove_action(gpointer user_data)
{
	GSList *users = (GSList *) user_data;

	while (users)
	{
		GGaduContact *k = (GGaduContact *) users->data;

		if (k)
		{
			if (lm_connection_is_authenticated(jabber_data.connection))
			{
				LmMessage *m;
				LmMessageNode *node;

				m = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);

				node = lm_message_node_add_child(m->node, "query", NULL);
				lm_message_node_set_attributes(node, "xmlns", "jabber:iq:roster", NULL);
				lm_message_node_set_attribute(m->node, "id", "roster_remove");

				node = lm_message_node_add_child(node, "item", NULL);
				lm_message_node_set_attributes(node, "jid", g_strdup(k->id), "subscription", "remove",
							       NULL);

				if (lm_connection_send(jabber_data.connection, m, NULL))
				{
					print_debug("send remove request");
					action_queue_add("roster_remove", "result", action_roster_remove_result, k->id,
							 TRUE);
				}
				else
				{
					print_debug("jabber: Can't send!\n");
				}

				lm_message_unref(m);
			}
		}

		users = users->next;
	}

	return NULL;
}

gpointer user_resend_auth_to(gpointer user_data)
{
	GSList *user = (GSList *) user_data;
	GGaduContact *k = (GGaduContact *) user->data;
	LmMessage *m = lm_message_new_with_sub_type(k->id, LM_MESSAGE_TYPE_PRESENCE, LM_MESSAGE_SUB_TYPE_SUBSCRIBED);
	lm_connection_send(jabber_data.connection, m, NULL);
	lm_message_unref(m);
	return NULL;
}

gpointer user_rerequest_auth_from(gpointer user_data)
{
	GSList *user = (GSList *) user_data;
	GGaduContact *k = NULL;
	LmMessage *m = NULL;
	
	if (!user) return NULL;
	
	k = (GGaduContact *) user->data;
	m = lm_message_new_with_sub_type(k->id, LM_MESSAGE_TYPE_PRESENCE, LM_MESSAGE_SUB_TYPE_SUBSCRIBE);
	lm_connection_send(jabber_data.connection, m, NULL);
	lm_message_unref(m);
	return NULL;
}

gpointer user_remove_auth_from(gpointer user_data)
{
	GSList *user = (GSList *) user_data;
	GGaduContact *k = (GGaduContact *) user->data;
	LmMessage *m = lm_message_new_with_sub_type(k->id, LM_MESSAGE_TYPE_PRESENCE, LM_MESSAGE_SUB_TYPE_UNSUBSCRIBED);
	lm_connection_send(jabber_data.connection, m, NULL);
	lm_message_unref(m);
	return NULL;
}

gpointer jabber_register_account_dialog(gpointer user_data)
{
	GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("Register Jabber account"), "register account");

	ggadu_dialog_add_entry(dialog, GGADU_JABBER_SERVER, _("Server : "), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_USERNAME, _("Username : "), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_PASSWORD, _("Password : "), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_UPDATE_CONFIG, _("Update settings on success?"), VAR_BOOL, FALSE,
				VAR_FLAG_NONE);

	signal_emit("jabber", "gui show dialog", dialog, "main-gui");

	return NULL;
}

GGaduMenu *build_userlist_menu(void)
{
	GGaduMenu *menu = ggadu_menu_create();
	GGaduMenu *listmenu;

	ggadu_menu_add_submenu(menu, ggadu_menu_new_item(_("Chat"), user_chat_action, NULL));
	ggadu_menu_add_submenu(menu, ggadu_menu_new_item(_("View History"), user_view_history_action, NULL));

	ggadu_menu_add_submenu(menu, ggadu_menu_new_item("", NULL, NULL));
	ggadu_menu_add_user_menu_extensions(menu,jabber_handler);

	ggadu_menu_add_submenu(menu, ggadu_menu_new_item("", NULL, NULL));
	
	listmenu = ggadu_menu_new_item(_("Authorization"), NULL, NULL);
	ggadu_menu_add_submenu(listmenu,
			       ggadu_menu_new_item(_("Request presence subscription"), user_rerequest_auth_from, NULL));
	ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Send presence authorization"), user_resend_auth_to, NULL));
	ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item("", NULL, NULL));
	ggadu_menu_add_submenu(listmenu,
			       ggadu_menu_new_item(_("Remove authorization"), user_remove_auth_from, NULL));
	ggadu_menu_add_submenu(menu, listmenu);

	
	ggadu_menu_add_submenu(menu, ggadu_menu_new_item("", NULL, NULL));
	ggadu_menu_add_submenu(menu, ggadu_menu_new_item(_("Edit"), user_edit_action, NULL));
	ggadu_menu_add_submenu(menu, ggadu_menu_new_item(_("Remove"), user_ask_remove_action, NULL));
	ggadu_menu_add_submenu(menu, ggadu_menu_new_item("", NULL, NULL));
	ggadu_menu_add_submenu(menu, ggadu_menu_new_item(_("Add New"), user_add_action, NULL));


	return menu;
}

void jabber_signal_recv(gpointer name, gpointer signal_ptr)
{
	GGaduSignal *signal = (GGaduSignal *) signal_ptr;

	print_debug("%s: receive signal %d %s", GGadu_PLUGIN_NAME, signal->name, g_quark_to_string(signal->name));

	if (signal->name == UPDATE_CONFIG_SIG)
	{
		GGaduDialog *dialog = signal->data;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			GSList *tmplist = ggadu_dialog_get_entries(dialog);
			while (tmplist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
				switch (kv->key)
				{
				case GGADU_JABBER_JID:
					ggadu_config_var_set(jabber_handler, "jid", kv->value);
					break;
				case GGADU_JABBER_PASSWORD:
					ggadu_config_var_set(jabber_handler, "password", kv->value);
					break;
				case GGADU_JABBER_LOG:
					ggadu_config_var_set(jabber_handler, "log", kv->value);
					break;
				case GGADU_JABBER_AUTOCONNECT:
					ggadu_config_var_set(jabber_handler, "autoconnect", kv->value);
					break;
				case GGADU_JABBER_USESSL:
					ggadu_config_var_set(jabber_handler, "use_ssl", kv->value);
					break;
				case GGADU_JABBER_RESOURCE:
					ggadu_config_var_set(jabber_handler, "resource", kv->value);
					break;
				case GGADU_JABBER_SERVER:
					ggadu_config_var_set(jabber_handler, "server", kv->value);
					break;
				case GGADU_JABBER_PROXY:
			       		ggadu_config_var_set(jabber_handler, "proxy", kv->value);
					break;
				}
				tmplist = tmplist->next;
			}
			ggadu_config_save(jabber_handler);
		}
		GGaduDialog_free(dialog);
	}
	else if (signal->name == REGISTER_ACCOUNT)
	{
		GGaduDialog *dialog = signal->data;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			GGaduJabberRegister *gjr = g_new0(GGaduJabberRegister, 1);
			GSList *tmplist = ggadu_dialog_get_entries(dialog);

			while (tmplist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
				switch (kv->key)
				{
				case GGADU_JABBER_SERVER:
					gjr->server = kv->value;
					break;
				case GGADU_JABBER_PASSWORD:
					gjr->password = kv->value;
					break;
				case GGADU_JABBER_USERNAME:
					gjr->username = kv->value;
					break;
				case GGADU_JABBER_UPDATE_CONFIG:
					gjr->update_config = (gboolean) kv->value;
					break;
				}
				tmplist = tmplist->next;
			}

			if (gjr->server && gjr->password && gjr->username)
			{
				LmConnection *con = lm_connection_new(gjr->server);
				lm_connection_open(con, (LmResultFunction) jabber_register_account_cb, gjr, NULL, NULL);
			}
			else
			{
				signal_emit("jabber", "gui show warning", g_strdup(_("Invalid data passed, try again")),
					    "main-gui");
				g_free(gjr);
			}
		}
		GGaduDialog_free(dialog);
	}
	else if (signal->name == CHANGE_STATUS_DESCR_SIG)
	{
		GGaduDialog *dialog = signal->data;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK && dialog->user_data)
		{
			GGaduStatusPrototype *sp = dialog->user_data;
			GGaduKeyValue *kv = NULL;
			
			if (ggadu_dialog_get_entries(dialog))
			{
				gchar *desc_utf = NULL;
				kv = (GGaduKeyValue *) ggadu_dialog_get_entries(dialog)->data;
				desc_utf = kv->value;

				if (jabber_data.status_descr)
					g_free(jabber_data.status_descr);
				
				jabber_data.status_descr = g_strdup(desc_utf);
				jabber_change_status(sp->status);
			}
			signal_emit(GGadu_PLUGIN_NAME, "gui status changed", 
				(gpointer) sp->status, "main-gui");
			
		}
		GGaduDialog_free(dialog);
	}
	else if (signal->name == CHANGE_STATUS_SIG)
	{
		GGaduStatusPrototype *sp = signal->data;

		if (!sp)
			return;

		if (sp->status == JABBER_STATUS_DESCR)
		{
			GGaduDialog *dialog =
				ggadu_dialog_new_full(GGADU_DIALOG_GENERIC, _("Enter status description"),
						       "change status descr", sp);

			ggadu_dialog_add_entry(dialog, 0, _("Description:"), VAR_STR, jabber_data.status_descr,
						VAR_FLAG_FOCUS);
			signal_emit("jabber", "gui show dialog", dialog, "main-gui");
			jabber_change_status(jabber_data.status);
			return;
		}

		jabber_change_status(sp->status);
	}
	else if (signal->name == GET_CURRENT_STATUS_SIG)
	{
		signal->data_return = (gpointer) jabber_data.status;
	}
	else if (signal->name == SEND_MESSAGE_SIG)
	{
		GGaduMsg *msg = signal->data;

		if (msg && lm_connection_is_authenticated(jabber_data.connection))
		{
			LmMessage *m;
			gboolean result;
			GError *error = NULL;

			m = lm_message_new_with_sub_type(msg->id, LM_MESSAGE_TYPE_MESSAGE, LM_MESSAGE_SUB_TYPE_CHAT);
			lm_message_node_add_child(m->node, "body", msg->message);
			result = lm_connection_send(jabber_data.connection, m, &error);
			if (!result)
			{
				print_debug("jabber: Can't send!\n");
			}

			ggadu_jabber_save_history(GGADU_HISTORY_TYPE_SEND,msg, msg->id);

			lm_message_unref(m);
		}
	}
	else if (signal->name == ADD_USER_SIG)
	{
		GGaduDialog *dialog = (GGaduDialog *) signal->data;
		GGaduContact *k = NULL;
		GSList *entry = NULL;
		LmMessage *m = NULL;
		LmMessageNode *node_query, *node_item;
		gboolean send_request = FALSE;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			k = g_new0(GGaduContact, 1);

			entry = ggadu_dialog_get_entries(dialog);
			while (entry)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) entry->data;

				switch ((gint) kv->key)
				{
				case GGADU_ID:
					k->id = g_strdup((gchar *) kv->value);
					break;
				case GGADU_NICK:
					if (kv->value && (((gchar *) kv->value)[0] != '\0'))
						k->nick = g_strdup((gchar *) kv->value);
					break;
				case GGADU_JABBER_REQUEST_AUTH_FROM:
					if ((gboolean)kv->value == TRUE)
						send_request = TRUE;
						/*user_rerequest_auth_from(k);*/
					break;
				}

				entry = entry->next;
			}

			m = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
			lm_message_node_set_attribute(m->node, "id", "roster_add");

			node_query = lm_message_node_add_child(m->node, "query", NULL);
			lm_message_node_set_attribute(node_query, "xmlns", "jabber:iq:roster");

			node_item = lm_message_node_add_child(node_query, "item", NULL);
			lm_message_node_set_attribute(node_item, "jid", k->id);

			if (k->nick)
				lm_message_node_set_attribute(node_item, "name", k->nick);

			if (lm_connection_send(jabber_data.connection, m, NULL))
			{
				print_debug("Add : %s", k->id);
				action_queue_add("roster_add", "result", action_roster_add_result, k->id, TRUE);
			}
			else
			{
				print_debug("jabber: Couldn't send.");
			}
			
			if (send_request) {
				GSList *listtmp = g_slist_append(NULL,k);
				user_rerequest_auth_from(listtmp);
				user_resend_auth_to(listtmp);
				g_slist_free(listtmp);
			}
			
			lm_message_unref(m);
			GGaduContact_free(k);
		}

		GGaduDialog_free(dialog);
	}
	else if (signal->name == JABBER_SUBSCRIBE_SIG)
	{

		GGaduDialog *dialog = signal->data;
		gchar *jid = dialog->user_data;
		LmMessage *msg;

		if (jid && ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			msg = lm_message_new_with_sub_type(jid, LM_MESSAGE_TYPE_PRESENCE,
							   LM_MESSAGE_SUB_TYPE_SUBSCRIBED);
		}
		else if (ggadu_dialog_get_response(dialog) == GGADU_CANCEL)
		{
			msg = lm_message_new_with_sub_type(jid, LM_MESSAGE_TYPE_PRESENCE,
							   LM_MESSAGE_SUB_TYPE_UNSUBSCRIBED);
		}

		lm_connection_send(jabber_data.connection, msg, NULL);
		lm_message_unref(msg);
		if (jid)
			g_free(jid);
			
		GGaduDialog_free(dialog);

	}
	else if (signal->name == GET_USER_MENU_SIG)
	{
		GGaduMenu *menu = build_userlist_menu();
		signal->data_return = menu;
	}
	else if (signal->name == SEARCH_SERVER_SIG)
	{
		GGaduDialog *dialog = signal->data;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			GSList *tmplist = ggadu_dialog_get_entries(dialog);
			while (tmplist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
				switch (kv->key)
				{
				case 0:
					if (kv->value)
					{
						LmMessage *message;
						LmMessageNode *node;

						message =
							lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ,
										     LM_MESSAGE_SUB_TYPE_GET);

						lm_message_node_set_attributes(message->node, "to", kv->value, "id",
									       "search1", NULL);
						node = lm_message_node_add_child(message->node, "query", NULL);
						lm_message_node_set_attribute(node, "xmlns", "jabber:iq:search");

						ggadu_config_var_set(jabber_handler, "search_server", kv->value);
						ggadu_config_save(jabber_handler);

						action_queue_add("search1", "result", action_search_form, NULL, FALSE);

						if (!lm_connection_send(jabber_data.connection, message, NULL))
						{
							signal_emit("jabber", "gui show warning",
								    g_strdup(_("Can't perform search!")), "main-gui");
						}
						lm_message_unref(message);
					}
					break;
				default:
					break;
				}

				tmplist = tmplist->next;
			}
		}
		GGaduDialog_free(dialog);
	}
	else if (signal->name == SEARCH_SIG)
	{
		GGaduDialog *dialog = signal->data;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			GSList *tmplist = ggadu_dialog_get_entries(dialog);
			LmMessage *message = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
			LmMessageNode *node = lm_message_node_add_child(message->node, "query", NULL);
			gchar *search_server = ggadu_config_var_get(jabber_handler,"search_server");

			lm_message_node_set_attributes(message->node, "to", search_server, "id", "search2", NULL);
			lm_message_node_set_attribute(node, "xmlns", "jabber:iq:search");
			
			while (tmplist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;

				switch (kv->key)
				{
				case GGADU_SEARCH_FIRSTNAME:
					if (kv->value)
						lm_message_node_add_child(node, "first", kv->value);
					break;
				case GGADU_SEARCH_LASTNAME:
					if (kv->value)
						lm_message_node_add_child(node, "last", kv->value);
					break;
				case GGADU_SEARCH_NICKNAME:
					if (kv->value)
						lm_message_node_add_child(node, "nick", kv->value);
					break;
				case GGADU_SEARCH_ID:
					if (kv->value)
					{
						gchar *nick = g_strndup(kv->value, strlen(kv->value)-strlen(g_strrstr(kv->value,"@")));
						lm_message_node_add_child(node, "nick", nick);
						g_free(nick);
					}
					break;
				default:
					break;
				}
				tmplist = tmplist->next;
			}

			action_queue_add("search2", "result", action_search_result, NULL, FALSE);

			if (!lm_connection_send(jabber_data.connection, message, NULL))
				signal_emit("jabber", "gui show warning", g_strdup(_("Can't perform search!")),
					    "main-gui");
			
			print_debug(lm_message_node_to_string(message->node));

			lm_message_unref(message);

		}
		GGaduDialog_free(dialog);
	}
	
	if (signal->name == USER_REMOVE_SIG) {
		GGaduDialog *d = signal->data;
		if (ggadu_dialog_get_response(d) == GGADU_OK) {
			user_remove_action(d->user_data);
		}
		g_slist_free(d->user_data);
		return;
	}																
}

static GSList *status_init()
{
	GSList *list = NULL;
	GGaduStatusPrototype *sp = g_new0(GGaduStatusPrototype, 10);

	if (!(sp = g_new0(GGaduStatusPrototype, 10)))
		return NULL;

	sp->status = JABBER_STATUS_AVAILABLE;
	sp->description = g_strdup(_("Available"));
	sp->image = g_strdup("jabber-online.png");
	list = g_slist_append(list, sp++);

	sp->status = JABBER_STATUS_CHAT;
	sp->description = g_strdup(_("Free for chat"));
	sp->image = g_strdup("jabber-online.png");
	list = g_slist_append(list, sp++);

	sp->status = JABBER_STATUS_AWAY;
	sp->description = g_strdup(_("Away"));
	sp->image = g_strdup("jabber-away.png");
	list = g_slist_append(list, sp++);

	sp->status = JABBER_STATUS_XA;
	sp->description = g_strdup(_("eXtended Away"));
	sp->image = g_strdup("jabber-xa.png");
	list = g_slist_append(list, sp++);

	sp->status = JABBER_STATUS_DND;
	sp->description = g_strdup(_("DND"));
	sp->image = g_strdup("jabber-dnd.png");
	list = g_slist_append(list, sp++);

	sp->status = JABBER_STATUS_AUTH_FROM;
	sp->description = g_strdup(_("is subscribed to you presence"));
	sp->image = g_strdup("jabber-auth-from.png");
	sp->receive_only = TRUE;
	list = g_slist_append(list, sp++);

	sp->status = JABBER_STATUS_NOAUTH;
	sp->description = g_strdup(_("No authorization"));
	sp->image = g_strdup("jabber-noauth.png");
	sp->receive_only = TRUE;
	list = g_slist_append(list, sp++);

	sp->status = JABBER_STATUS_UNAVAILABLE;
	sp->description = g_strdup(_("Unavailable"));
	sp->image = g_strdup("jabber-offline.png");
	list = g_slist_append(list, sp++);

	sp->status = JABBER_STATUS_DESCR;
	sp->description = g_strdup(_("Set description ..."));
	sp->image = g_strdup("tlen-desc.png");
	list = g_slist_append(list, sp++);

	sp->status = JABBER_STATUS_ERROR;
	sp->description = g_strdup(_("Error"));
	sp->image = g_strdup("jabber-error.png");
	sp->receive_only = TRUE;
	list = g_slist_append(list, sp++);

	return list;
}

gpointer user_search_action(gpointer user_data)
{
	GGaduDialog *dialog = NULL;
	gchar *server;

	if (!lm_connection_is_authenticated(jabber_data.connection))
	{
		signal_emit("jabber", "gui show warning", g_strdup(_("You have to be connected to perform searching!")),
			    "main-gui");
		return NULL;
	}

	/* try 'search_server' from config */
	if (!(server = ggadu_config_var_get(jabber_handler, "search_server")))
	{
		/* if not, try 'server' from config */
		if (!(server = ggadu_config_var_get(jabber_handler, "server")))
		{
			/* if not, search for server in 'jid' */
			server = ggadu_config_var_get(jabber_handler, "jid");
			if (server && (server = g_strstr_len(server, strlen(server), "@")))
				server++;
		}
	}

	if (!server || !*server)
		server = NULL;

	dialog = ggadu_dialog_new(GGADU_DIALOG_GENERIC, _("Jabber search: server"), "search-server");

	ggadu_dialog_add_entry(dialog, 0, _("Server:"), VAR_STR, server, VAR_FLAG_NONE);
	signal_emit("jabber", "gui show dialog", dialog, "main-gui");

	return NULL;
}

gpointer user_preferences_action(gpointer user_data)
{
	GGaduDialog *dialog;

	dialog = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("Jabber plugin configuration"), "update config");
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_JID, _("Jabber ID"), VAR_STR,
				ggadu_config_var_get(jabber_handler, "jid"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_PASSWORD, _("Password"), VAR_STR,
				ggadu_config_var_get(jabber_handler, "password"), VAR_FLAG_PASSWORD);
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_LOG, _("Log chats to history file"), VAR_BOOL,
				ggadu_config_var_get(jabber_handler, "log"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_AUTOCONNECT, _("Autoconnect on startup"), VAR_BOOL,
				ggadu_config_var_get(jabber_handler, "autoconnect"), VAR_FLAG_NONE);

	if (lm_ssl_is_supported())
	{
		ggadu_dialog_add_entry(dialog, GGADU_JABBER_USESSL, _("Use SSL"), VAR_BOOL,
					ggadu_config_var_get(jabber_handler, "use_ssl"), VAR_FLAG_NONE);
	}

	ggadu_dialog_add_entry(dialog, GGADU_JABBER_RESOURCE, _("Resource"), VAR_STR,
				ggadu_config_var_get(jabber_handler, "resource"), VAR_FLAG_NONE);

	ggadu_dialog_add_entry(dialog, GGADU_JABBER_SERVER, _("Server\n(optional)"), VAR_STR,
				ggadu_config_var_get(jabber_handler, "server"), VAR_FLAG_NONE);
	
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_PROXY,
			       _("Proxy server (optional)\n[user:pass@]host.com[:8080]"), VAR_STR,
			       ggadu_config_var_get(jabber_handler, "proxy"), VAR_FLAG_NONE);

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
	return NULL;
}

GGaduMenu *build_jabber_menu()
{
	GGaduMenu *root;
	GGaduMenu *item;

	root = ggadu_menu_create();
	item = ggadu_menu_add_item(root, "_Jabber", NULL, NULL);

	ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Add Contact"), user_add_action, NULL));
	ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Preferences"), user_preferences_action, NULL));
	ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Search for friends"), user_search_action, NULL));
	ggadu_menu_add_submenu(item, ggadu_menu_new_item("", NULL, NULL));
	ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Register account"), jabber_register_account_dialog, NULL));
	return root;
}

void start_plugin()
{
	p = g_new0(GGaduProtocol, 1);
	p->display_name = g_strdup("Jabber");
	p->img_filename = g_strdup("jabber.png");
	p->statuslist = status_init();
	p->offline_status = g_slist_append(p->offline_status, (gint *) JABBER_STATUS_UNAVAILABLE);
	p->online_status = g_slist_append(p->online_status, (gint *) JABBER_STATUS_AVAILABLE);
	p->away_status = g_slist_append(p->away_status, (gint *) JABBER_STATUS_AWAY);
	p->away_status = g_slist_append(p->away_status, (gint *) JABBER_STATUS_DND);
	p->away_status = g_slist_append(p->away_status, (gint *) JABBER_STATUS_XA);

	ggadu_repo_add_value("_protocols_", p->display_name, p, REPO_VALUE_PROTOCOL);

	signal_emit(GGadu_PLUGIN_NAME, "gui register protocol", p, "main-gui");

	CHANGE_STATUS_SIG = register_signal(jabber_handler, "change status");
	CHANGE_STATUS_DESCR_SIG = register_signal(jabber_handler, "change status descr");
	GET_CURRENT_STATUS_SIG = register_signal(jabber_handler, "get current status");
	UPDATE_CONFIG_SIG = register_signal(jabber_handler, "update config");
	SEND_MESSAGE_SIG = register_signal(jabber_handler, "send message");
	JABBER_SUBSCRIBE_SIG = register_signal(jabber_handler, "jabber subscribe");
	GET_USER_MENU_SIG = register_signal(jabber_handler, "get user menu");
	SEARCH_SERVER_SIG = register_signal(jabber_handler, "search-server");
	SEARCH_SIG = register_signal(jabber_handler, "search");
	ADD_USER_SIG = register_signal(jabber_handler, "add user");
	REGISTER_ACCOUNT = register_signal(jabber_handler, "register account");
	USER_REMOVE_SIG = register_signal(jabber_handler, "user remove action");

	jabbermenu = build_jabber_menu();

	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", jabbermenu, "main-gui");

	if (ggadu_config_var_get(jabber_handler, "autoconnect"))
	{
		print_debug("jabber: autoconneting");
		jabber_change_status(JABBER_STATUS_AVAILABLE);
	}

}

GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
	gchar *path = NULL;
	GGadu_PLUGIN_ACTIVATE(conf_ptr);

	print_debug("%s: initialize", GGadu_PLUGIN_NAME);

	jabber_handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("Jabber protocol"));

	register_signal_receiver(jabber_handler, (signal_func_ptr) jabber_signal_recv);
	ggadu_repo_add("jabber");

	path = g_build_filename(config->configdir, "jabber", NULL);
	ggadu_config_set_filename(jabber_handler, path);
	g_free(path);

	ggadu_config_var_add_with_default(jabber_handler, "search_server", VAR_STR, "users.jabber.org");
	ggadu_config_var_add(jabber_handler, "jid", VAR_STR);
	ggadu_config_var_add(jabber_handler, "password", VAR_STR);
	ggadu_config_var_add(jabber_handler, "server", VAR_STR);
	ggadu_config_var_add(jabber_handler, "log", VAR_BOOL);
	ggadu_config_var_add(jabber_handler, "autoconnect", VAR_BOOL);
	ggadu_config_var_add(jabber_handler, "resource", VAR_STR);
	ggadu_config_var_add(jabber_handler, "proxy", VAR_STR);

	if (lm_ssl_is_supported())
		ggadu_config_var_add(jabber_handler, "use_ssl", VAR_BOOL);

	if (!ggadu_config_read(jabber_handler))
		g_warning(_("Unable to read configuration file for plugin jabber"));


	jabber_data.status = JABBER_STATUS_UNAVAILABLE;

	return jabber_handler;
}

void destroy_plugin()
{
	print_debug("destroy_plugin %s", GGadu_PLUGIN_NAME);

	if (jabbermenu)
	{
		signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", jabbermenu, "main-gui");
		ggadu_menu_free(jabbermenu);
	}

	ggadu_repo_del_value("_protocols", p->display_name);

	signal_emit(GGadu_PLUGIN_NAME, "gui unregister protocol", p, "main-gui");
}
