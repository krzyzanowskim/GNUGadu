/* $Id: history_viewer.c,v 1.14 2005/01/19 21:02:16 krzyzak Exp $ */

/* 
 * Plugin code for GNU Gadu 2 
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
#include "history_viewer.h"

static GGaduPlugin *handler;
static GGaduMenu *menu_pluginmenu = NULL;
static GGaduPluginExtension *ext = NULL;	/* extension to handle vie hisatory from foreign plugins */

GGadu_PLUGIN_INIT("history-external", GGADU_PLUGIN_TYPE_MISC);

gpointer history_external_preferences(gpointer user_data)
{
	GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("External History Viewer Preferences"), "update config");

	print_debug("%s: Preferences\n", "External history viewer");
	ggadu_dialog_add_entry(dialog, GGADU_HISTORY_CONFIG_VIEWER, _("_Path to external viewer:"), VAR_FILE_CHOOSER, ggadu_config_var_get(handler, "viewer"), VAR_FLAG_NONE);
	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");

	return NULL;
}


/* SIGNALS HANDLER */
void my_signal_receive(gpointer name, gpointer signal_ptr)
{
	GGaduSignal *signal = (GGaduSignal *) signal_ptr;

	if (signal && signal->name == UPDATE_CONFIG_SIG)
	{
		GGaduDialog *dialog = signal->data;
		print_debug("%s : receive signal %d", GGadu_PLUGIN_NAME, signal->name);

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			GSList *tmplist = ggadu_dialog_get_entries(dialog);
			while (tmplist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
				switch (kv->key)
				{
				case GGADU_HISTORY_CONFIG_VIEWER:
					print_debug("changing var setting viewer to %s\n", kv->value);
					ggadu_config_var_set(handler, "viewer", kv->value);
					break;
				}
				tmplist = tmplist->next;
			}
			ggadu_config_save(handler);
		}
		GGaduDialog_free(dialog);
	}
}


gpointer show_external_history(gpointer user_data)
{
	GSList *users = (GSList *) user_data;
	GGaduContact *k = (users) ? (GGaduContact *) users->data : NULL;
	gchar *path;

	if (!k)
	{
		signal_emit(GGadu_PLUGIN_NAME, "gui show message", g_strdup(_("User not selected")), "main-gui");
		return NULL;
	}
	    
	path = g_strdup_printf("%s/%s/%s",config->configdir, "history", k->id);
	ggadu_spawn(ggadu_config_var_get(handler, "viewer"),path);
	g_free(path);
	return NULL;
}


/* MENU */
GGaduMenu *build_plugin_menu()
{
	GGaduMenu *root = ggadu_menu_create();
	GGaduMenu *item_gg = ggadu_menu_add_item(root, _("History _Viewer"), NULL, NULL);
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Preferences"), history_external_preferences, NULL));

	return root;
}

/* MAIN */
void start_plugin()
{

	ext = g_new0(GGaduPluginExtension, 1);
	ext->type = GGADU_PLUGIN_EXTENSION_USER_MENU_TYPE;
	ext->callback = show_external_history;
	ext->txt = _("View _History");

	register_extension_for_plugin(ext, GGADU_PLUGIN_TYPE_PROTOCOL);

	menu_pluginmenu = build_plugin_menu();
	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_pluginmenu, "main-gui");
}

/* PLUGIN INITIALISATION */
GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
	print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);

	GGadu_PLUGIN_ACTIVATE(conf_ptr);
	handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("External history viewer"));

	HISTORY_SHOW_FILE_SIG = register_signal(handler, "history show file");
	UPDATE_CONFIG_SIG = register_signal(handler, "update config");

	ggadu_config_set_filename((GGaduPlugin *) handler, g_build_filename(config->configdir, "history-external", NULL));

	ggadu_config_var_add_with_default(handler, "viewer", VAR_STR, g_build_filename(BINDIR, "gghist %s", NULL));

	if (!ggadu_config_read(handler))
		g_warning(_("Unable to read configuration file for plugin %s"), "");

	register_signal_receiver((GGaduPlugin *) handler, (signal_func_ptr) my_signal_receive);

	return handler;
}

void destroy_plugin()
{

	print_debug("destroy_plugin %s", GGadu_PLUGIN_NAME);

	if (menu_pluginmenu)
	{
		signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", menu_pluginmenu, "main-gui");
	}

	if (ext)
	{
		unregister_extension_for_plugins(ext);
	}
}
