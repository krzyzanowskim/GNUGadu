/* $Id: ignore.c,v 1.10 2004/12/27 09:43:43 krzyzak Exp $ */

/* 
 * Ignore plugin code for GNU Gadu 2 
 * 
 * Copyright (C) 2001-2005 GNU Gadu Team 
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gg2_core.h"
#include "ignore.h"

static GGaduPlugin *ignore_handler;
static GGaduPluginExtension *ext_in;
static GGaduMenu *menu_ignoremenu;

GGadu_PLUGIN_INIT("ignore-main", GGADU_PLUGIN_TYPE_MISC);

static gchar *ggadu_add_ignored_contact(gchar * ignored_list, GGaduContact * k)
{
	gchar *ret = NULL;

	if (!k || !k->id)
		return NULL;

	if (!ignored_list)
		ignored_list = "";

	if (!g_strrstr(ignored_list, k->id))
		ret = g_strconcat(ignored_list, k->id, ":", NULL);

	return ret;
}

static gchar *ggadu_remove_ignored_contact(gchar * ignored_list, GGaduContact * k)
{
	gchar *ret = NULL;
	gchar *searchk;

	if (!k || !ignored_list || !k->id)
		return NULL;

	searchk = g_strconcat(k->id, ":", NULL);
	if (g_strrstr(ignored_list, searchk))
	{
		gchar **tab;

		tab = g_strsplit(ignored_list, searchk, 2);
		ret = g_strconcat(tab[0], tab[1], NULL);

		g_strfreev(tab);
	}

	g_free(searchk);
	return ret;
}


/* SIGNALS HANDLER */
static void my_signal_receive(gpointer name, gpointer signal_ptr)
{
	GGaduSignal *signal = (GGaduSignal *) signal_ptr;

	if (signal->name == IGNORE_ADD_CONTACT_SIG)
	{
	}

	if (signal->name == IGNORE_REMOVE_CONTACT_SIG)
	{
	}

	if (signal->name == IGNORE_DIALOG_ADD_ID_SIG)
	{
		GGaduDialog *dialog = (GGaduDialog *) signal->data;
		GSList *entry = NULL;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			entry = ggadu_dialog_get_entries(dialog);
			while (entry)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) entry->data;

				switch ((gint) kv->key)
				{
				case GGADU_IGNORE_ADD_ID:
					{
						GGaduContact *k = g_new0(GGaduContact, 1);
						k->id = g_strdup(kv->value);
						gchar *ignored_list_prev = ggadu_config_var_get(ignore_handler, "list");
						gchar *ignored_list_after = NULL;

						ignored_list_after = ggadu_add_ignored_contact(ignored_list_prev, k);
						if (ignored_list_after)
						{
							ggadu_config_var_set(ignore_handler, "list", ignored_list_after);
							g_free(ignored_list_prev);
						}

						GGaduContact_free(k);
					}
					break;
				}
				entry = entry->next;
			}
			ggadu_config_save(ignore_handler);
		}
		GGaduDialog_free(dialog);
	}


	if (signal->name == IGNORE_DIALOG_REMOVE_ID_SIG)
	{
		GGaduDialog *dialog = (GGaduDialog *) signal->data;
		GSList *entry = NULL;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			entry = ggadu_dialog_get_entries(dialog);
			while (entry)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) entry->data;

				switch ((gint) kv->key)
				{
				case GGADU_IGNORE_REMOVE_ID:
					{
						GGaduContact *k = g_new0(GGaduContact, 1);
						k->id = g_strdup(kv->value);
						gchar *ignored_list_prev = ggadu_config_var_get(ignore_handler, "list");
						gchar *ignored_list_after = NULL;

						ignored_list_after = ggadu_remove_ignored_contact(ignored_list_prev, k);
						if (ignored_list_after)
						{
							ggadu_config_var_set(ignore_handler, "list", ignored_list_after);
							g_free(ignored_list_prev);
						}

						GGaduContact_free(k);
					}
					break;
				}
				entry = entry->next;
			}
			ggadu_config_save(ignore_handler);
		}
		GGaduDialog_free(dialog);
	}

	if (signal->name == IGNORE_CHECK_CONTACT_SIG)
	{
		GGaduContact *k = (GGaduContact *) signal->data;
		gchar *list = ggadu_config_var_get(ignore_handler, "list");

		signal->data_return = FALSE;

		if (list && g_strrstr(list, k->id))
			signal->data_return = (gpointer) TRUE;
	}
}


static gpointer ignore_un_ignore_action(gpointer selected_contacts)
{
	GSList *users = NULL;
	gchar *ignored_list_prev = NULL;
	gchar *ignored_list_after = NULL;

	users = (GSList *) selected_contacts;
	while (users)
	{
		GGaduContact *k = (GGaduContact *) users->data;

		ignored_list_prev = ggadu_config_var_get(ignore_handler, "list");
		ignored_list_after = NULL;

		print_debug("(Un)Ignore action %s", k->id);

		if (!g_strrstr(ignored_list_prev ? ignored_list_prev : "", k->id))
		{
			ignored_list_after = ggadu_add_ignored_contact(ignored_list_prev, k);
		}
		else
		{
			ignored_list_after = ggadu_remove_ignored_contact(ignored_list_prev, k);
		}

		if (ignored_list_after)
		{
			ggadu_config_var_set(ignore_handler, "list", ignored_list_after);
			g_free(ignored_list_prev);
		}

		users = users->next;
	}

	ggadu_config_save(ignore_handler);
	return NULL;
}

static gpointer ignore_show_list_action(gpointer user_data)
{
	gchar *ignored_list = ggadu_config_var_get(ignore_handler, "list");
	gchar *p = NULL;

	if (!ignored_list)
	{
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("List is empty")), "main-gui");
		return NULL;
	}

	while ((p = strchr(ignored_list, ':')))
		*p = '\n';

	signal_emit(GGadu_PLUGIN_NAME, "gui show window with text", ignored_list, "main-gui");

	return NULL;
}

static gpointer ignore_add_list_action(gpointer user_data)
{
	GGaduDialog *dialog;

	dialog = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("Add to ignored"), "ignore dialog add id");
	ggadu_dialog_add_entry(dialog, GGADU_IGNORE_ADD_ID, _("ID to ignore:"), VAR_STR, NULL, VAR_FLAG_NONE);
	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");

	return NULL;
}

static gpointer ignore_remove_list_action(gpointer user_data)
{
	GGaduDialog *dialog;

	dialog = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("Removed from ignored"), "ignore dialog remove id");
	ggadu_dialog_add_entry(dialog, GGADU_IGNORE_REMOVE_ID, _("ID to remove:"), VAR_STR, NULL, VAR_FLAG_NONE);
	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");

	return NULL;
}


/* MAIN */
void start_plugin()
{
	GGaduMenu *item_gg;

	ext_in = g_new0(GGaduPluginExtension, 1);
	ext_in->type = GGADU_PLUGIN_EXTENSION_USER_MENU_TYPE;
	ext_in->callback = ignore_un_ignore_action;
	ext_in->txt = _("(Un)Ig_nore");
	register_extension_for_plugin(ext_in, GGADU_PLUGIN_TYPE_PROTOCOL);

	menu_ignoremenu = ggadu_menu_create();
	item_gg = ggadu_menu_add_item(menu_ignoremenu, _("_Ignore"), NULL, NULL);
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("_Add"), ignore_add_list_action, NULL));
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("_Remove"), ignore_remove_list_action, NULL));
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item("", NULL, NULL));
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("_List"), ignore_show_list_action, NULL));

	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_ignoremenu, "main-gui");
}

/* PLUGIN INITIALISATION */
GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
	gchar *this_configdir = NULL;

	GGadu_PLUGIN_ACTIVATE(conf_ptr);
	ignore_handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("Ignore option support"));

	IGNORE_ADD_CONTACT_SIG = register_signal(ignore_handler, "ignore add contact");
	IGNORE_REMOVE_CONTACT_SIG = register_signal(ignore_handler, "ignore remove contact");
	IGNORE_CHECK_CONTACT_SIG = register_signal(ignore_handler, "ignore check contact");
	IGNORE_DIALOG_ADD_ID_SIG = register_signal(ignore_handler, "ignore dialog add id");
	IGNORE_DIALOG_REMOVE_ID_SIG = register_signal(ignore_handler, "ignore dialog remove id");

	if (g_getenv("HOME_ETC"))
		this_configdir = g_build_filename(g_getenv("HOME_ETC"), "gg2", NULL);
	else
		this_configdir = g_build_filename(g_get_home_dir(), ".gg2", NULL);

	ggadu_config_set_filename(ignore_handler, g_build_filename(this_configdir, "ignore-main", NULL));
	ggadu_config_var_add(ignore_handler, "list", VAR_STR);

	if (!ggadu_config_read(ignore_handler))
		g_warning(_("Unable to read configuration file for plugin %s"), "");

	register_signal_receiver(ignore_handler, (signal_func_ptr) my_signal_receive);

	g_free(this_configdir);
	return ignore_handler;
}

void destroy_plugin()
{
	print_debug("destroy_plugin %s", GGadu_PLUGIN_NAME);

	if (ext_in)
		unregister_extension_for_plugins(ext_in);

	signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", menu_ignoremenu, "main-gui");

}
