/*
 * Auto-away plugin for GNU Gadu 2 
 * 
 * Copyright (C) 2003-2005 GNU Gadu Team 
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

 /* authors: slawek mikula <zorba@silesianet.pl> 
             Marcin Krzyzanowski <krzak@hakore.com> 
 */

 /* TODO:
    - pobieranie opisu statusow dla kazdego protokolu
    - 
  */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gg2_core.h>

#include <gtk/gtk.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time.h>
#include <signal.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/scrnsaver.h>
#include <gdk/gdkx.h>

#include "aaway.h"

guint timer_handle;

static GGaduPlugin *handler;
static GGaduMenu *menu_pluginmenu;

GGadu_PLUGIN_INIT("aaway", GGADU_PLUGIN_TYPE_MISC);

/* zwraca wartosc czasu bezczynnosci w minutach 
	@return gint : idle_time - w minutach - gdy brak bezczynnosci zwraca 0 */
gint get_idle()
{
	int idle_time;
	int event_base, error_base;
	XScreenSaverInfo *mit_info = NULL;

	if (XScreenSaverQueryExtension(GDK_DISPLAY(), &event_base, &error_base))
	{
		mit_info = XScreenSaverAllocInfo();
		XScreenSaverQueryInfo(GDK_DISPLAY(), GDK_ROOT_WINDOW(), mit_info);
		idle_time = (mit_info->idle) / 1000;
		XFree(mit_info);
	}
	else
	{
		idle_time = 0;
	}

	return idle_time / 60;
}

/* 
   procedura sprawdzajaca czas idle oraz zmieniajaca stan protokolow komunikacyjnych zgodnie 
   z stanem bezczynnosci 
*/
static gboolean check_idle_time()
{
	gint local_idle = get_idle();
	static gboolean away_enabled = FALSE;
	
	if ((local_idle >= (gint) ggadu_config_var_get(handler, "interval")) && !away_enabled && (gint) ggadu_config_var_get(handler, "enable"))
	{
		GSList *plugins = config->loaded_plugins;
		
		print_debug("%s : Setting AWAY state\n", GGadu_PLUGIN_NAME);

		while (plugins)
		{
		    GGaduPlugin *plugin = (GGaduPlugin *) plugins->data;
		    GGaduProtocol *protocol = plugin->plugin_data;
		    if (plugin && protocol && (plugin->type == GGADU_PLUGIN_TYPE_PROTOCOL))
		    {
			print_debug("lustruje %s",plugin->name);
			GGaduStatusPrototype *sp = signal_emit(GGadu_PLUGIN_NAME, "get current status", NULL, plugin->name);
			
			if (sp && ggadu_is_in_status(sp->status, protocol->online_status))
			{
			    GGaduStatusPrototype *sp2;
			    gchar *message = NULL;
			    gint newstatus;

			    if (sp->status_description)
			    {
				message = g_strdup(sp->status_description);
			    } else
			    {
				message = g_strdup(ggadu_config_var_get(handler, "message"));
			    }

			    newstatus = (gint)protocol->away_status->data;
			    sp2 = ggadu_find_status_prototype(protocol, newstatus);
			    sp2->status_description = message;
			    
			    print_debug("change from %d to %d",sp->status,newstatus);
			    signal_emit(GGadu_PLUGIN_NAME, "change status", sp2, plugin->name);
			    
			    away_enabled = TRUE;
			    
			    print_debug("SET %d %s",newstatus,plugin->name);
//			    g_free(message);
			}
		    }
		    plugins = plugins->next;
		}
	}
	else if ((local_idle == 0) && away_enabled)
	{
		GSList *plugins = config->loaded_plugins;
		
		print_debug("%s : Setting ACTIVE state\n", GGadu_PLUGIN_NAME);

		while (plugins)
		{
		    GGaduPlugin *plugin = (GGaduPlugin *) plugins->data;
		    GGaduProtocol *protocol = plugin->plugin_data;
		    if (plugin && protocol && (plugin->type == GGADU_PLUGIN_TYPE_PROTOCOL))
		    {
			GGaduStatusPrototype *sp = signal_emit(GGadu_PLUGIN_NAME, "get current status", NULL, plugin->name);
			if (sp && ggadu_is_in_status(sp->status, protocol->away_status))
			{
			    gchar *message = NULL;
			    GGaduStatusPrototype *sp2;
			    gint newstatus;

			    if (sp->status_description && 
			        !strstr(sp->status_description,ggadu_config_var_get(handler, "message")))
			    {
				message = g_strdup(sp->status_description);
			    }
			    
			    newstatus = (gint)protocol->online_status->data;
			    
			    sp2 = ggadu_find_status_prototype(protocol, newstatus);
			    sp2->status_description = message;
			    
			    
			    print_debug("change from %d to %d",sp->status,newstatus);
 			    signal_emit(GGadu_PLUGIN_NAME, "change status", sp2, plugin->name);
//			    g_free(message); 
			}
		    }
		    plugins = plugins->next;
		}
		away_enabled = FALSE;
	}
	return TRUE;
}


/* strictly debug stuff */
static void timeout_remove()
{
	print_debug("%s : Timeout REMOVED !\n", GGadu_PLUGIN_NAME);
}

static void my_signal_receive(gpointer name, gpointer signal_ptr)
{
	GGaduSignal *signal = (GGaduSignal *) signal_ptr;

	print_debug("%s : receive signal %d\n", GGadu_PLUGIN_NAME, signal->name);

	if (signal->name == g_quark_from_static_string("update config"))
	{
		GGaduDialog *dialog = signal->data;
		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			GSList *entries = ggadu_dialog_get_entries(dialog);
			gchar *utf = NULL;
			while (entries)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) entries->data;
				switch (kv->key)
				{
				case GGADU_AAWAY_CONFIG_ENABLE_AUTOAWAY:
					print_debug("%s : changing var setting enable-autoaway to %d\n", GGadu_PLUGIN_NAME, kv->value);
					ggadu_config_var_set(handler, "enable", kv->value);
					break;
				case GGADU_AAWAY_CONFIG_INTERVAL:
					print_debug("%s : changing var setting interval to %d\n", GGadu_PLUGIN_NAME, kv->value);
					ggadu_config_var_set(handler, "interval", kv->value);
					break;
				case GGADU_AAWAY_CONFIG_ENABLE_AWAY_MSG:
					print_debug("%s - changing var setting enable_message to %d\n", GGadu_PLUGIN_NAME, kv->value);
					ggadu_config_var_set(handler, "enable_message", kv->value);
					break;
				case GGADU_AAWAY_CONFIG_AWAY_MSG:
					print_debug("%s - changing var setting message to %s\n", GGadu_PLUGIN_NAME, kv->value);
					utf = from_utf8("ISO-8859-2", kv->value);
					ggadu_config_var_set(handler, "message", utf);
					g_free(utf);
					break;

				}
				entries = entries->next;
			}
			ggadu_config_save(handler);
		}
		GGaduDialog_free(dialog);
	}
}

static gpointer aaway_preferences(gpointer user_data)
{
	GGaduDialog *d = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("Auto-Away Preferences"), "update config");
	print_debug("%s: Preferences\n", "aaway");
//	gchar *utf = NULL;
	ggadu_dialog_add_entry(d, GGADU_AAWAY_CONFIG_ENABLE_AUTOAWAY, _("Enable auto-away"), VAR_BOOL, (gpointer) ggadu_config_var_get(handler, "enable"), VAR_FLAG_ADVANCED);
	ggadu_dialog_add_entry(d, GGADU_AAWAY_CONFIG_INTERVAL, _("Auto away after time (minutes)"), VAR_INT, (gpointer) ggadu_config_var_get(handler, "interval"), VAR_FLAG_NONE);
/*	ggadu_dialog_add_entry(d, GGADU_AAWAY_CONFIG_ENABLE_AWAY_MSG, _("Enable away message"), VAR_BOOL,
			       (gpointer) ggadu_config_var_get(handler, "enable_message"), VAR_FLAG_NONE);
	utf = to_utf8("ISO-8859-2", ggadu_config_var_get(handler, "message"));
	ggadu_dialog_add_entry(d, GGADU_AAWAY_CONFIG_AWAY_MSG, _("Away message (%s-time)"), VAR_STR, utf, VAR_FLAG_NONE);
*/

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");
	return NULL;
}

static void build_plugin_menu()
{
	GGaduMenu *root = ggadu_menu_create();
	GGaduMenu *item_aa = ggadu_menu_add_item(root, "Auto-Away", NULL, NULL);
	ggadu_menu_add_submenu(item_aa, ggadu_menu_new_item(_("Preferences"), aaway_preferences, NULL));
	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", root, "main-gui");
	menu_pluginmenu = root;
}

void start_plugin()
{
	print_debug("%s : start_plugin\n", GGadu_PLUGIN_NAME);

	build_plugin_menu();

	register_signal(handler, "update config");

	print_debug("%s : Start Timer\n", GGadu_PLUGIN_NAME);
	timer_handle = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 1000, check_idle_time, NULL, timeout_remove);
}


GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
	gchar *this_configdir = NULL;
	print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);
	GGadu_PLUGIN_ACTIVATE(conf_ptr);
	handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("Auto Away"));

	if (g_getenv("HOME_ETC"))
		this_configdir = g_build_filename(g_getenv("HOME_ETC"), "gg2", NULL);
	else
		this_configdir = g_build_filename(g_get_home_dir(), ".gg2", NULL);

	ggadu_config_set_filename((GGaduPlugin *) handler, g_build_filename(this_configdir, "aaway", NULL));
	g_free(this_configdir);

	ggadu_config_var_add_with_default(handler, "enable", VAR_BOOL, (gpointer) TRUE);
	ggadu_config_var_add_with_default(handler, "interval", VAR_INT, (gpointer) 5);
	ggadu_config_var_add_with_default(handler, "enable_message", VAR_BOOL,(gpointer) FALSE);
	ggadu_config_var_add_with_default(handler, "message", VAR_STR, _("I'm away from computer"));

	if (!ggadu_config_read(handler))
		g_warning(_("Unable to read configuration file for plugin %s"), "aaway");

	register_signal_receiver((GGaduPlugin *) handler, (signal_func_ptr) my_signal_receive);
	return handler;
}

void destroy_plugin()
{
	print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);

	if (menu_pluginmenu)
	{
		signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", menu_pluginmenu, "main-gui");
		ggadu_menu_free(menu_pluginmenu);
	}

	if (timer_handle)
	{
		g_source_remove(timer_handle);
	}

}
