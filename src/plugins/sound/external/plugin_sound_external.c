/* $Id: plugin_sound_external.c,v 1.32 2006/07/21 23:09:16 krzyzak Exp $ */

/* 
 * sound-external plugin for GNU Gadu 2 
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

#include <glib.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "ggadu_types.h"
#include "plugins.h"
#include "signals.h"
#include "ggadu_menu.h"
#include "ggadu_conf.h"
#include "ggadu_support.h"
#include "ggadu_dialog.h"
#include "plugin_sound_external.h"

static GGaduPlugin *handler;
static GGaduMenu *menu_pluginmenu = NULL;

GGadu_PLUGIN_INIT("sound-external", GGADU_PLUGIN_TYPE_MISC);

static gpointer ggadu_play_file(gpointer user_data)
{
    static GStaticMutex play_mutex = G_STATIC_MUTEX_INIT;
    gchar *cmd_native = NULL;
    gchar *cmd = NULL;
    gsize r,w;
    
    g_static_mutex_lock(&play_mutex);
    
    if (!ggadu_config_var_get(handler, "player"))
    {
	g_static_mutex_unlock(&play_mutex);
	return NULL;
    }
    
    cmd = g_strdup(ggadu_config_var_get(handler, "player"));
    cmd_native = g_filename_from_utf8(cmd,strlen(cmd),&r,&w,NULL);

    if (!ggadu_spawn(cmd_native,(gchar *)user_data))
          signal_emit_from_thread(GGadu_PLUGIN_NAME, "gui show message", g_strdup(_("External sound plugin: Error while playing file")), "main-gui");
    
/*    if ((WIFSIGNALED(ret) && (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT)) || (ret == -1))
    {
    }
*/    
    g_free(cmd_native);
    g_free(cmd);

    g_static_mutex_unlock(&play_mutex);    
    return NULL;
}

void my_signal_receive(gpointer name, gpointer signal_ptr)
{
    GGaduSignal *signal = (GGaduSignal *) signal_ptr;

    print_debug("%s : receive signal %d\n", GGadu_PLUGIN_NAME, signal->name);

    if (signal->name == g_quark_from_static_string("sound play file"))
    {
	gchar *filename = signal->data;

	if ((filename != NULL) && g_file_test(filename, G_FILE_TEST_IS_REGULAR))
	    g_thread_create(ggadu_play_file, filename, FALSE, NULL);
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


static gpointer se_preferences(gpointer user_data)
{
    GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("External sound preferences"), "update config");

    print_debug("%s: Preferences\n", "Sound external");

    ggadu_dialog_add_entry(dialog, GGADU_SE_CONFIG_PLAYER, _("Player program name"), VAR_FILE_CHOOSER, ggadu_config_var_get(handler, "player"),
			   VAR_FLAG_NONE);

    signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");

    return NULL;
}

static GGaduMenu *build_plugin_menu()
{
    GGaduMenu *root = ggadu_menu_create();
    GGaduMenu *item_gg = ggadu_menu_add_item(root, "Sound", NULL, NULL);
    ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Preferences"), se_preferences, NULL));

    return root;
}


void start_plugin()
{
    menu_pluginmenu = build_plugin_menu();
    signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_pluginmenu, "main-gui");
}

GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
    gchar *path = NULL;

    print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);

    GGadu_PLUGIN_ACTIVATE(conf_ptr);
    handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("External player sound driver"));

    register_signal(handler, "sound play file");
    register_signal(handler, "update config");

    path = g_build_filename(config->configdir, "sound-external", NULL);
    ggadu_config_set_filename((GGaduPlugin *) handler, path);
    g_free(path);

    ggadu_config_var_add_with_default(handler, "player", VAR_STR,"/usr/bin/play %s");

    if (!ggadu_config_read(handler))
	g_warning(_("Unable to read configuration file for plugin %s"), "");

    register_signal_receiver((GGaduPlugin *) handler, (signal_func_ptr) my_signal_receive);

    return handler;
}

void destroy_plugin()
{
    print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
    if (menu_pluginmenu)
    {
	signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", menu_pluginmenu, "main-gui");
    }
}

