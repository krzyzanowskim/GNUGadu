/* $Id: jabber_plugin.c,v 1.155 2005/01/27 16:42:57 mkobierzycki Exp $ */

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
GGaduProtocol *p;

LmMessageHandler *iq_handler;
LmMessageHandler *iq_roster_handler;
LmMessageHandler *iq_version_handler;
LmMessageHandler *iq_vcard_handler;
LmMessageHandler *iq_account_data_handler;
LmMessageHandler *presence_handler;
LmMessageHandler *message_handler;

static GQuark SEARCH_SERVER_SIG;
static GQuark JABBER_SUBSCRIBE_SIG;
static GQuark CHANGE_STATUS_SIG;
static GQuark CHANGE_STATUS_DESCR_DIALOG_SIG;
static GQuark SEND_MESSAGE_SIG;
static GQuark ADD_USER_SIG;
static GQuark GET_USER_SIG;
static GQuark UPDATE_CONFIG_SIG;
static GQuark SEARCH_SIG;
static GQuark GET_CURRENT_STATUS_SIG;
static GQuark GET_USER_MENU_SIG;
static GQuark REGISTER_ACCOUNT;
static GQuark REMOVE_ACCOUNT;
static GQuark REGISTER_GET_FIELDS;
static GQuark USER_REMOVE_SIG;
static GQuark USER_EDIT_VCARD_SIG;
static GQuark USER_SHOW_VCARD_SIG;
static GQuark USER_CHANGE_PASSWORD_SIG;
static GQuark USER_GET_SOFTWARE_SIG;

jabber_data_type jabber_data;

GGaduMenu *jabbermenu;
GGaduMenu *jabber_services_discovery_menu;

static GGaduMenu *build_jabber_menu();


GGadu_PLUGIN_INIT("jabber", GGADU_PLUGIN_TYPE_PROTOCOL);

static gpointer user_chat_action(gpointer user_data)
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
		msg->time = time(NULL);

		/* check if ignored */
		{
			GGaduContact *ki = g_new0(GGaduContact, 1);

			ki->id = g_strdup(msg->id);
			if (signal_emit(GGadu_PLUGIN_NAME, "ignore check contact", ki, "ignore*"))
			{
				GGaduMsg_free(msg);
				GGaduContact_free(ki);
				return NULL;
			}

			GGaduContact_free(ki);
		}

		signal_emit("jabber", "gui msg receive", msg, "main-gui");
	}

	return NULL;
}

static gpointer user_add_action(gpointer user_data)
{
	GGaduDialog *dialog;

	if (!jabber_data.connection || !lm_connection_is_open(jabber_data.connection))
	{
		signal_emit("jabber", "gui show warning", g_strdup(_("Not connected to server")), "main-gui");
		return NULL;
	}

	dialog = ggadu_dialog_new(GGADU_DIALOG_GENERIC, _("Add contact"), "add user");
	ggadu_dialog_add_entry(dialog, GGADU_ID, _("Jabber ID:"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_NICK, _("Nickname:"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_REQUEST_AUTH_FROM, _("Request authorization from"), VAR_BOOL, (gpointer) TRUE, VAR_FLAG_ADVANCED);

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
	return NULL;
}

static gpointer user_edit_action(gpointer user_data)
{
	GSList *user = (GSList *) user_data;
	GGaduDialog *dialog = NULL;
	GGaduContact *k = NULL;

	if (!user)
		return NULL;

	dialog = ggadu_dialog_new(GGADU_DIALOG_GENERIC, _("Edit contact"), "add user");
	k = (GGaduContact *) user->data;
	ggadu_dialog_add_entry(dialog, GGADU_ID, _("Jabber ID:"), VAR_STR, k->id, VAR_FLAG_INSENSITIVE);
	ggadu_dialog_add_entry(dialog, GGADU_NICK, _("Nickname:"), VAR_STR, k->nick, VAR_FLAG_NONE);

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
	return NULL;
}

static gpointer user_own_vcard_action(gpointer user_data)
{
	LmMessage *msg;
	LmMessageNode *node;

	if (!jabber_data.connection || !lm_connection_is_open(jabber_data.connection))
	{
		signal_emit("jabber", "gui show warning", g_strdup(_("Not connected to server")), "main-gui");
		return NULL;
	}

	msg = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);
	lm_message_node_set_attribute(msg->node, "from", ggadu_config_var_get(jabber_handler, "jid"));
	lm_message_node_set_attribute(msg->node, "id", "v1");

	node = lm_message_node_add_child(msg->node, "vCard", NULL);
	lm_message_node_set_attribute(node, "xmlns", "vcard-temp");
	lm_connection_send(jabber_data.connection, msg, NULL);
	lm_message_unref(msg);

	return NULL;
}

static gpointer user_vcard_action(gpointer user_data)
{
	GSList *user = (GSList *) user_data;
	GGaduContact *k = (GGaduContact *) user->data;
	LmMessage *msg;
	LmMessageNode *node;

	if (!user)
		return NULL;

	msg = lm_message_new_with_sub_type(k->id, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);
	lm_message_node_set_attribute(msg->node, "id", "v3");
	node = lm_message_node_add_child(msg->node, "vCard", NULL);
	lm_message_node_set_attribute(node, "xmlns", "vcard-temp");
	lm_connection_send(jabber_data.connection, msg, NULL);

	lm_message_unref(msg);

	return NULL;
}


static gpointer user_get_software_action(gpointer user_data)
{
	LmMessage *m;
	LmMessageNode *node;
	GSList *user = (GSList *) user_data;
	GGaduContact *k = (GGaduContact *) user->data;
	GGaduContact *k0;
	GSList *roster = ggadu_repo_get_as_slist("jabber", REPO_VALUE_CONTACT);
	gchar *string0, *string1;

	while (roster)
	{
		k0 = roster->data;
		if (!strcmp(k->id, k0->id))
			break;

		roster = roster->next;
	}

	string0 = g_strconcat(k0->id, "/", k0->resource, NULL);
	m = lm_message_new_with_sub_type(string0, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);
	string1 = g_strconcat(ggadu_config_var_get(jabber_handler, "jid"), "/",
			      ggadu_config_var_get(jabber_handler, "resource") ? ggadu_config_var_get(jabber_handler, "resource") : JABBER_DEFAULT_RESOURCE, NULL);
	lm_message_node_set_attribute(m->node, "from", string1);
	lm_message_node_set_attribute(m->node, "id", "version_1");
	node = lm_message_node_add_child(m->node, "query", NULL);
	lm_message_node_set_attribute(node, "xmlns", "jabber:iq:version");
	print_debug(lm_message_node_to_string(m->node));
	lm_connection_send(jabber_data.connection, m, NULL);
	lm_message_unref(m);

	g_free(string0);
	g_free(string1);
	g_slist_free(roster);

	return NULL;
}

static gpointer user_change_password_action(gpointer user_data)
{
	GGaduDialog *dialog;

	if (!jabber_data.connection || !lm_connection_is_open(jabber_data.connection))
	{
		signal_emit("jabber", "gui show warning", g_strdup(_("Not connected to server")), "main-gui");
		return NULL;
	}

	dialog = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("Change password"), "user change password");
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_PASSWORD, _("_Old password:"), VAR_STR, NULL, VAR_FLAG_PASSWORD);
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_PASSWORD_NEW, _("_New password:"), VAR_STR, NULL, VAR_FLAG_PASSWORD);
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_PASSWORD_RETYPE, _("_Retype password:"), VAR_STR, NULL, VAR_FLAG_PASSWORD);

	signal_emit("jabber", "gui show dialog", dialog, "main-gui");

	return NULL;
}

static gpointer user_remove_account_action(gpointer user_data)
{
	GGaduDialog *dialog;

	if (!jabber_data.connection || !lm_connection_is_open(jabber_data.connection))
	{
		signal_emit("jabber", "gui show warning", g_strdup(_("Not connected to server")), "main-gui");
		return NULL;
	}

	dialog = ggadu_dialog_new(GGADU_DIALOG_YES_NO, _("Account removal confirmation"), "remove account");
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_SERVER, g_strdup_printf("Do you really want to remove account: %s ?",
									    (gchar *) ggadu_config_var_get(jabber_handler, "jid")), VAR_NULL, NULL, VAR_FLAG_NONE);
	signal_emit("jabber", "gui show dialog", dialog, "main-gui");

	return NULL;
}

static gpointer user_ask_remove_action(gpointer user_data)
{
	GGaduDialog *dialog;

	dialog = ggadu_dialog_new_full(GGADU_DIALOG_YES_NO, _("Are You sure You want to delete selected user(s)?"), "user remove action", user_data);

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
	return NULL;
}

static gpointer user_remove_action(gpointer user_data)
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
				lm_message_node_set_attributes(node, "jid", g_strdup(k->id), "subscription", "remove", NULL);

				if (lm_connection_send(jabber_data.connection, m, NULL))
				{
					print_debug("send remove request");
					action_queue_add("roster_remove", "result", action_roster_remove_result, k->id, TRUE);
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

static gpointer user_resend_auth_to(gpointer user_data)
{
	GSList *user = (GSList *) user_data;
	GGaduContact *k = (GGaduContact *) user->data;
	LmMessage *m = lm_message_new_with_sub_type(k->id, LM_MESSAGE_TYPE_PRESENCE, LM_MESSAGE_SUB_TYPE_SUBSCRIBED);
	lm_connection_send(jabber_data.connection, m, NULL);
	lm_message_unref(m);
	return NULL;
}

static gpointer user_rerequest_auth_from(gpointer user_data)
{
	GSList *user = (GSList *) user_data;
	GGaduContact *k = NULL;
	LmMessage *m = NULL;

	if (!user)
		return NULL;

	k = (GGaduContact *) user->data;
	m = lm_message_new_with_sub_type(k->id, LM_MESSAGE_TYPE_PRESENCE, LM_MESSAGE_SUB_TYPE_SUBSCRIBE);
	lm_connection_send(jabber_data.connection, m, NULL);
	lm_message_unref(m);
	return NULL;
}

static gpointer user_remove_auth_from(gpointer user_data)
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
	GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("Jabber server for account registration"),
					       "register get fields");

	ggadu_dialog_add_entry(dialog, GGADU_JABBER_SERVER, _("_Server:"), VAR_STR, NULL, VAR_FLAG_NONE);
	signal_emit("jabber", "gui show dialog", dialog, "main-gui");

	return NULL;
}

static GGaduMenu *build_userlist_menu(void)
{
	GGaduMenu *menu = ggadu_menu_create();
	GGaduMenu *listmenu, *infomenu;

	ggadu_menu_add_submenu(menu, ggadu_menu_new_item(_("_Chat"), user_chat_action, NULL));
	ggadu_menu_add_user_menu_extensions(menu, jabber_handler);

	infomenu = ggadu_menu_new_item(_("Contact info"), NULL, NULL);
	ggadu_menu_add_submenu(infomenu, ggadu_menu_new_item(_("_Personal data"), user_vcard_action, NULL));
	ggadu_menu_add_submenu(infomenu, ggadu_menu_new_item(_("_Software"), user_get_software_action, NULL));
	ggadu_menu_add_submenu(menu, infomenu);

	listmenu = ggadu_menu_new_item(_("Authorization"), NULL, NULL);
	ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("_Request presence subscription"), user_rerequest_auth_from, NULL));
	ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("S_end presence authorization"), user_resend_auth_to, NULL));
	ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item("", NULL, NULL));
	ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Re_move authorization"), user_remove_auth_from, NULL));
	ggadu_menu_add_submenu(menu, listmenu);

	ggadu_menu_add_submenu(menu, ggadu_menu_new_item("", NULL, NULL));
	ggadu_menu_add_submenu(menu, ggadu_menu_new_item(_("E_dit"), user_edit_action, NULL));
	ggadu_menu_add_submenu(menu, ggadu_menu_new_item(_("Rem_ove"), user_ask_remove_action, NULL));
	ggadu_menu_add_submenu(menu, ggadu_menu_new_item("", NULL, NULL));
	ggadu_menu_add_submenu(menu, ggadu_menu_new_item(_("Add _New"), user_add_action, NULL));


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
			gboolean reconn = FALSE;
			gchar *field;
			while (tmplist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
				switch (kv->key)
				{
				case GGADU_JABBER_JID:
					{
						field = ggadu_config_var_get(jabber_handler, "jid");
						if (kv->value && field && ggadu_strcmp(kv->value, field))
							reconn = TRUE;
						if (!(kv->value && field) && (kv->value || field))
							reconn = TRUE;

						ggadu_config_var_set(jabber_handler, "jid", kv->value);
					}
					break;
				case GGADU_JABBER_PASSWORD:
					ggadu_config_var_set(jabber_handler, "password", kv->value);
					break;
				case GGADU_JABBER_LOG:
					ggadu_config_var_set(jabber_handler, "log", kv->value);
					break;
				case GGADU_JABBER_ONLY_FRIENDS:
					ggadu_config_var_set(jabber_handler, "only_friends", kv->value);
					break;
				case GGADU_JABBER_AUTOCONNECT:
					ggadu_config_var_set(jabber_handler, "autoconnect", kv->value);
					break;
				case GGADU_JABBER_USESSL:
					ggadu_config_var_set(jabber_handler, "use_ssl", kv->value);
					break;
				case GGADU_JABBER_RESOURCE:
					{
						field = ggadu_config_var_get(jabber_handler, "resource");
						if (kv->value && field && ggadu_strcmp(kv->value, field))
							reconn = TRUE;
						if (!(kv->value && field) && (kv->value || field))
							reconn = TRUE;

						ggadu_config_var_set(jabber_handler, "resource", kv->value);
					}
					break;
				case GGADU_JABBER_SERVER:
					{
						field = ggadu_config_var_get(jabber_handler, "server");
						if (kv->value && field && ggadu_strcmp(kv->value, field))
							reconn = TRUE;
						if (!(kv->value && field) && (kv->value || field))
							reconn = TRUE;

						ggadu_config_var_set(jabber_handler, "server", kv->value);
					}
					break;
				case GGADU_JABBER_PROXY:
					{
						field = ggadu_config_var_get(jabber_handler, "proxy");
						if (kv->value && field && ggadu_strcmp(kv->value, field))
							reconn = TRUE;
						if (!(kv->value && field) && (kv->value || field))
							reconn = TRUE;

						ggadu_config_var_set(jabber_handler, "proxy", kv->value);
					}
					break;
				}
				tmplist = tmplist->next;
			}
			ggadu_config_save(jabber_handler);
			if (reconn && jabber_data.status != JABBER_STATUS_UNAVAILABLE)
			{
				lm_connection_close(jabber_data.connection, NULL);
				g_thread_create(jabber_login_connect, (gpointer) JABBER_STATUS_AVAILABLE, FALSE, NULL);
			}
		}
		GGaduDialog_free(dialog);
	}
	else if (signal->name == REGISTER_GET_FIELDS)
	{
		GGaduDialog *dialog = signal->data;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			GSList *list = ggadu_dialog_get_entries(dialog);
			GGaduKeyValue *kv = (GGaduKeyValue *) list->data;
			LmConnection *conn;

			conn = lm_connection_new(kv->value);
			if (!lm_connection_open(conn, (LmResultFunction) jabber_register_account_cb, NULL, NULL, NULL))
				signal_emit("jabber", "gui show warning", g_strdup(_("Couldn't open connection for account registration.")), "main-gui");
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
			LmMessage *msg;
			LmMessageNode *node;
			LmMessageHandler *handler;
			gboolean error = FALSE;

			msg = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
			lm_message_node_set_attribute(msg->node, "id", "reg2");
			node = lm_message_node_add_child(msg->node, "query", NULL);
			lm_message_node_set_attribute(node, "xmlns", "jabber:iq:register");

			while (tmplist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
				switch (kv->key)
				{
				case GGADU_JABBER_USERNAME:
					{
						if (!kv->value)
						{
							error = TRUE;
							break;
						}
						else
						{
							lm_message_node_add_child(node, "username", kv->value);
							gjr->username = g_strdup(kv->value);
						}
					}
					break;
				case GGADU_JABBER_PASSWORD:
					{
						if (!kv->value)
						{
							error = TRUE;
							break;
						}
						else
						{
							lm_message_node_add_child(node, "password", kv->value);
							gjr->password = g_strdup(kv->value);
						}
					}
					break;
				case GGADU_JABBER_FN:
					{
						if (!kv->value)
						{
							error = TRUE;
							break;
						}
						else
						{
							lm_message_node_add_child(node, "name", kv->value);
						}
					}
					break;
				case GGADU_JABBER_USERID:
					{
						if (!kv->value)
						{
							error = TRUE;
							break;
						}
						else
						{
							lm_message_node_add_child(node, "email", kv->value);
						}
					}
					break;
				case GGADU_JABBER_UPDATE_CONFIG:
					{
						gjr->update_config = (gboolean) kv->value;
						gjr->server = g_strdup(lm_connection_get_server((LmConnection *) dialog->user_data));
					}
					break;
				}
				tmplist = tmplist->next;
			}

			if (error)
			{
				signal_emit("jabber", "gui show warning", g_strdup(_("Invalid data passed, try again")), "main-gui");
			}
			else
			{
				handler = lm_message_handler_new((LmHandleMessageFunction) register_register_handler, (gpointer) gjr, NULL);
				lm_connection_send_with_reply((LmConnection *) dialog->user_data, msg, handler, NULL);
			}

			lm_message_unref(msg);
		}
		GGaduDialog_free(dialog);
	}
	else if (signal->name == REMOVE_ACCOUNT)
	{
		GGaduDialog *dialog = (GGaduDialog *) signal->data;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			LmMessage *msg;
			LmMessageNode *node;

			msg = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
			lm_message_node_set_attribute(msg->node, "to", lm_connection_get_server(jabber_data.connection));
			lm_message_node_set_attribute(msg->node, "id", "unreg1");
			node = lm_message_node_add_child(msg->node, "query", NULL);
			lm_message_node_set_attribute(node, "xmlns", "jabber:iq:register");
			lm_message_node_add_child(node, "remove", NULL);

			lm_connection_send(jabber_data.connection, msg, NULL);
			lm_message_unref(msg);
		}

		GGaduDialog_free(dialog);
	}
	else if (signal->name == CHANGE_STATUS_DESCR_DIALOG_SIG)
	{
		GGaduDialog *dialog = signal->data;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK && dialog->user_data)
		{
			GGaduStatusPrototype *sp = dialog->user_data;
			GGaduKeyValue *kv = NULL;

			if (ggadu_dialog_get_entries(dialog))
			{
				GGaduStatusPrototype *sp2;
				kv = (GGaduKeyValue *) ggadu_dialog_get_entries(dialog)->data;

				g_free(sp->status_description);
				sp->status_description = NULL;
				
				sp2 = ggadu_find_status_prototype(p,jabber_data.status);

				g_free(sp2->status_description);
				sp2->status_description = NULL;
				
				if (kv && kv->value && (strlen(kv->value) > 0))
				{
					sp2->status_description = g_strdup(kv->value); /* not sure */
				}

				jabber_change_status(sp2, FALSE);
				GGaduStatusPrototype_free(sp2);
			}
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

			GGaduDialog *dialog = ggadu_dialog_new_full(GGADU_DIALOG_GENERIC, _("Enter status description"),
								    "change status descr dialog", sp);

			ggadu_dialog_add_entry(dialog, 0, _("Description:"), VAR_STR, jabber_data.description, VAR_FLAG_FOCUS);
			signal_emit("jabber", "gui show dialog", dialog, "main-gui");
			return;
		} else
		{
		    jabber_change_status(sp, TRUE);
		}

	}
	else if (signal->name == GET_CURRENT_STATUS_SIG)
	{
		GGaduStatusPrototype *sp;
		
		if (jabber_data.connection)
			sp = ggadu_find_status_prototype(p, jabber_data.status);
		else
			sp = ggadu_find_status_prototype(p, JABBER_STATUS_UNAVAILABLE);
			
		if (jabber_data.description)
		{
		    g_free(sp->status_description);
		    sp->status_description = g_strdup(jabber_data.description);
		}
		else
		    sp->status_description = NULL;
		
		// to free
		signal->data_return = sp;
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

			ggadu_jabber_save_history(GGADU_HISTORY_TYPE_SEND, msg, msg->id);

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
					k->id = g_strdup(kv->value);
					break;
				case GGADU_NICK:
					if (kv->value && (((gchar *) kv->value)[0] != '\0'))
						k->nick = g_strdup(kv->value);
					break;
				case GGADU_JABBER_REQUEST_AUTH_FROM:
					if ((gboolean) kv->value == TRUE)
						send_request = TRUE;
					/*user_rerequest_auth_from(k); */
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

			if (send_request)
			{
				GSList *listtmp = g_slist_append(NULL, k);
				user_rerequest_auth_from(listtmp);
				user_resend_auth_to(listtmp);
				g_slist_free(listtmp);
			}

			lm_message_unref(m);
			GGaduContact_free(k);
		}

		GGaduDialog_free(dialog);
	}
	else if (signal->name == GET_USER_SIG)
	{
		GGaduContact *k = NULL;
		gchar *id = (gchar *) signal->data;
		if (id && (k = ggadu_repo_find_value("jabber", ggadu_repo_key_from_string(id))))
		{
			/* return copy of GGaduContact */
			signal->data_return = GGaduContact_copy(k);
		}

	}
	else if (signal->name == USER_EDIT_VCARD_SIG)
	{
		GGaduDialog *dialog = signal->data;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			GSList *entries = ggadu_dialog_get_entries(dialog);
			LmMessage *msg = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
			LmMessageNode *node;
			gchar *string;
			gchar *given = NULL;
			gchar *family = NULL;
			gint bday;
			gint bmonth;
			gint byear;

			lm_message_node_set_attribute(msg->node, "id", "v1");
			node = lm_message_node_add_child(msg->node, "vCard", NULL);
			lm_message_node_set_attribute(node, "xmlns", "vcard-temp");

			while (entries)
			{
				GGaduKeyValue *kv = entries->data;

				switch (kv->key)
				{
				case GGADU_JABBER_GIVEN:
					given = (gchar *) kv->value;
					break;
				case GGADU_JABBER_FAMILY:
					family = (gchar *) kv->value;
					break;
				case GGADU_JABBER_FN:
					{
						string = g_strconcat(given ? given : "", (given && family) ? " " : "", family, NULL);
						lm_message_node_add_child(node, "FN", string);
						node = lm_message_node_add_child(node, "N", NULL);
						lm_message_node_add_child(node, "FAMILY", family);
						lm_message_node_add_child(node, "GIVEN", given);
						lm_message_node_add_child(node, "MIDDLE", NULL);

						g_free(string);
					}
					break;
				case GGADU_JABBER_NICKNAME:
					{
						node = lm_message_node_find_child(msg->node, "vCard");
						lm_message_node_add_child(node, "NICKNAME", kv->value);
					}
					break;
				case GGADU_JABBER_URL:
					lm_message_node_add_child(node, "URL", kv->value);
					break;
				case GGADU_JABBER_BDAY:
					bday = (gint) kv->value;
					break;
				case GGADU_JABBER_BMONTH:
					bmonth = (gint) kv->value;
					break;
				case GGADU_JABBER_BYEAR:
					{
						byear = (gint) kv->value;
						string = (byear && bmonth && bday) ? g_strdup_printf("%d-%02d-%02d", byear, bmonth, bday) : NULL;
						lm_message_node_add_child(node, "BDAY", string);

						g_free(string);
					}
					break;
				case GGADU_JABBER_ORGNAME:
					{
						node = lm_message_node_add_child(node, "ORG", NULL);
						lm_message_node_add_child(node, "ORGNAME", kv->value);
						lm_message_node_add_child(node, "ORGUNIT", NULL);
						node = lm_message_node_find_child(msg->node, "vCard");
						lm_message_node_add_child(node, "TITLE", NULL);
						lm_message_node_add_child(node, "ROLE", NULL);
					}
					break;
				case GGADU_JABBER_NUMBER:
					{
						node = lm_message_node_add_child(node, "TEL", NULL);
						lm_message_node_add_child(node, "NUMBER", kv->value);
						node = lm_message_node_find_child(msg->node, "vCard");
					}
					break;
				case GGADU_JABBER_LOCALITY:
					{
						node = lm_message_node_add_child(node, "ADR", NULL);
						lm_message_node_add_child(node, "HOME", NULL);
						lm_message_node_add_child(node, "EXTADD", NULL);
						lm_message_node_add_child(node, "STREET", NULL);
						lm_message_node_add_child(node, "LOCALITY", kv->value);
						lm_message_node_add_child(node, "REGION", NULL);
						lm_message_node_add_child(node, "PCODE", NULL);

					}
					break;
				case GGADU_JABBER_CTRY:
					{
						lm_message_node_add_child(node, "CTRY", kv->value);
						node = lm_message_node_find_child(msg->node, "vCard");
					}
					break;
				case GGADU_JABBER_USERID:
					{
						node = lm_message_node_add_child(node, "EMAIL", NULL);
						lm_message_node_add_child(node, "USERID", kv->value);
					}
					break;
				}
				entries = entries->next;
			}

			lm_connection_send(jabber_data.connection, msg, NULL);
			lm_message_unref(msg);
		}

		GGaduDialog_free(dialog);
	}
	else if (signal->name == USER_SHOW_VCARD_SIG)
	{
		GGaduDialog_free((GGaduDialog *) signal->data);
		/* We have to free this dialog somewhere */
	}
	else if (signal->name == USER_CHANGE_PASSWORD_SIG)
	{
		GGaduDialog *dialog = (GGaduDialog *) signal->data;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			GSList *list = ggadu_dialog_get_entries(dialog);
			gchar *pass0;

			while (list)
			{
				GGaduKeyValue *kv = list->data;

				switch (kv->key)
				{
				case GGADU_JABBER_PASSWORD:
					{
						if (strcmp(ggadu_config_var_get(jabber_handler, "password"), kv->value))
						{
							signal_emit("jabber", "gui show warning", g_strdup(_("Incorrect old password")), "main-gui");
							GGaduDialog_free(dialog);
							return;
						}
					}
					break;
				case GGADU_JABBER_PASSWORD_NEW:
					{
						if (kv->value)
						{
							pass0 = kv->value;
						}
						else
						{
							signal_emit("jabber", "gui show warning", g_strdup(_("New password not specified")), "main-gui");
							GGaduDialog_free(dialog);
							return;
						}
					}
					break;
				case GGADU_JABBER_PASSWORD_RETYPE:
					{
						if (kv->value)
						{
							if (!strcmp(pass0, kv->value))
							{
								LmMessage *msg;
								LmMessageNode *node;
								gchar *username = g_strdup(ggadu_config_var_get(jabber_handler,
														"jid"));

								msg = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
								lm_message_node_set_attributes(msg->node,
											       "to", lm_connection_get_server(jabber_data.connection), "id", "change1", NULL);
								node = lm_message_node_add_child(msg->node, "query", NULL);
								lm_message_node_set_attribute(node, "xmlns", "jabber:iq:register");
								strchr(username, '@')[0] = 0;
								lm_message_node_add_child(node, "username", username);
								lm_message_node_add_child(node, "password", kv->value);

								ggadu_config_var_set(jabber_handler, "password", kv->value);
								lm_connection_send(jabber_data.connection, msg, NULL);
								lm_message_unref(msg);

								g_free(username);
							}
							else
							{
								signal_emit("jabber", "gui show warning", g_strdup(_("Passwords mismatch")), "main-gui");
								GGaduDialog_free(dialog);
								return;
							}
						}
						else
						{
							signal_emit("jabber", "gui show warning", g_strdup(_("New password not specified twice")), "main-gui");
							GGaduDialog_free(dialog);
							return;
						}
					}
					break;
				}

				list = list->next;
			}
		}

		GGaduDialog_free(dialog);
	}
	else if (signal->name == USER_GET_SOFTWARE_SIG)
	{
		GGaduDialog_free((GGaduDialog *) signal->data);
	}
	else if (signal->name == JABBER_SUBSCRIBE_SIG)
	{

		GGaduDialog *dialog = signal->data;
		gchar *jid = dialog->user_data;
		LmMessage *msg = NULL;

		if (jid && ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			msg = lm_message_new_with_sub_type(jid, LM_MESSAGE_TYPE_PRESENCE, LM_MESSAGE_SUB_TYPE_SUBSCRIBED);
		}
		else if (jid && ggadu_dialog_get_response(dialog) == GGADU_CANCEL)
		{
			msg = lm_message_new_with_sub_type(jid, LM_MESSAGE_TYPE_PRESENCE, LM_MESSAGE_SUB_TYPE_UNSUBSCRIBED);
		}

		lm_connection_send(jabber_data.connection, msg, NULL);
		lm_message_unref(msg);

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

						message = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);

						lm_message_node_set_attributes(message->node, "to", kv->value, "id", "search1", NULL);
						node = lm_message_node_add_child(message->node, "query", NULL);
						lm_message_node_set_attribute(node, "xmlns", "jabber:iq:search");

						ggadu_config_var_set(jabber_handler, "search_server", kv->value);
						ggadu_config_save(jabber_handler);

						action_queue_add("search1", "result", action_search_form, NULL, FALSE);

						if (!lm_connection_send(jabber_data.connection, message, NULL))
						{
							signal_emit("jabber", "gui show warning", g_strdup(_("Can't perform search!")), "main-gui");
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
			gchar *search_server = ggadu_config_var_get(jabber_handler, "search_server");

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
					{
						gchar *nick = g_strrstr(kv->value, "@")
							? g_strndup(kv->value, strlen(kv->value) - strlen(g_strrstr(kv->value, "@"))) : g_strdup(kv->value);
						lm_message_node_add_child(node, "nick", nick);
						g_free(nick);
					}
					break;
				case GGADU_SEARCH_EMAIL:
					if (kv->value)
						lm_message_node_add_child(node, "email", kv->value);
					break;

				default:
					break;
				}
				tmplist = tmplist->next;
			}

			action_queue_add("search2", "result", action_search_result, NULL, FALSE);

			if (!lm_connection_send(jabber_data.connection, message, NULL))
				signal_emit("jabber", "gui show warning", g_strdup(_("Can't perform search!")), "main-gui");

			print_debug(lm_message_node_to_string(message->node));

			lm_message_unref(message);

		}
		GGaduDialog_free(dialog);
	}

	if (signal->name == USER_REMOVE_SIG)
	{
		GGaduDialog *d = signal->data;
		if (ggadu_dialog_get_response(d) == GGADU_OK)
		{
			user_remove_action(d->user_data);
		}
		g_slist_free(d->user_data);
		return;
	}
}

static GSList *status_init()
{
	GSList *list = NULL;
	GGaduStatusPrototype *sp = NULL;

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
		signal_emit("jabber", "gui show warning", g_strdup(_("You have to be connected to perform searching!")), "main-gui");
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

	dialog = ggadu_dialog_new(GGADU_DIALOG_GENERIC, _("Jabber search server"), "search-server");

	ggadu_dialog_add_entry(dialog, 0, _("_Server:"), VAR_STR, server, VAR_FLAG_NONE);
	signal_emit("jabber", "gui show dialog", dialog, "main-gui");

	return NULL;
}

gpointer user_preferences_action(gpointer user_data)
{
	GGaduDialog *dialog;

	dialog = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("Jabber plugin configuration"), "update config");
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_JID, _("_Jabber ID:"), VAR_STR, ggadu_config_var_get(jabber_handler, "jid"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_PASSWORD, _("_Password:"), VAR_STR, ggadu_config_var_get(jabber_handler, "password"), VAR_FLAG_PASSWORD);
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_ONLY_FRIENDS, _("_Receive messages from friends only"), VAR_BOOL,
			       ggadu_config_var_get(jabber_handler, "only_friends"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_JABBER_AUTOCONNECT, _("_Autoconnect on startup"), VAR_BOOL, ggadu_config_var_get(jabber_handler, "autoconnect"), VAR_FLAG_NONE);

	if (lm_ssl_is_supported())
	{
		ggadu_dialog_add_entry(dialog, GGADU_JABBER_USESSL, _("Use _encrypted connection (SSL)"), VAR_BOOL, ggadu_config_var_get(jabber_handler, "use_ssl"), VAR_FLAG_NONE);
	}

	ggadu_dialog_add_entry(dialog, GGADU_JABBER_LOG, _("_Log chats to history file"), VAR_BOOL, ggadu_config_var_get(jabber_handler, "log"), VAR_FLAG_ADVANCED);

	ggadu_dialog_add_entry(dialog, GGADU_JABBER_RESOURCE, _("Re_source:"), VAR_STR, ggadu_config_var_get(jabber_handler, "resource"), VAR_FLAG_ADVANCED);

	ggadu_dialog_add_entry(dialog, GGADU_JABBER_SERVER, _("Jabber server a_ddress:"), VAR_STR, ggadu_config_var_get(jabber_handler, "server"), VAR_FLAG_ADVANCED);

	ggadu_dialog_add_entry(dialog, GGADU_JABBER_PROXY,
			       _("Pro_xy server\n[user:pass@]host.com[:port]"), VAR_STR, ggadu_config_var_get(jabber_handler, "proxy"), VAR_FLAG_ADVANCED);

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
	return NULL;
}

static LmHandlerResult jabber_services_discovery_handler(LmMessageHandler * handler, LmConnection * connection, LmMessage * message, gpointer user_data)
{
	LmMessageNode *node;
	LmMessageNode *nodes_service;
	LmMessageNode *n_service;
	/* add checking */
	node = lm_message_get_node(message);
	/* before JEP-0030 style */
	nodes_service = lm_message_node_get_child(node, "service");
	if (nodes_service) 
	{
	    signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", jabbermenu, "main-gui");
	    jabbermenu = build_jabber_menu();
		n_service = lm_message_node_get_child(nodes_service, "service");
		while (n_service)
		{
/*
			ggadu_dialog_add_entry(dialog, GGADU_JABBER_JID,
					       (gpointer) lm_message_node_get_attribute(n_service, "name"),
					       VAR_STR, (gpointer) lm_message_node_get_attribute(n_service, "jid"), VAR_FLAG_INSENSITIVE);
*/    			

			ggadu_menu_add_submenu(jabber_services_discovery_menu,
					       ggadu_menu_new_item( (gchar *)lm_message_node_get_attribute(n_service, "name"),
					    			    jabber_services_discovery_action,
								    NULL)
					       );
			
			print_debug(lm_message_node_to_string(n_service));
			n_service = n_service->next;
		}
	    signal_emit(GGadu_PLUGIN_NAME, "gui register menu", jabbermenu, "main-gui");
	    return LM_HANDLER_RESULT_REMOVE_MESSAGE;

	}
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

gpointer jabber_services_discovery_action(gpointer user_data)
{
	LmMessage *msg;
	LmMessageNode *node;
	LmMessageHandler *message_handler;

	if (!jabber_data.connection || !lm_connection_is_open(jabber_data.connection))
	{
		signal_emit("jabber", "gui show warning", g_strdup(_("Not connected to server")), "main-gui");
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}

	msg = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);
	lm_message_node_set_attribute(msg->node, "to", lm_connection_get_server(jabber_data.connection));

	node = lm_message_node_add_child(msg->node, "query", NULL);
	lm_message_node_set_attribute(node, "xmlns", "jabber:iq:browse");

	message_handler = lm_message_handler_new(jabber_services_discovery_handler, NULL, NULL);

	lm_connection_send_with_reply(jabber_data.connection, msg, message_handler, NULL);
	lm_message_unref(msg);
	lm_message_handler_unref(message_handler);
	return NULL;
}

static GGaduMenu *build_jabber_menu()
{
	GGaduMenu *root;
	GGaduMenu *item;
	GGaduMenu *account;

	root = ggadu_menu_create();
	item = ggadu_menu_add_item(root, "_Jabber", NULL, NULL);

	ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("_Add Contact"), user_add_action, NULL));
	ggadu_menu_add_submenu(item, ggadu_menu_new_item("", NULL, NULL));
	ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("_Search for friends"), user_search_action, NULL));
	ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("_Preferences"), user_preferences_action, NULL));
	ggadu_menu_add_submenu(item, ggadu_menu_new_item("", NULL, NULL));
	ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Personal _data"), user_own_vcard_action, NULL));

	jabber_services_discovery_menu = ggadu_menu_new_item(_("Service d_iscovery"), NULL, NULL);
	ggadu_menu_add_submenu(item, jabber_services_discovery_menu);

	ggadu_menu_add_submenu(item, ggadu_menu_new_item("", NULL, NULL));
	account = ggadu_menu_new_item(_("Account"), NULL, NULL);
	ggadu_menu_add_submenu(account, ggadu_menu_new_item(_("R_egister account"), jabber_register_account_dialog, NULL));
	ggadu_menu_add_submenu(account, ggadu_menu_new_item(_("Remo_ve account"), user_remove_account_action, NULL));
	ggadu_menu_add_submenu(account, ggadu_menu_new_item(_("C_hange password"), user_change_password_action, NULL));
	ggadu_menu_add_submenu(item, account);

	return root;
}

void start_plugin()
{
	p = g_new0(GGaduProtocol, 1);
	p->display_name = g_strdup("Jabber");
	p->protocol_uri = g_strdup("xmpp:");
	p->img_filename = g_strdup("jabber.png");
	p->statuslist = status_init();
	p->offline_status = g_slist_append(p->offline_status, (gint *) JABBER_STATUS_UNAVAILABLE);
	p->online_status = g_slist_append(p->online_status, (gint *) JABBER_STATUS_AVAILABLE);
	p->away_status = g_slist_append(p->away_status, (gint *) JABBER_STATUS_AWAY);
	p->away_status = g_slist_append(p->away_status, (gint *) JABBER_STATUS_DND);
	p->away_status = g_slist_append(p->away_status, (gint *) JABBER_STATUS_XA);
	jabber_handler->plugin_data = p;

	ggadu_repo_add_value("_protocols_", GGadu_PLUGIN_NAME, p, REPO_VALUE_PROTOCOL);

	signal_emit(GGadu_PLUGIN_NAME, "gui register protocol", p, "main-gui");

	CHANGE_STATUS_SIG = register_signal(jabber_handler, "change status");
	CHANGE_STATUS_DESCR_DIALOG_SIG = register_signal(jabber_handler, "change status descr dialog");
	GET_CURRENT_STATUS_SIG = register_signal(jabber_handler, "get current status");
	UPDATE_CONFIG_SIG = register_signal(jabber_handler, "update config");
	SEND_MESSAGE_SIG = register_signal(jabber_handler, "send message");
	JABBER_SUBSCRIBE_SIG = register_signal(jabber_handler, "jabber subscribe");
	GET_USER_MENU_SIG = register_signal(jabber_handler, "get user menu");
	SEARCH_SERVER_SIG = register_signal(jabber_handler, "search-server");
	SEARCH_SIG = register_signal(jabber_handler, "search");
	ADD_USER_SIG = register_signal(jabber_handler, "add user");
	GET_USER_SIG = register_signal(jabber_handler, "get user");
	REGISTER_ACCOUNT = register_signal(jabber_handler, "register account");
	REMOVE_ACCOUNT = register_signal(jabber_handler, "remove account");
	REGISTER_GET_FIELDS = register_signal(jabber_handler, "register get fields");
	USER_REMOVE_SIG = register_signal(jabber_handler, "user remove action");
	USER_EDIT_VCARD_SIG = register_signal(jabber_handler, "user edit vcard");
	USER_SHOW_VCARD_SIG = register_signal(jabber_handler, "user show vcard");
	USER_CHANGE_PASSWORD_SIG = register_signal(jabber_handler, "user change password");
	USER_GET_SOFTWARE_SIG = register_signal(jabber_handler, "user get software");

	jabbermenu = build_jabber_menu();

	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", jabbermenu, "main-gui");

	if (ggadu_config_var_get(jabber_handler, "autoconnect"))
	{
		print_debug("jabber: autoconneting");
		GGaduStatusPrototype *sp = ggadu_find_status_prototype(p,JABBER_STATUS_AVAILABLE);
		jabber_change_status(sp, FALSE);
		GGaduStatusPrototype_free(sp);
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
	ggadu_config_var_add_with_default(jabber_handler, "log", VAR_BOOL, (gpointer) TRUE);
	ggadu_config_var_add(jabber_handler, "only_friends", VAR_BOOL);
	ggadu_config_var_add(jabber_handler, "autoconnect", VAR_BOOL);
	ggadu_config_var_add_with_default(jabber_handler, "resource", VAR_STR, JABBER_DEFAULT_RESOURCE);
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

	ggadu_repo_del_value("_protocols_", p);

	signal_emit(GGadu_PLUGIN_NAME, "gui unregister protocol", p, "main-gui");
}
