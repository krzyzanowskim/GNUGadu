/* $Id: update_plugin.c,v 1.22 2004/05/04 21:39:13 krzyzak Exp $ */

/*  
 * Update plugin for GNU Gadu 2  
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
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "ggadu_types.h"
#include "plugins.h"
#include "ggadu_conf.h"
#include "signals.h"
#include "ggadu_menu.h"
#include "ggadu_support.h"
#include "ggadu_dialog.h"
#include "update_plugin.h"

GGaduPlugin *update_handler;

GGaduMenu *menu_updatemenu;
guint timer = -1;

GGadu_PLUGIN_INIT("update", GGADU_PLUGIN_TYPE_MISC);

/* helper */
gint update_get_interval(void)
{
	return (((gint) ggadu_config_var_get(update_handler, "check_interval") > 0 ?
		(gint) ggadu_config_var_get(update_handler, "check_interval") * 60 : 3600) * 1000);
}

/* helper */
gint update_use_xosd(void)
{
	return (((gint) ggadu_config_var_get(update_handler, "use_xosd") && find_plugin_by_name("xosd")) ? 1 : 0);
}

/* connects, parses.
 * returns pointer to version string
 * HEAVILY RELIES ON SF.NET's RSS LAYOUT.
 */
gchar *update_get_current_version(gboolean show)
{
	int sock_r, sock_s;
	gint i = 0;
	struct hostent *h;
	struct sockaddr_in servAddr;
	const gchar *server = GGADU_UPDATE_SERVER;
	gchar temp[2];
	gchar *get = NULL, *recv_buff = NULL, *buf = NULL, *buf1 = NULL;
	gchar *reply = NULL;

	/* where to connect */
	if (server == NULL)
		return NULL;

	/* resolve server */
	if ((h = gethostbyname(server)) == NULL)
	{
		print_debug("%s : Unknown host %s\n", GGadu_PLUGIN_NAME, server);
		if (show)
		{
			if (update_use_xosd())
				signal_emit(GGadu_PLUGIN_NAME, "xosd show message", g_strdup_printf(_("Unknown host: %s"), server), "xosd");
			else
				signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup_printf(_("Unknown host: %s"), server), "main-gui");
		}
		return NULL;
	}

	/* create a socket */
	sock_s = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_s < 0)
	{
		print_debug("%s : ERROR: Cannot create socket\n", GGadu_PLUGIN_NAME);
		return NULL;
	}

	/* connect */
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(GGADU_UPDATE_PORT);
	servAddr.sin_addr = *((struct in_addr *) h->h_addr);
	bzero(&(servAddr.sin_zero), 8);
	sock_r = connect(sock_s, (struct sockaddr *) &servAddr, sizeof (struct sockaddr));

	if (sock_r < 0)
	{
		print_debug("%s : Cannot connect\n", GGadu_PLUGIN_NAME);
		if (show)
		{
			if (update_use_xosd())
				signal_emit(GGadu_PLUGIN_NAME, "xosd show message", g_strdup_printf(_("Error while connecting to %s"), server),	"xosd");
			else
				signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup_printf(_("Error while connecting to %s"), server), "main-gui");
		}
		return NULL;
	}

	/* *INDENT-OFF* */
	/* the GET request */
	get = g_strdup_printf("GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n",
			GGADU_UPDATE_URL, server, GGADU_UPDATE_USERAGENT_STRING);
	/* *INDENT-ON* */
	send(sock_s, get, strlen(get), MSG_WAITALL);
	g_free(get);

	/* allocate receive buffers */
	recv_buff = g_malloc0(GGADU_UPDATE_BUFLEN);

	/* receiving data */
	while (recv(sock_s, temp, 1, MSG_WAITALL) && i < GGADU_UPDATE_BUFLEN)
		recv_buff[i++] = temp[0];

	/* disconnect */
	close(sock_s);

	/* check for "200 OK" response */
	if ((buf = g_strstr_len(recv_buff, i, "200 OK")) == NULL)
	{
		if (show)
		{
			if (update_use_xosd())
				signal_emit(GGadu_PLUGIN_NAME, "xosd show message", g_strdup(_("Server-side error during update check")), "xosd");
			else
				signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Server-side error during update check")), "main-gui");
		}
		g_free(recv_buff);
		return NULL;
	}

	/* Search for first occurence of: <title>gg2 XXXXXX released */
	if ((buf = g_strstr_len(recv_buff, i, "<title>gg2")) == NULL)
	{
		if (show)
		{
			if (update_use_xosd())
				signal_emit(GGadu_PLUGIN_NAME, "xosd show message", g_strdup(_("Malformed server reply")), "xosd");
			else
				signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Malformed server reply")), "main-gui");
		}
		g_free(recv_buff);
		return NULL;
	}

	buf1 = g_strstr_len(buf + 11, strlen(buf) - 11, " released");
	if (!buf1)
	{
		if (show)
		{
			if (update_use_xosd())
				signal_emit(GGadu_PLUGIN_NAME, "xosd show message", g_strdup(_("Malformed server reply")), "xosd");
			else
				signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Malformed server reply")), "main-gui");
		}
		g_free(recv_buff);
		return NULL;
	}

	/* version returned by server */
	reply = g_strndup(buf + 11, strlen(buf) - 11 - strlen(buf1));

	print_debug("%s : Server returned version ,,%s''\n", GGadu_PLUGIN_NAME, reply);

	g_free(recv_buff);
	return reply;
}

/* this is kludgy, because we want to:
 *  - "2.0rc1" be newer than "2.0pre1"
 *  - "2.0" be newer than "2.0pre1"
 *  - "2.0" be newer than "2.0rc1"
 *  - "2.1" be newer than "2.0"
 */
gint update_compare(const gchar * servers, const gchar * ours)
{
	int slen, olen;
	gint i, ret = 0;
	gchar *buf = NULL;

	if (!servers || !ours)
		return 0;

	slen = strlen(servers);
	olen = strlen(ours);

	print_debug("strlen(server)=%d, strlen(ours)=%d\n", slen, olen);

	/* same lengths, just compare them */
	if (slen == olen)
	{
		print_debug("calling ggadu_strcasecmp(%s, %s);\n", servers, ours);
		return ggadu_strcasecmp(servers, ours);
	}

	/* servers is longer than ours */
	if (slen > olen)
	{
		buf = g_strnfill(slen, 'z');
		for (i = 0; i < olen; i++)
			buf[i] = ours[i];
		ret = ggadu_strcasecmp(servers, buf);
		print_debug("ggadu_stracasecmp(%s, %s) returns %d\n", servers, buf, ret);
	}
	else
	{
		buf = g_strnfill(olen, 'z');
		for (i = 0; i < slen; i++)
			buf[i] = servers[i];
		ret = ggadu_strcasecmp(buf, ours);
		print_debug("ggadu_stracasecmp(%s, %s) returns %d\n", buf, ours, ret);
	}

	g_free(buf);
	return ret;
}

/* compares. show == FALSE will disable any gui/xosd notices
 * returns -1 on error, 0 if no updates found, 1 if update found
 */
gpointer update_check_real(gboolean show)
{
	gchar *reply = update_get_current_version(show);
	gchar *version;
	gint i;

	if (reply == NULL)
		return NULL;

	version = g_strdup(VERSION);
	/* hack for _CVS */
	for (i = 0; i < strlen(version); i++)
		if (version[i] == '_')
			version[i] = 'z';

	/* compare, notice if something new was found */
	if (update_compare(reply, version) > 0)
	{
		if (update_use_xosd())
			signal_emit_from_thread(GGadu_PLUGIN_NAME, "xosd show message", g_strdup_printf(_("Update available: %s"), reply), "xosd");
		else
			signal_emit_from_thread(GGadu_PLUGIN_NAME, "gui show message", g_strdup_printf(_("Update available: %s"), reply), "main-gui");
	}
	else if (show)
	{
		if (update_use_xosd())
			signal_emit_from_thread(GGadu_PLUGIN_NAME, "xosd show message", g_strdup(_("No updates available")), "xosd");
		else
			signal_emit_from_thread(GGadu_PLUGIN_NAME, "gui show message", g_strdup(_("No updates available")), "main-gui");
	}

	g_free(reply);
	g_free(version);

	g_thread_exit(NULL);
	return NULL;
}

void _update_check_real(gboolean show)
{
	g_thread_create((gpointer(*)())update_check_real, (gpointer) show, FALSE, NULL);
}

/* for g_timeout_add */
gboolean update_check(gpointer data)
{
	/* don't notify about 'no updates found' unless we're using xosd */
	_update_check_real(update_use_xosd());

	/* never ending story ;) */
	return TRUE;
}

/* called with menu-click */
gpointer update_menu_check(gpointer data)
{
	_update_check_real(TRUE);
	return NULL;
}

gboolean update_check_on_startup(gpointer data)
{
	_update_check_real(TRUE);

	/* it's called ONCE. */
	return FALSE;
}

gpointer update_preferences(gpointer user_data)
{
	GGaduDialog *d;

	print_debug("%s : Preferences\n", GGadu_PLUGIN_NAME);

	d = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("Update Preferences"), "update config");

	/* *INDENT-OFF* */
	ggadu_dialog_add_entry(d, GGADU_UPDATE_CONFIG_CHECK_ON_STARTUP, _("Check for updates on startup"), VAR_BOOL, (gpointer) ggadu_config_var_get(update_handler, "check_on_startup"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(d, GGADU_UPDATE_CONFIG_CHECK_AUTOMATICALLY, _("Check for updates automatically"), VAR_BOOL, (gpointer) ggadu_config_var_get(update_handler, "check_automatically"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(d, GGADU_UPDATE_CONFIG_CHECK_INTERVAL, _("Check interval (minutes)"), VAR_INT, (gpointer) ggadu_config_var_get(update_handler, "check_interval"), VAR_FLAG_NONE);
	if (find_plugin_by_name("xosd"))
		ggadu_dialog_add_entry(d, GGADU_UPDATE_CONFIG_USE_XOSD, _("Use XOSD instead of dialog boxes"), VAR_BOOL, (gpointer) ggadu_config_var_get(update_handler, "use_xosd"), VAR_FLAG_NONE);
	/* *INDENT-ON* */

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");

	return NULL;
}

GGaduMenu *update_menu()
{
	GGaduMenu *root = ggadu_menu_create();
	GGaduMenu *item = ggadu_menu_add_item(root, "Update", NULL, NULL);

	ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Preferences"), update_preferences, NULL));
	ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Check for updates"), update_menu_check, NULL));

	return root;
}

void signal_receive(gpointer name, gpointer signal_ptr)
{
	GGaduSignal *signal = (GGaduSignal *) signal_ptr;

	print_debug("%s : received signal %d\n", GGadu_PLUGIN_NAME, signal->name);

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
				case GGADU_UPDATE_CONFIG_CHECK_ON_STARTUP:
					print_debug("change var check_on_startup to %d\n", kv->value);
					ggadu_config_var_set(update_handler, "check_on_startup", kv->value);
					break;
				case GGADU_UPDATE_CONFIG_CHECK_AUTOMATICALLY:
					print_debug("change var check_automatically to %d\n", kv->value);
					ggadu_config_var_set(update_handler, "check_automatically", kv->value);
					break;
				case GGADU_UPDATE_CONFIG_CHECK_INTERVAL:
					print_debug("change var check_interval to %d\n", kv->value);
					ggadu_config_var_set(update_handler, "check_interval", kv->value);
					break;
				case GGADU_UPDATE_CONFIG_USE_XOSD:
					print_debug("change var use_xosd to %d\n", kv->value);
					ggadu_config_var_set(update_handler, "use_xosd", kv->value);
					break;
				}
				tmplist = tmplist->next;
			}
			ggadu_config_save(update_handler);

			/* re-do timers */
			if (timer != -1)
				g_source_remove(timer);

			if ((gint) ggadu_config_var_get(update_handler, "check_automatically"))
			{
				timer = g_timeout_add(update_get_interval(), update_check, NULL);
				print_debug("%s : Timer ID set to %d\n", GGadu_PLUGIN_NAME, timer);
			}
			else
				timer = -1;
		}
		
		GGaduDialog_free(dialog);
		return;
	}

	if (signal->name == g_quark_from_static_string("get current version"))
	{
		signal->data_return = (gpointer) update_get_current_version(0);
		return;
	}
}

GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
	gchar *this_configdir = NULL;
	gchar *path = NULL;
	GGadu_PLUGIN_ACTIVATE(conf_ptr);

	print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);
	update_handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("Update checker"));

	print_debug("%s : read configuration\n", GGadu_PLUGIN_NAME);
	if (g_getenv("HOME_ETC"))
		this_configdir = g_build_filename(g_getenv("HOME_ETC"), "gg2", NULL);
	else
		this_configdir = g_build_filename(g_get_home_dir(), ".gg2", NULL);

	path = g_build_filename(this_configdir, "update", NULL);
	ggadu_config_set_filename((GGaduPlugin *) update_handler, path);
	g_free(path);
	g_free(this_configdir);

	ggadu_config_var_add(update_handler, "check_on_startup", VAR_BOOL);
	ggadu_config_var_add(update_handler, "check_automatically", VAR_BOOL);
	ggadu_config_var_add(update_handler, "check_interval", VAR_INT);
	ggadu_config_var_add(update_handler, "use_xosd", VAR_BOOL);

	if (!ggadu_config_read(update_handler))
		g_warning(_("Unable to read config file for plugin update"));

	register_signal_receiver((GGaduPlugin *) update_handler, (signal_func_ptr) signal_receive);

	return update_handler;
}

void start_plugin()
{
	print_debug("%s : start_plugin\n", GGadu_PLUGIN_NAME);
	register_signal(update_handler, "update config");
	register_signal(update_handler, "get current version");

	print_debug("%s : create menu\n", GGadu_PLUGIN_NAME);
	menu_updatemenu = update_menu();
	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_updatemenu, "main-gui");

	if (timer != -1)
		g_source_remove(timer);

	/* initialize timers */
	if ((gint) ggadu_config_var_get(update_handler, "check_automatically"))
	{
		timer = g_timeout_add(update_get_interval(), update_check, NULL);
		print_debug("%s : Timer ID set to %d\n", GGadu_PLUGIN_NAME, timer);
	}
	else
	{
		print_debug("%s : Resetting timer!\n", GGadu_PLUGIN_NAME);
		timer = -1;
	}

	if ((gint) ggadu_config_var_get(update_handler, "check_on_startup"))
		/* wait a while before looking for updates */
		g_timeout_add(3000, update_check_on_startup, NULL);
}

void destroy_plugin()
{
	print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);

	if (timer != -1)
		g_source_remove(timer);

	if (menu_updatemenu)
	{
		signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", menu_updatemenu, "main-gui");
		ggadu_menu_free(menu_updatemenu);
	}
}
