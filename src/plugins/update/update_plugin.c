/* $Id: update_plugin.c,v 1.3 2003/06/01 00:19:28 shaster Exp $ */
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

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "menu.h"
#include "support.h"
#include "dialog.h"
#include "update_plugin.h"

GGaduPlugin *update_handler;

GGaduMenu *menu_updatemenu;
guint timer = -1;

GGadu_PLUGIN_INIT("update", GGADU_PLUGIN_TYPE_MISC);

/* helper */
int update_get_interval(void)
{
    return (((gint) config_var_get(update_handler, "check_interval") >
	     0 ? (gint) config_var_get(update_handler, "check_interval") * 60 : 3600) * 1000);
}

/* helper */
int update_use_xosd(void)
{
    return (((gboolean) config_var_get(update_handler, "use_xosd") && find_plugin_by_name("xosd")) ? 1 : 0);
}

/* connects, parses.
 * returns pointer to version string
 * HEAVILY RELIES ON SF.NET's RSS LAYOUT.
 */
gchar *update_get_current_version(int show)
{
    int sock_r, sock_s, i = 0;
    struct hostent *h;
    struct sockaddr_in servAddr;
    const gchar *server = GGADU_UPDATE_SERVER;
    gchar *get = NULL, *recv_buff = NULL, *temp = NULL, *buf1 = NULL;
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
		signal_emit(GGadu_PLUGIN_NAME, "xosd show message", g_strdup_printf(_("Unknown host: %s"), server),
			    "xosd");
	    else
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup_printf(_("Unknown host: %s"), server),
			    "main-gui");
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
		signal_emit(GGadu_PLUGIN_NAME, "xosd show message",
			    g_strdup_printf(_("Error while connecting to %s"), server), "xosd");
	    else
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning",
			    g_strdup_printf(_("Error while connecting to %s"), server), "main-gui");
	}
	return NULL;
    }

    /* the GET request */
    get =
	g_strdup_printf("GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n", GGADU_UPDATE_URL, server,
			GGADU_UPDATE_USERAGENT_STRING);
    send(sock_s, get, strlen(get), MSG_WAITALL);
    g_free(get);

    /* allocate some buffers */
    recv_buff = g_malloc0(8000);
    temp = g_malloc0(2);

    if (!recv_buff || !temp)
    {
	close(sock_s);
	return NULL;
    }

    /* receiving data */
    while (recv(sock_s, temp, 1, MSG_WAITALL))
	recv_buff[i++] = temp[0];

    /* close socket, free some memory */
    close(sock_s);
    g_free(temp);

    /* check for "200 OK" response */
    if ((recv_buff = g_strstr_len(recv_buff, i, "200 OK")) == NULL)
    {
	if (show)
	{
	    if (update_use_xosd())
		signal_emit(GGadu_PLUGIN_NAME, "xosd show message",
			    g_strdup(_("Server-side error during update check")), "xosd");
	    else
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Server-side error during update check")),
			    "main-gui");
	}
	return NULL;
    }

    /* Search for: <title>gg2 XXXXXX released (XXXXXXXXXXXXXXXXXXXXXXX)</title> */
    if ((recv_buff = g_strstr_len(recv_buff, strlen(recv_buff), "<title>gg2")) == NULL)
    {
	if (show)
	{
	    if (update_use_xosd())
		signal_emit(GGadu_PLUGIN_NAME, "xosd show message", g_strdup(_("Malformed server reply")), "xosd");
	    else
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Malformed server reply")), "main-gui");
	}
	return NULL;
    }

    buf1 = g_malloc0(8000);
    buf1 = g_strstr_len(recv_buff + 11, strlen(recv_buff) - 11, " released");
    if (!buf1)
    {
	if (show)
	{
	    if (update_use_xosd())
		signal_emit(GGadu_PLUGIN_NAME, "xosd show message", g_strdup(_("Malformed server reply")), "xosd");
	    else
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Malformed server reply")), "main-gui");
	}
	return NULL;
    }

    /* version returned by server */
    reply = g_strndup(recv_buff + 11, strlen(recv_buff) - 11 - strlen(buf1));

    print_debug("%s : Server returned version ,,%s''\n", GGadu_PLUGIN_NAME, reply);

    if (reply && strlen(reply) > 0)
	return reply;
    else
	return NULL;
}

/* compares. show == 0 will disable any gui/xosd notices
 * returns -1 on error, 0 if no updates found, 1 if update found
 */
int update_check_real(int show)
{
    gchar *reply = update_get_current_version(show);
    gchar *version = NULL;
    int i, result = 0;

    if (reply == NULL)
	return -1;

    version = g_strdup(VERSION);
    /* hack for _CVS */
    for (i = 0; i < strlen(version); i++)
	if (version[i] == '_')
	    version[i] = 'z';

    /* compare, notice if something new was found */
    if (ggadu_strcasecmp(reply, version) > 0)
    {
	if (show)
	{
	    if (update_use_xosd())
		signal_emit(GGadu_PLUGIN_NAME, "xosd show message",
			    g_strdup_printf(_("Update available: %s"), (reply ? reply : _("[error]"))), "xosd");
	    else
		signal_emit(GGadu_PLUGIN_NAME, "gui show message",
			    g_strdup_printf(_("Update available: %s"), (reply ? reply : _("[error]"))), "main-gui");
	}
	result = 1;
    }

    g_free(reply);
    g_free(version);

    return result;
}

/* for g_timeout_add */
gboolean update_check(gpointer data)
{
    update_check_real(1);

    /* never ending story ;) */
    return TRUE;
}

/* called with menu-click */
gpointer update_menu_check(gpointer data)
{
    if (update_check_real(1) == 0)
    {
	if (update_use_xosd())
	    signal_emit(GGadu_PLUGIN_NAME, "xosd show message", g_strdup(_("No updates available")), "xosd");
	else
	    signal_emit(GGadu_PLUGIN_NAME, "gui show message", g_strdup(_("No updates available")), "main-gui");
    }

    return NULL;
}

gboolean update_check_on_startup(gpointer data)
{
    update_check_real(1);

    /* it's called ONCE. */
    return FALSE;
}

gpointer update_preferences(gpointer user_data)
{
    GGaduDialog *d;

    print_debug("%s : Preferences\n", GGadu_PLUGIN_NAME);

    d = ggadu_dialog_new();
    ggadu_dialog_set_title(d, _("Update Preferences"));
    ggadu_dialog_callback_signal(d, "update config");
    ggadu_dialog_set_type(d, GGADU_DIALOG_CONFIG);

    /* *INDENT-OFF* */
    ggadu_dialog_add_entry(&(d->optlist), GGADU_UPDATE_CONFIG_CHECK_ON_STARTUP, _("Check for updates on startup"), VAR_BOOL, (gpointer) config_var_get(update_handler, "check_on_startup"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_UPDATE_CONFIG_CHECK_AUTOMATICALLY, _("Check for updates automatically"), VAR_BOOL, (gpointer) config_var_get(update_handler, "check_automatically"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_UPDATE_CONFIG_CHECK_INTERVAL, _("Check interval (minutes)"), VAR_INT, (gpointer) config_var_get(update_handler, "check_interval"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_UPDATE_CONFIG_USE_XOSD, _("Use XOSD instead of dialog boxes"), VAR_BOOL, (gpointer) config_var_get(update_handler, "use_xosd"), VAR_FLAG_NONE);
    /* *INDENT-ON* */

    signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");

    return NULL;
}

GGaduMenu *update_menu()
{
    GGaduMenu *root = ggadu_menu_create();
    GGaduMenu *item = ggadu_menu_add_item(root, "Update", NULL, NULL);

    ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Check for updates"), update_menu_check, NULL));
    ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Preferences"), update_preferences, NULL));

    return root;
}

void signal_receive(gpointer name, gpointer signal_ptr)
{
    GGaduSignal *signal = (GGaduSignal *) signal_ptr;

    print_debug("%s : received signal %s\n", GGadu_PLUGIN_NAME, (gchar *) signal->name);

    if (!ggadu_strcasecmp(signal->name, "update config"))
    {
	GGaduDialog *d = signal->data;
	GSList *tmplist = d->optlist;

	if (d->response == GGADU_OK)
	{
	    while (tmplist)
	    {
		GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
		switch (kv->key)
		{
		case GGADU_UPDATE_CONFIG_CHECK_ON_STARTUP:
		    print_debug("change var check_on_startup to %d\n", kv->value);
		    config_var_set(update_handler, "check_on_startup", kv->value);
		    break;
		case GGADU_UPDATE_CONFIG_CHECK_AUTOMATICALLY:
		    print_debug("change var check_automatically to %d\n", kv->value);
		    config_var_set(update_handler, "check_automatically", kv->value);
		    break;
		case GGADU_UPDATE_CONFIG_CHECK_INTERVAL:
		    print_debug("change var check_interval to %d\n", kv->value);
		    config_var_set(update_handler, "check_interval", kv->value);
		    break;
		case GGADU_UPDATE_CONFIG_USE_XOSD:
		    print_debug("change var use_xosd to %d\n", kv->value);
		    config_var_set(update_handler, "use_xosd", kv->value);
		    break;
		}
		tmplist = tmplist->next;
	    }
	    config_save(update_handler);

	    /* re-do timers */
	    if (timer != -1)
		g_source_remove(timer);

	    if ((gint) config_var_get(update_handler, "check_automatically"))
	    {
		timer = g_timeout_add(update_get_interval(), update_check, NULL);
		print_debug("%s : Timer ID set to %d\n", GGadu_PLUGIN_NAME, timer);
	    }
	    else
		timer = -1;
	}
	GGaduDialog_free(d);

	return;
    }

    if (!ggadu_strcasecmp(signal->name, "get current version"))
    {
	gchar *tmp = update_get_current_version(0);
	signal->data_return = (gpointer) g_strdup(tmp);
	g_free(tmp);

	return;
    }
}

GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
    gchar *this_configdir = NULL;
    print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);

    GGadu_PLUGIN_ACTIVATE(conf_ptr);
    update_handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("Update checker"));

    print_debug("%s : read configuration\n", GGadu_PLUGIN_NAME);
    if (g_getenv("CONFIG_DIR"))
	this_configdir = g_build_filename(g_get_home_dir(), g_getenv("CONFIG_DIR"), "gg2", NULL);
    else
	this_configdir = g_build_filename(g_get_home_dir(), ".gg2", NULL);

    set_config_file_name((GGaduPlugin *) update_handler, g_build_filename(this_configdir, "update", NULL));

    g_free(this_configdir);

    config_var_add(update_handler, "check_on_startup", VAR_BOOL);
    config_var_add(update_handler, "check_automatically", VAR_BOOL);
    config_var_add(update_handler, "check_interval", VAR_INT);
    config_var_add(update_handler, "use_xosd", VAR_BOOL);

    if (!config_read(update_handler))
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
    if ((gint) config_var_get(update_handler, "check_automatically"))
    {
	timer = g_timeout_add(update_get_interval(), update_check, NULL);
	print_debug("%s : Timer ID set to %d\n", GGadu_PLUGIN_NAME, timer);
    }
    else
    {
	print_debug("%s : Resetting timer!\n", GGadu_PLUGIN_NAME);
	timer = -1;
    }

    if ((gint) config_var_get(update_handler, "check_on_startup"))
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
