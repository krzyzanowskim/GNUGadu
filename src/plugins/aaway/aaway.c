/*
 * Auto-away plugin for GNU Gadu 2 
 * 
 * Copyright (C) 2003-2004 GNU Gadu Team 
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


gint ENABLE;
gint INTERVAL;
gint ENABLE_MESSAGE;
gchar *MESSAGE;
guint timer_handle;

GGaduPlugin *handler;
GGaduMenu *menu_pluginmenu;
XScreenSaverInfo *mit_info = NULL;

GGadu_PLUGIN_INIT("aaway", GGADU_PLUGIN_TYPE_MISC);

/* zwraca wartosc czasu bezczynnosci w minutach 
	@return gint : idle_time - w minutach - gdy brak bezczynnosci zwraca 0 */
gint get_idle()
{
	int event_base, error_base;
	int idle_time;
	if (XScreenSaverQueryExtension(GDK_DISPLAY(), &event_base, &error_base))
	{
		if (mit_info == NULL)
		{
			mit_info = XScreenSaverAllocInfo();
		}
		XScreenSaverQueryInfo(GDK_DISPLAY(), GDK_ROOT_WINDOW(), mit_info);
		idle_time = (mit_info->idle) / 1000;
	}
	else
	{
		idle_time = 0;
	}

	return idle_time / 60;
}

/* procedura sprawdzajaca czas idle oraz zmieniajaca stan protokolow komunika-
	cyjnych zgodnie z stanem bezczynnosci */
gboolean check_idle_time()
{
	gint local_idle = get_idle();
	static gboolean away_enabled = FALSE;
	
	if ((local_idle >= INTERVAL) && ENABLE && !away_enabled)
	{
		GSList *plugins = config->loaded_plugins;
		
		print_debug("%s : Setting AWAY state\n", GGadu_PLUGIN_NAME);

		while (plugins)
		{
		    GGaduPlugin *plugin = (GGaduPlugin *) plugins->data;
		    if (plugin && plugin->protocol && (plugin->type == GGADU_PLUGIN_TYPE_PROTOCOL))
		    {
			GGaduStatusPrototype *sp = signal_emit(GGadu_PLUGIN_NAME, "get current status", NULL, plugin->name);
			
			if (is_in_status(sp->status, plugin->protocol->online_status))
			{
			    gchar *message = g_strdup(ggadu_config_var_get(handler, "message"));
			    GGaduDialog *d = ggadu_dialog_new(GGADU_DIALOG_GENERIC, NULL, NULL);
			    GGaduKeyValue *kv = g_new0(GGaduKeyValue, 1);
			    gint newstatus =  (gint)plugin->protocol->away_status->data;
			    
			    d->response = GGADU_OK;
			    kv->value = (gpointer) message;
			    d->optlist = g_slist_append(d->optlist, kv);
			    d->user_data = ggadu_find_status_prototype(plugin->protocol, newstatus);
			    
			    signal_emit(GGadu_PLUGIN_NAME, "change status descr", d, plugin->name);
			    g_free(message);
			    away_enabled = TRUE;
			    print_debug("SET %d %s",newstatus,plugin->name);
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
		    if (plugin && plugin->protocol && (plugin->type == GGADU_PLUGIN_TYPE_PROTOCOL))
		    {
			GGaduStatusPrototype *sp = signal_emit(GGadu_PLUGIN_NAME, "get current status", NULL, plugin->name);
			
			if (is_in_status(sp->status, plugin->protocol->away_status))
			{
			    gchar *message = g_strdup(ggadu_config_var_get(handler, "message"));
			    GGaduDialog *d = ggadu_dialog_new(GGADU_DIALOG_GENERIC, NULL, NULL);
			    GGaduKeyValue *kv = g_new0(GGaduKeyValue, 1);
			    gint newstatus =  (gint)plugin->protocol->online_status->data;
			    
			    d->response = GGADU_OK;
			    kv->value = (gpointer) message;
			    d->optlist = g_slist_append(d->optlist, kv);
			    d->user_data = ggadu_find_status_prototype(plugin->protocol,newstatus);
			    signal_emit(GGadu_PLUGIN_NAME, "change status descr", d, plugin->name);
			    g_free(message);
			}
			
		    }
		    plugins = plugins->next;
		}

		away_enabled = FALSE;
	}


	return TRUE;
}


/* strictly debug stuff */
void timeout_remove()
{
	print_debug("%s : Timeout REMOVED !\n", GGadu_PLUGIN_NAME);
}

void my_signal_receive(gpointer name, gpointer signal_ptr)
{
	GGaduSignal *signal = (GGaduSignal *) signal_ptr;

	print_debug("%s : receive signal %d\n", GGadu_PLUGIN_NAME, signal->name);

	if (signal->name == g_quark_from_static_string("update config"))
	{
		GGaduDialog *d = signal->data;
		gchar *utf = NULL;

		if (ggadu_dialog_get_response(d) == GGADU_OK)
		{

			GSList *tmplist = ggadu_dialog_get_entries(d);

			while (tmplist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
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
					break;

				}
				tmplist = tmplist->next;
			}
			ggadu_config_save(handler);
			set_configuration();
		}
		g_free(utf);
		GGaduDialog_free(d);
		return;
	}


	return;
}

gint set_configuration(void)
{
	ENABLE = GGADU_AAWAY_CONFIG_DEFAULT_ENABLE_AUTOAWAY;
	INTERVAL = GGADU_AAWAY_CONFIG_DEFAULT_INTERVAL;
	ENABLE_MESSAGE = GGADU_AAWAY_CONFIG_DEFAULT_ENABLE_AWAY_MSG;
	MESSAGE = GGADU_AAWAY_CONFIG_DEFAULT_AWAY_MSG;

	if (!ggadu_config_var_check(handler, "enable"))
		print_debug("aaway: No enable config found, setting default\n");
	else
		ENABLE = (gint) ggadu_config_var_get(handler, "enable");

	if (!ggadu_config_var_check(handler, "interval"))
		print_debug("aaway: No interval config found, setting default\n");
	else
		INTERVAL = (gint) ggadu_config_var_get(handler, "interval");

	if (!ggadu_config_var_check(handler, "enable_message"))
		print_debug("aaway: No enable_message config found, setting default\n");
	else
		ENABLE_MESSAGE = (gint) ggadu_config_var_get(handler, "enable_message");

	if (!ggadu_config_var_check(handler, "message"))
		print_debug("aaway: No message config found, setting default\n");
	else
		MESSAGE = (gchar *) ggadu_config_var_get(handler, "message");

	return 1;
}

gpointer aaway_preferences(gpointer user_data)
{
	GGaduDialog *d;

	print_debug("%s: Preferences\n", "aaway");

	d = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("Auto-Away Preferences"), "update config");

//	gchar *utf = NULL;
	ggadu_dialog_add_entry(d, GGADU_AAWAY_CONFIG_ENABLE_AUTOAWAY, _("Enable auto-away"), VAR_BOOL, (gpointer) ggadu_config_var_get(handler, "enable"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(d, GGADU_AAWAY_CONFIG_INTERVAL, _("Auto away after time (minutes)"), VAR_INT, (gpointer) ggadu_config_var_get(handler, "interval"), VAR_FLAG_NONE);
/*	ggadu_dialog_add_entry(d, GGADU_AAWAY_CONFIG_ENABLE_AWAY_MSG, _("Enable away message"), VAR_BOOL,
			       (gpointer) ggadu_config_var_get(handler, "enable_message"), VAR_FLAG_NONE);
	utf = to_utf8("ISO-8859-2", ggadu_config_var_get(handler, "message"));
	ggadu_dialog_add_entry(d, GGADU_AAWAY_CONFIG_AWAY_MSG, _("Away message (%s-time)"), VAR_STR, utf, VAR_FLAG_NONE);
*/
	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");

	return NULL;
}

GGaduMenu *build_plugin_menu()
{
	GGaduMenu *root = ggadu_menu_create();
	GGaduMenu *item_gg = ggadu_menu_add_item(root, "Auto-Away", NULL, NULL);
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Preferences"), aaway_preferences, NULL));

	return root;
}

void start_plugin()
{
	print_debug("%s : start_plugin\n", GGadu_PLUGIN_NAME);

	/* Menu stuff */
	print_debug("%s : Create Menu\n", GGadu_PLUGIN_NAME);
	menu_pluginmenu = build_plugin_menu();
	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_pluginmenu, "main-gui");

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

	print_debug("%s : READ CONFIGURATION\n", GGadu_PLUGIN_NAME);

	if (g_getenv("HOME_ETC"))
		this_configdir = g_build_filename(g_getenv("HOME_ETC"), "gg2", NULL);
	else
		this_configdir = g_build_filename(g_get_home_dir(), ".gg2", NULL);

	ggadu_config_set_filename((GGaduPlugin *) handler, g_build_filename(this_configdir, "aaway", NULL));

	g_free(this_configdir);

	ggadu_config_var_add(handler, "enable", VAR_BOOL);
	ggadu_config_var_add(handler, "interval", VAR_INT);
	ggadu_config_var_add(handler, "enable_message", VAR_BOOL);
	ggadu_config_var_add(handler, "message", VAR_STR);

	if (!ggadu_config_read(handler))
		g_warning(_("Unable to read configuration file for plugin %s"), "aaway");

	set_configuration();

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
