/* $Id: sms_gui.c,v 1.67 2005/09/24 03:43:21 shaster Exp $ */

/*
 * SMS plugin for GNU Gadu 2
 *
 * Copyright (C) 2003 Bartlomiej Pawlak
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
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "ggadu_types.h"
#include "plugins.h"
#include "signals.h"
#include "ggadu_conf.h"
#include "ggadu_menu.h"
#include "ggadu_support.h"
#include "ggadu_dialog.h"
#include "ggadu_repo.h"
#include "sms_gui.h"
#include "sms_core.h"

GGaduPlugin *sms_handler;
GSList *smslist = NULL;
gchar *orange_token_path = NULL;
gint method;

GGaduProtocol *p;
static GGaduMenu *menu_smsmenu;
static GGaduPluginExtension *ext = NULL;	/* extension to handle send sms from foreign plugins */

GGadu_PLUGIN_INIT("sms", GGADU_PLUGIN_TYPE_PROTOCOL);

/* okienko z ustawieniami */
gpointer sms_preferences(gpointer user_data)
{
	GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("SMS Preferences"), "update config");

	print_debug("%s : Preferences\n", GGadu_PLUGIN_NAME);

	/* *INDENT-OFF* */
	ggadu_dialog_add_entry(dialog, GGADU_SMS_CONFIG_EXTERNAL, _("Use \"SMS\" program to send"), VAR_BOOL, (gpointer) ggadu_config_var_get(sms_handler, "external"), VAR_FLAG_ADVANCED);
	ggadu_dialog_add_entry(dialog, GGADU_SMS_CONFIG_SENDER, _("Sender"), VAR_STR, (gpointer) ggadu_config_var_get(sms_handler, "sender"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_SMS_CONFIG_ERA_LOGIN, _("Era login"), VAR_STR, (gpointer) ggadu_config_var_get(sms_handler, "era_login"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_SMS_CONFIG_ERA_PASSWORD, _("Era password"), VAR_STR, (gpointer) ggadu_config_var_get(sms_handler, "era_password"), VAR_FLAG_PASSWORD);
	ggadu_dialog_add_entry(dialog, GGADU_SMS_CONFIG_SHOW_IN_STATUS, _("Show plugin in status list (needs reload)"), VAR_BOOL, (gpointer) ggadu_config_var_get(sms_handler, "show_in_status"), VAR_FLAG_ADVANCED);
	/* *INDENT-ON* */

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");

	return NULL;
}

/* edycja usera */
gpointer sms_edit_contact(gpointer user_data)
{
	GSList *users = (GSList *) user_data;
	GGaduContact *k = (GGaduContact *) users->data;
	gchar *id = g_strconcat(k->nick, "@", k->mobile, NULL);
	GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_GENERIC, _("Edit contact"), "change user");

	print_debug("%s : Edit Contact\n", GGadu_PLUGIN_NAME);

	/* *INDENT-OFF* */
	ggadu_dialog_add_entry(dialog, GGADU_SMS_CONTACT_ID, _("Id"), VAR_STR, id, VAR_FLAG_INSENSITIVE);
	ggadu_dialog_add_entry(dialog, GGADU_SMS_CONTACT_NICK, _("Nick"), VAR_STR, k->nick, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_SMS_CONTACT_NUMBER, _("Number"), VAR_STR, k->mobile, VAR_FLAG_NONE);
	/* *INDENT-ON* */

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");

	g_free(id);
	return NULL;
}

/* okienko do dodawania usera */
gpointer sms_add_contact(gpointer user_data)
{
	GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_GENERIC, _("Add contact"), "add user");

	print_debug("%s : Add Contact\n", GGadu_PLUGIN_NAME);

	/* *INDENT-OFF* */
	ggadu_dialog_add_entry(dialog, GGADU_SMS_CONTACT_NICK, _("Nick"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_SMS_CONTACT_NUMBER, _("Number"), VAR_STR, NULL, VAR_FLAG_NONE);
	/* *INDENT-ON* */

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");

	return NULL;
}

/* wywalenie usera */
gpointer sms_remove_contact(gpointer user_data)
{
	GSList *users = (GSList *) user_data;

	while (users)
	{
		GGaduContact *k = (GGaduContact *) users->data;
		smslist = g_slist_remove(smslist, k);
		ggadu_repo_del_value("sms", k->id);

		users = users->next;
	}

	signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", NULL, "main-gui");
	save_smslist();

	return NULL;
}

/* wyslanie smsa po kliknieciu prawego przycisku */
gpointer sms_send_sms(gpointer user_data)
{
	GSList *users = (GSList *) user_data;
	GGaduContact *k = (users) ? (GGaduContact *) users->data : NULL;
	GGaduDialog *dialog = NULL;
	gchar *tmp;

	if ((!k) || (!k->mobile) || (strlen(k->mobile) <= 0))
	{
		signal_emit("sms", "gui show message", g_strdup(_("No phone number")), "main-gui");
		return NULL;
	}

	tmp = g_strconcat(_("Send to : "), k->nick, " (", k->mobile, ")", NULL);
	dialog = ggadu_dialog_new(GGADU_DIALOG_GENERIC, tmp, "sms send");
	g_free(tmp);

	ggadu_config_var_set(sms_handler, "number", k->mobile);

	/* *INDENT-OFF* */
	ggadu_dialog_add_entry(dialog, GGADU_SMS_CONFIG_EXTERNAL, _("Use _external program"), VAR_BOOL, (gpointer) ggadu_config_var_get(sms_handler, "external"), VAR_FLAG_ADVANCED);
	ggadu_dialog_add_entry(dialog, GGADU_SMS_CONFIG_SENDER, _("_Sender"), VAR_STR, (gpointer) ggadu_config_var_get(sms_handler, "sender"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_SMS_CONFIG_BODY, _("_Message"), VAR_STR, (gpointer) ggadu_config_var_get(sms_handler, "body"), VAR_FLAG_FOCUS);
	/* *INDENT-ON* */

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");

	return NULL;
}

/* tworzenie menu w "MENU" */
GGaduMenu *sms_menu()
{
	GGaduMenu *root = ggadu_menu_create();
	GGaduMenu *item = ggadu_menu_add_item(root, "SMS", NULL, NULL);

	ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Add Contact"), sms_add_contact, NULL));
	ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Preferences"), sms_preferences, NULL));

	return root;
}

/* wiadomo */
GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
	gchar *path = NULL;
	GGadu_PLUGIN_ACTIVATE(conf_ptr);

	sms_handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("SMS sender"));
	print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);

	print_debug("%s : read configuration\n", GGadu_PLUGIN_NAME);

	path = g_build_filename(config->configdir, "sms", NULL);
	ggadu_config_set_filename((GGaduPlugin *) sms_handler, path);
	g_free(path);

	ggadu_config_var_add(sms_handler, "sender", VAR_STR);
	ggadu_config_var_add(sms_handler, "number", VAR_STR);
	ggadu_config_var_add(sms_handler, "body", VAR_STR);
	ggadu_config_var_add(sms_handler, "era_login", VAR_STR);
	ggadu_config_var_add(sms_handler, "era_password", VAR_STR);
	ggadu_config_var_add(sms_handler, "external", VAR_BOOL);
	ggadu_config_var_add(sms_handler, "show_in_status", VAR_BOOL);

	if (!ggadu_config_read(sms_handler))
		g_warning(_("Unable to read config file for plugin sms"));

	orange_token_path = g_build_filename(config->configdir, ORANGE_GFX, NULL);

	register_signal_receiver((GGaduPlugin *) sms_handler, (signal_func_ptr) signal_receive);

	ggadu_repo_add("sms");

	return sms_handler;
}

/* odczyt sygnalow i podjecie odpowiednich czynnosci zwiazanych z tym */
void signal_receive(gpointer name, gpointer signal_ptr)
{
	GGaduSignal *signal = (GGaduSignal *) signal_ptr;

	print_debug("%s : received signal %d\n", GGadu_PLUGIN_NAME, signal->name);

	if (signal->name == g_quark_from_static_string("get user menu"))
	{
		GGaduMenu *umenu = ggadu_menu_create();

		ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("Send SMS"), sms_send_sms, NULL));
		ggadu_menu_add_submenu(umenu, ggadu_menu_new_item("", NULL, NULL));
		ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("Edit"), sms_edit_contact, NULL));
		ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("Remove"), sms_remove_contact, NULL));
		ggadu_menu_add_submenu(umenu, ggadu_menu_new_item("", NULL, NULL));
		ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("Add New"), sms_add_contact, NULL));

		ggadu_menu_print(umenu, NULL);

		signal->data_return = umenu;
	}

	if (signal->name == g_quark_from_static_string("update config"))
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
				case GGADU_SMS_CONFIG_SENDER:
					print_debug("change default sender to %s\n", kv->value);
					ggadu_config_var_set(sms_handler, "sender", kv->value);
					break;
				case GGADU_SMS_CONFIG_ERA_LOGIN:
					print_debug("change default era_login to %s\n", kv->value);
					ggadu_config_var_set(sms_handler, "era_login", kv->value);
					break;
				case GGADU_SMS_CONFIG_ERA_PASSWORD:
					print_debug("change default era_password to %s\n", kv->value);
					ggadu_config_var_set(sms_handler, "era_password", kv->value);
					break;
				case GGADU_SMS_CONFIG_EXTERNAL:
					print_debug("change external program to %d\n", kv->value);
					ggadu_config_var_set(sms_handler, "external", kv->value);
					break;
				case GGADU_SMS_CONFIG_SHOW_IN_STATUS:
					print_debug("change show_in_status to %d\n", kv->value);
					ggadu_config_var_set(sms_handler, "show_in_status", kv->value);
					break;
				}
				tmplist = tmplist->next;
			}
			ggadu_config_save(sms_handler);
		}

		GGaduDialog_free(dialog);
		return;
	}

	if (signal->name == g_quark_from_static_string("change status"))
	{
		GGaduStatusPrototype *sp = signal->data;
		

		if (!sp)
			return;

		if (sp->status == 1 || sp->status == 2)
		{
			GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_GENERIC, _("SMS Send"), "sms send");

			if (sp->status == 1)
				ggadu_config_var_set(sms_handler, "external", (gpointer) FALSE);
			else
				ggadu_config_var_set(sms_handler, "external", (gpointer) TRUE);

			print_debug("%s : Send sms\n", GGadu_PLUGIN_NAME);

			/* *INDENT-OFF* */
			ggadu_dialog_add_entry(dialog, GGADU_SMS_CONFIG_SENDER, _("Sender"), VAR_STR, (gpointer) ggadu_config_var_get(sms_handler, "sender"), VAR_FLAG_NONE);
			ggadu_dialog_add_entry(dialog, GGADU_SMS_CONFIG_NUMBER, _("Number"), VAR_STR, (gpointer) ggadu_config_var_get(sms_handler, "number"), VAR_FLAG_NONE);
			ggadu_dialog_add_entry(dialog, GGADU_SMS_CONFIG_BODY, _("Message"), VAR_STR, (gpointer) ggadu_config_var_get(sms_handler, "body"), VAR_FLAG_FOCUS);
			/* *INDENT-ON* */

			signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
		}
	}

	if (signal->name == g_quark_from_static_string("send message"))
	{
		GGaduMsg *msg = signal->data;

		if (msg)
		{
			method = GGADU_SMS_METHOD_CHAT;
			if (ggadu_config_var_get(sms_handler, "sender") != NULL)
			{
				SMS *message = (SMS *) g_new0(SMS, 1);
				message->external = (gboolean) ggadu_config_var_get(sms_handler, "external");
				message->sender = g_strdup(ggadu_config_var_get(sms_handler, "sender"));	/* due to sender_temp somewhere below */
				message->number = g_strdup(msg->id);
				message->body = from_utf8("ISO-8859-2", msg->message);
				message->era_login = ggadu_config_var_get(sms_handler, "era_login");
				message->era_password = ggadu_config_var_get(sms_handler, "era_password");

				g_thread_create((gpointer(*)())send_sms, (gpointer) message, FALSE, NULL);
			}
			else
				sms_preferences(0);
		}
	}

	if (signal->name == g_quark_from_static_string("sms send"))
	{
		GGaduDialog *dialog = signal->data;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			gboolean old_external = (gboolean) ggadu_config_var_get(sms_handler, "external");
			GSList *tmplist = ggadu_dialog_get_entries(dialog);
			SMS *message = (SMS *) g_new0(SMS, 1);
			gchar *sender_temp = NULL;
			
			while (tmplist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
				switch (kv->key)
				{
				case GGADU_SMS_CONFIG_SENDER:
					sender_temp = g_strdup(kv->value);
					break;
				case GGADU_SMS_CONFIG_EXTERNAL:
					ggadu_config_var_set(sms_handler, "external", kv->value);
					break;
				case GGADU_SMS_CONFIG_NUMBER:
					ggadu_config_var_set(sms_handler, "number", kv->value);
					break;
				case GGADU_SMS_CONFIG_BODY:
					ggadu_config_var_set(sms_handler, "body", kv->value);
					break;
				}
				tmplist = tmplist->next;
			}
			method = GGADU_SMS_METHOD_POPUP;

			message->external = (gboolean) ggadu_config_var_get(sms_handler, "external");
			message->sender = sender_temp;
			message->number = g_strdup(ggadu_config_var_get(sms_handler, "number"));
			message->body = g_strdup(ggadu_config_var_get(sms_handler, "body"));
			message->era_login = ggadu_config_var_get(sms_handler, "era_login");
			message->era_password = ggadu_config_var_get(sms_handler, "era_password");

			g_thread_create((gpointer(*)())send_sms, message, FALSE, NULL);

			ggadu_config_var_set(sms_handler, "external", (gpointer) old_external);
			ggadu_config_save(sms_handler);
		}

		GGaduDialog_free(dialog);
		return;
	}

	if (signal->name == g_quark_from_static_string("add user"))
	{
		GGaduDialog *d = signal->data;
		
		if (ggadu_dialog_get_response(d) == GGADU_OK) {
			GGaduContact *k = g_new0(GGaduContact, 1);
			GSList *entry = NULL;

			entry = ggadu_dialog_get_entries(d);
			while (entry)	{
				GGaduKeyValue *kv = (GGaduKeyValue *) entry->data;
				switch (kv->key) {
					case GGADU_SMS_CONTACT_NICK:
						k->nick = g_strdup(kv->value);
						break;
					case GGADU_SMS_CONTACT_NUMBER:
						k->mobile = g_strdup(kv->value);
						k->id = k->mobile;
						k->status = 1;
						break;
				}
				entry = entry->next;
			}

			smslist = g_slist_append(smslist, k);
			ggadu_repo_add_value("sms", k->id, k, REPO_VALUE_CONTACT);
			signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", NULL, "main-gui");

			save_smslist();

		}
		GGaduDialog_free(d);
		return;
	}

	if (signal->name == g_quark_from_static_string("change user"))
	{
		GGaduDialog *d = signal->data;

		if (ggadu_dialog_get_response(d) == GGADU_OK) {
			GGaduContact *k = g_new0(GGaduContact, 1);
			GSList *uslist = smslist;
			GSList *entry = NULL;

			entry = ggadu_dialog_get_entries(d);
			while (entry) {
				GGaduKeyValue *kv = (GGaduKeyValue *) entry->data;
				switch (kv->key){
					case GGADU_SMS_CONTACT_ID:
						k->id = g_strdup(kv->value);
						break;
					case GGADU_SMS_CONTACT_NICK:
						k->nick = g_strdup(kv->value);
						break;
					case GGADU_SMS_CONTACT_NUMBER:
						k->mobile = g_strdup(kv->value);
						k->status = 1;
						break;
				}
				entry = entry->next;
			}

			while (uslist) {
				GGaduContact *kvtmp = (GGaduContact *) uslist->data;
				gchar *id = g_strconcat(kvtmp->nick, "@", kvtmp->mobile, NULL);

				if (!ggadu_strcasecmp(id, k->id))
				{
					g_free(kvtmp->nick);
					g_free(kvtmp->mobile);
					kvtmp->mobile = g_strdup(k->mobile);
					kvtmp->nick = g_strdup(k->nick);
					ggadu_repo_change_value("sms", kvtmp->id, kvtmp, REPO_VALUE_CONTACT);
					g_free(id);
					break;
				}
				g_free(id);
				uslist = uslist->next;
			}
			save_smslist();
			signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", NULL, "main-gui");

			GGaduContact_free(k);
		}
		
		GGaduDialog_free(d);
		return;
	}
	
	if (signal->name == g_quark_from_static_string("get token"))
	{
		GGaduDialog *dialog = signal->data;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			GSList *tmplist = ggadu_dialog_get_entries(dialog);
			SMS *message = (SMS *) dialog->user_data;
			
			while (tmplist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
				if (kv->key == 1)
					message->orange_pass = g_strdup(kv->value);
					
				tmplist = tmplist->next;
			}
			g_thread_create((gpointer(*)())send_ORANGE_stage2, message, FALSE, NULL);
		}
		
		GGaduDialog_free(dialog);
	}
}

/* guziczki na dole, podpiete pod zmiane statusu */
GSList *button_send()
{
	GSList *list = NULL;
	GGaduStatusPrototype *sp = NULL;
	gboolean receive_only_value = ggadu_config_var_get(sms_handler,"show_in_status") ? FALSE : TRUE;

	sp = g_new0(GGaduStatusPrototype, 2);

	sp->status = 1;
	sp->description = g_strdup(_("Send SMS (internal)"));
	sp->image = g_strdup("sms_i.png");
	sp->receive_only = receive_only_value;
	list = g_slist_append(list, sp);
	sp++;

	sp->status = 2;
	sp->description = g_strdup(_("Send SMS (external)"));
	sp->image = g_strdup("sms_i.png");
	sp->receive_only = receive_only_value;
	list = g_slist_append(list, sp);

	return list;
}

/* gdzies to musi sie zaczac */
void start_plugin()
{
	print_debug("%s : start_plugin\n", GGadu_PLUGIN_NAME);

	p = g_new0(GGaduProtocol, 1);
	p->display_name = g_strdup("SMS");
	p->protocol_uri = g_strdup("sms://");
	p->img_filename = g_strdup("sms.png");
	p->statuslist = button_send();
	sms_handler->plugin_data = p;

	register_signal(sms_handler, "update config");
	register_signal(sms_handler, "change status");
	register_signal(sms_handler, "sms send");
	register_signal(sms_handler, "add user");
	register_signal(sms_handler, "change user");
	register_signal(sms_handler, "send message");
	register_signal(sms_handler, "get token");

	ggadu_repo_add_value("_protocols_", GGadu_PLUGIN_NAME, p, REPO_VALUE_PROTOCOL);

	signal_emit(GGadu_PLUGIN_NAME, "gui register protocol", p, "main-gui");

	print_debug("%s : create menu\n", GGadu_PLUGIN_NAME);

	menu_smsmenu = sms_menu();
	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_smsmenu, "main-gui");

	load_smslist();

	signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", NULL, "main-gui");

	ext = g_new0(GGaduPluginExtension, 1);
	ext->type = GGADU_PLUGIN_EXTENSION_USER_MENU_TYPE;
	ext->callback = sms_send_sms;
	ext->txt = _("Send SMS");

	register_extension_for_plugin(ext,GGADU_PLUGIN_TYPE_PROTOCOL);
}

void destroy_plugin()
{
	print_debug("destroy_plugin%s\n", GGadu_PLUGIN_NAME);

	unregister_extension_for_plugins(ext);

	ggadu_repo_del("sms");
	ggadu_repo_del_value("_protocols_", p);

	signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", menu_smsmenu, "main-gui");
	signal_emit(GGadu_PLUGIN_NAME, "gui unregister protocol", p, "main-gui");
	ggadu_menu_free(menu_smsmenu);

	g_free(orange_token_path);
}

/* zapis listy numerow do pliku */
void save_smslist()
{
	GSList *smu = smslist;
	gchar *path = NULL;
	gchar *path_tmp = NULL;
	GIOChannel *ch_tmp = NULL;
	gsize bytes_written;

	/* open temporary file */
	path_tmp = g_build_filename(config->configdir, GGADU_SMS_USERLIST_TMPFILE, NULL);
	ch_tmp = g_io_channel_new_file(path_tmp, "w", NULL);

	if (!ch_tmp)
	{
		print_debug("cannot create smslist! %s\n", path_tmp);
		signal_emit("sms", "gui show warning", g_strdup(_("Writing userlist failed!")), "main-gui");
		g_free(path_tmp);
		return;
	}

	/* set encoding to utf-8 */
	g_io_channel_set_encoding(ch_tmp, NULL, NULL);

	/* write smslist */
	while (smu)
	{
		GGaduContact *k = (GGaduContact *) smu->data;
		gchar *t, *line;

		/* Podmiana srednikow na przecinki */
		for (t = k->nick; *t; t++)
			if (*t == ';')
				*t = ',';

		line = g_strdup_printf("%s;%s\n", k->nick, k->mobile);
		g_io_channel_write_chars(ch_tmp, line, -1, &bytes_written, NULL);
		g_free(line);

		smu = smu->next;
	}

	if (g_io_channel_shutdown(ch_tmp, TRUE, NULL) != G_IO_STATUS_NORMAL)
	{
		print_debug("error writing temporary smslist file\n");
		signal_emit("sms", "gui show warning", g_strdup(_("Writing userlist failed!")), "main-gui");
		g_free(path_tmp);
		return;
	}
	g_io_channel_unref(ch_tmp);

	/* mv temporary_file destination_file */
	path = g_build_filename(config->configdir, GGADU_SMS_USERLIST_FILENAME, NULL);
	if (rename(path_tmp, path) != 0)
	{
		print_debug("error renaming %s to %s\n", path_tmp, path);
		signal_emit("sms", "gui show warning", g_strdup(_("Writing userlist failed!")), "main-gui");
	}

	g_free(path);
	g_free(path_tmp);

	return;
}

/* wczytanie listy numerow z pliku */
void load_smslist()
{
	FILE *fp;
	GGaduContact *k = NULL;
	gchar *path;
	gchar *nick = NULL, *mobile = NULL;

	path = g_build_filename(config->configdir, GGADU_SMS_USERLIST_FILENAME, NULL);

	fp = fopen(path, "r");

	g_free(path);
	if (!fp)
	{
		print_debug("smslist not found\n");
		return;
	}

	nick = g_malloc0(GGADU_SMS_MAXLEN_NICK);
	mobile = g_malloc0(GGADU_SMS_MAXLEN_NUMBER);

	while (fscanf(fp, "%[^;];%[^\n]\n", nick, mobile) != EOF)
	{
		gchar *number = mobile;

		/* Cut-off prefixes. 'just-in-case' */
		if (g_str_has_prefix(number, "+48"))
			number += 3;

		if (g_str_has_prefix(number, "0"))
			number++;

		k = g_new0(GGaduContact, 1);
		k->nick = g_strdup(nick);
		k->mobile = g_strdup(number);
		k->id = k->mobile;
		k->status = 1;
		print_debug("%s %s\n", k->nick, k->mobile);
		smslist = g_slist_append(smslist, k);
		ggadu_repo_add_value("sms", k->id, k, REPO_VALUE_CONTACT);
	}

	if (fclose(fp) != 0)
		print_debug("fclose() failed while reading smslist!\n");

	g_free(nick);
	g_free(mobile);
}
