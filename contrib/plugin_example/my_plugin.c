/* $Id: my_plugin.c,v 1.15 2004/12/26 22:23:14 shaster Exp $ */

/* 
 * Example: plugin code for GNU Gadu 2 
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

/*

	This is example plugin based on sound-external plugin
	type ./build.sh to build plugin outside the gg2 tree
	after compile copy to $prefix/lib/gg2/my-plugin.so
	and should work.
	
	NOTE you have to have installed gg2 to compile this plugin

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gg2_core.h"
#include "my_plugin.h"

static GGaduPlugin *handler;
static GGaduMenu *menu_pluginmenu = NULL;

GGadu_PLUGIN_INIT("my-plugin", GGADU_PLUGIN_TYPE_MISC);

static gpointer ggadu_play_file(gpointer user_data)
{
	gchar *cmd;
	if (!ggadu_config_var_get(handler, "player"))
		return NULL;

	cmd = g_strdup_printf("%s %s", (char *) ggadu_config_var_get(handler, "player"), (gchar *) user_data);

	system(cmd);
	g_free(cmd);
	return NULL;
}

static gpointer se_preferences(gpointer user_data)
{
	GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("My Plugin preferences"), "update config");

	print_debug("%s: Preferences\n", "My Plugin");
	ggadu_dialog_add_entry(dialog, GGADU_SE_CONFIG_PLAYER, _("Player program name"), VAR_STR, ggadu_config_var_get(handler, "player"), VAR_FLAG_NONE);
	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");

	return NULL;
}


/* SIGNALS HANDLER */
static void my_signal_receive(gpointer name, gpointer signal_ptr)
{
	GGaduSignal *signal = (GGaduSignal *) signal_ptr;

	print_debug("%s : receive signal %d", GGadu_PLUGIN_NAME, signal->name);

	if (signal->name == SOUND_PLAY_FILE_SIG)
	{
		gchar *filename = signal->data;

		if (filename != NULL)
			g_thread_create(ggadu_play_file, filename, FALSE, NULL);
	}

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
				case GGADU_SE_CONFIG_PLAYER:
					print_debug("changing var setting player to %s\n", kv->value);
					ggadu_config_var_set(handler, "player", kv->value);
					break;
				}
				tmplist = tmplist->next;
			}
			ggadu_config_save(handler);
		}
		GGaduDialog_free(dialog);
	}
}



/* MENU */
static GGaduMenu *build_plugin_menu()
{
	GGaduMenu *root = ggadu_menu_create();
	GGaduMenu *item_gg = ggadu_menu_add_item(root, "My Plugin", NULL, NULL);
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Preferences"), se_preferences, NULL));

	return root;
}

/* MAIN */

void start_plugin()
{
	menu_pluginmenu = build_plugin_menu();
	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_pluginmenu, "main-gui");
}

/* PLUGIN INITIALISATION */
GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
	gchar *this_configdir;

	print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);

	GGadu_PLUGIN_ACTIVATE(conf_ptr); /* THIS IS IMPORTANT HERE */
	handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("External player sound driver"));

	SOUND_PLAY_FILE_SIG = register_signal(handler, "sound play file");
	UPDATE_CONFIG_SIG = register_signal(handler, "update config");

	if (g_getenv("HOME_ETC"))
		this_configdir = g_build_filename(g_getenv("HOME_ETC"), "gg2", NULL);
	else
		this_configdir = g_build_filename(g_get_home_dir(), ".gg2", NULL);

	ggadu_config_set_filename((GGaduPlugin *) handler, g_build_filename(this_configdir, "my-plugin", NULL));
	ggadu_config_var_add(handler, "player", VAR_STR);

	if (!ggadu_config_read(handler))
		g_warning(_("Unable to read configuration file for plugin %s"), "");

	register_signal_receiver((GGaduPlugin *) handler, (signal_func_ptr) my_signal_receive);

	g_free(this_configdir);
	return handler;
}

void destroy_plugin()
{

	print_debug("destroy_plugin %s", GGadu_PLUGIN_NAME);

	if (menu_pluginmenu)
		signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", menu_pluginmenu, "main-gui");
}
