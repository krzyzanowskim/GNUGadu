/* $Id: gadu_gadu_plugin.c,v 1.205 2004/11/03 12:42:45 krzyzak Exp $ */

/* 
 * Gadu-Gadu plugin for GNU Gadu 2 
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <libgadu.h>

#include "ggadu_types.h"
#include "plugins.h"
#include "ggadu_conf.h"
#include "signals.h"
#include "ggadu_support.h"
#include "ggadu_menu.h"
#include "gadu_gadu_plugin.h"
#include "ggadu_dialog.h"
#include "ggadu_repo.h"
#include "perl_embed.h"

GGaduPlugin *handler;

static struct gg_session *session;
static struct gg_dcc *dcc_session_get = NULL;

static gchar *dcc_send_request_filename = NULL;	/* nazwa pliku do wyslania */

static gint connect_count = 0;

static guint watch = 0;
static guint watch_dcc_file = 0;

static GIOChannel *source_chan = NULL;

static gboolean connected = FALSE;

static GGaduProtocol *p;
static GGaduMenu *menu_pluginmenu;

static gchar *this_configdir = NULL;

static GStaticMutex register_mutex = G_STATIC_MUTEX_INIT;

GGadu_PLUGIN_INIT("gadu-gadu", GGADU_PLUGIN_TYPE_PROTOCOL);


/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 						PRIVATE STUFF
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
void test()
{
}

gint ggadu_is_status_descriptive(gint status)
{
	return (gint) (status == GG_STATUS_AVAIL_DESCR || status == GG_STATUS_BUSY_DESCR || status == GG_STATUS_NOT_AVAIL_DESCR || status == GG_STATUS_INVISIBLE_DESCR);
}

void ggadu_gadu_gadu_disconnect()
{
	GSList *tmplist = NULL;

	if (watch > 0)
		g_source_remove(watch);

	if (source_chan)
		g_io_channel_unref(source_chan);

	gg_logoff(session);
	gg_free_session(session);
	session = NULL;
	connected = FALSE;

	tmplist = ggadu_repo_get_as_slist("gadu-gadu", REPO_VALUE_CONTACT);
	while (tmplist)
	{
		GGaduContact *k = tmplist->data;
		if (k->status != GG_STATUS_NOT_AVAIL)
		{
			k->status = GG_STATUS_NOT_AVAIL;
			g_free(k->status_descr);
			g_free(k->ip);
			k->status_descr = NULL;
			k->ip = NULL;
			ggadu_repo_change_value("gadu-gadu", ggadu_repo_key_from_string(k->id), k, REPO_VALUE_DC);
		}
		tmplist = tmplist->next;
	}
	signal_emit(GGadu_PLUGIN_NAME, "gui disconnected", NULL, "main-gui");
}

void ggadu_gadu_gadu_disconnect_msg(gchar * txt)
{
	ggadu_gadu_gadu_disconnect();
	print_debug("disconnect : %s", txt);
	signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup((txt) ? txt : _("Connection failed!")), "main-gui");
}

gchar *insert_cr(gchar * txt)
{
	gchar *in = txt;
	gchar *out = g_malloc0(strlen(txt) * 2);
	gchar *start = out;

	while (*in)
	{
		if (*in == '\n')
		{
			*out = '\r';
			out++;
		}
		*out = *in;
		in++;
		out++;
	}
	start = g_try_realloc(start, strlen(start) + 1);
	return start;
}

void gadu_gadu_enable_dcc_socket(gboolean state)
{
	if ((state == TRUE) && (!dcc_session_get) && ((gboolean) ggadu_config_var_get(handler, "dcc")))
	{
		GIOChannel *dcc_channel_get = NULL;
		gint cond = G_IO_IN;

		dcc_session_get = gg_dcc_socket_create((int) ggadu_config_var_get(handler, "uin"), 0);
		gg_dcc_ip = inet_addr("255.255.255.255");
		gg_dcc_port = dcc_session_get->port;

		/* ZONK */
		dcc_channel_get = g_io_channel_unix_new(dcc_session_get->fd);
		cond = (dcc_session_get->check == GG_CHECK_READ) ? G_IO_IN : G_IO_OUT;
		watch_dcc_file = g_io_add_watch(dcc_channel_get, G_IO_ERR | cond, test_chan_dcc_get, dcc_session_get);

	}
	else if (state == FALSE)
	{
		if (watch_dcc_file)
			g_source_remove(watch_dcc_file);

		gg_free_dcc(dcc_session_get);
		dcc_session_get = NULL;
		gg_dcc_ip = 0;
		gg_dcc_port = 0;
	}
}

static gpointer gadu_gadu_login(gpointer desc, gint status)
{
	struct gg_login_params p;
	gchar *server_addr_config = (gchar *) ggadu_config_var_get(handler, "server");
	gchar *server_addr_ip = NULL;
	gchar **server_addr_split = NULL;

	if (connected)
	{
		gg_logoff(session);
		gg_free_session(session);
		session = NULL;
		connected = FALSE;
		return FALSE;
	}

	gadu_gadu_enable_dcc_socket(TRUE);

	memset(&p, 0, sizeof (p));
	p.server_port = GG_DEFAULT_PORT;
	p.uin = (int) ggadu_config_var_get(handler, "uin");
	p.password = (gchar *) ggadu_config_var_get(handler, "password");
	p.async = 1;
	p.status_descr = desc;
	p.status = status;

	if ((gint) ggadu_config_var_get(handler, "private") == 1)
		p.status |= GG_STATUS_FRIENDS_MASK;

	/* set server ip and port number */
	if (server_addr_config)
	{
		if ((server_addr_split = g_strsplit(server_addr_config, ":", 2)))
		{
			if (server_addr_split[0])
				server_addr_ip = g_strdup(server_addr_split[0]);
			if (server_addr_split[1])
				p.server_port = g_strtod(server_addr_split[1], NULL);
			g_strfreev(server_addr_split);
		}
	}

	if (server_addr_ip)
	{
		p.server_addr = inet_addr(server_addr_ip);
		print_debug("server : %s %d", server_addr_ip, p.server_port);
		g_free(server_addr_ip);
	}

	/* proxy setting taken from EKG project */
	if (ggadu_config_var_check(handler, "proxy"))
	{
		char **auth = array_make((gchar *) ggadu_config_var_get(handler, "proxy"), "@", 0, 0, 0);
		char **proxy_userpass = NULL;
		char **proxy_hostport = NULL;

		gg_proxy_enabled = 1;

		if (auth[0] && auth[1])
		{
			proxy_userpass = array_make(auth[0], ":", 0, 0, 0);
			proxy_hostport = array_make(auth[1], ":", 0, 0, 0);
		}
		else
		{
			proxy_hostport = array_make(auth[0], ":", 0, 0, 0);
		}

		gg_proxy_host = g_strdup(proxy_hostport[0]);
		gg_proxy_port = (proxy_hostport[1]) ? atoi(proxy_hostport[1]) : 8080;

		array_free(proxy_hostport);
		array_free(proxy_userpass);
		array_free(auth);

		print_debug("proxy : %s %d", gg_proxy_host, gg_proxy_port);
	}

	if (!p.uin || (!p.password || !*p.password))
	{
		user_preferences_action(NULL);
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("You have to enter your GG# and password first!")), "main-gui");
		ggadu_gadu_gadu_disconnect();
		return NULL;
	}

	print_debug("Trying login as %d", (gint) ggadu_config_var_get(handler, "uin"));

	if (!(session = gg_login(&p)))
	{
		ggadu_gadu_gadu_disconnect_msg(NULL);
		return NULL;
	}

	source_chan = g_io_channel_unix_new(session->fd);
	watch = g_io_add_watch(source_chan, G_IO_IN | G_IO_ERR | G_IO_HUP, test_chan, NULL);

	return NULL;
}

static gboolean gadu_gadu_ping(gpointer data)
{
	if (!connected)
		return FALSE;

	gg_ping(session);
	print_debug("PING! send!\n");
	return TRUE;
}

static void handle_search_event(struct gg_event *e)
{
	gg_pubdir50_t res = e->event.pubdir50;
	gint count, i;
	GSList *list = NULL;
	GDate *cur_date;

	cur_date = g_date_new();
	g_date_set_time(cur_date, time(NULL));

	if ((count = gg_pubdir50_count(res)) < 1)
	{
		signal_emit(GGadu_PLUGIN_NAME, "gui show message", g_strdup(_("No users have been found!")), "main-gui");
		return;
	}

	for (i = 0; i < count; i++)
	{
		GGaduContact *k = g_new0(GGaduContact, 1);

		const gchar *uin = gg_pubdir50_get(res, i, GG_PUBDIR50_UIN);
		const gchar *first_name = gg_pubdir50_get(res, i, GG_PUBDIR50_FIRSTNAME);
		const gchar *nick = gg_pubdir50_get(res, i, GG_PUBDIR50_NICKNAME);
		const gchar *status = gg_pubdir50_get(res, i, GG_PUBDIR50_STATUS);
		const gchar *city = gg_pubdir50_get(res, i, GG_PUBDIR50_CITY);
		const gchar *birthyear = gg_pubdir50_get(res, i, GG_PUBDIR50_BIRTHYEAR);

		k->id = g_strdup((uin) ? uin : "?");

		if (birthyear != NULL)
		{
			gint b_year = ((cur_date->year) - atoi(birthyear));
			k->age = (b_year <= 99) ? g_strdup_printf("%d", b_year) : NULL;
		}
		else
		{
			k->age = NULL;
		}

		if (first_name)
			k->first_name = to_utf8("CP1250", (gchar *) first_name);

		if (nick)
			k->nick = to_utf8("CP1250", (gchar *) nick);

		if (city)
			k->city = to_utf8("CP1250", (gchar *) city);

		k->status = (status) ? atoi(status) : GG_STATUS_NOT_AVAIL;
		list = g_slist_append(list, k);
	}
	signal_emit(GGadu_PLUGIN_NAME, "gui show search results", list, "main-gui");
}

void ggadu_gadu_gadu_reconnect()
{
	if (++connect_count < 3)
	{
		gint _status = session->status;
		ggadu_gadu_gadu_disconnect();
		gadu_gadu_login(NULL, _status);
	}
	else
	{
		gchar *txt = g_strdup(_("Disconnected"));
		ggadu_gadu_gadu_disconnect_msg(txt);
		connect_count = 0;
		g_free(txt);
	}
}

gboolean test_chan(GIOChannel * source, GIOCondition condition, gpointer data)
{
	struct gg_event *e = NULL;
	static gint prev_check = GG_CHECK_READ;
	GSList *slistmp = NULL;
	uint32_t *uins;
	GGaduMsg *msg = NULL;
	gint i, j;

	/* w przypadku bledu/utraty polaczenia postap tak jak w przypadku disconnect */
	if (!session || !(e = gg_watch_fd(session)) || (condition & G_IO_ERR) ||
	    ((condition & G_IO_HUP) && ((session->state != GG_STATE_CONNECTING_GG) && (session->check != GG_CHECK_WRITE))))
	{
		ggadu_gadu_gadu_reconnect();
		return FALSE;
	}

	switch (e->type)
	{
	case GG_EVENT_NONE:
		print_debug("GG_EVENT_NONE");
		break;

	case GG_EVENT_PONG:
		print_debug("GG_EVENT_PONG");
		gg_ping(session);
		break;

	case GG_EVENT_CONN_SUCCESS:
		{
			GSList *list = ggadu_repo_get_as_slist("gadu-gadu", REPO_VALUE_CONTACT);
			print_debug("GG_EVENT_CONN_SUCCESS");

			connected = TRUE;
			i = g_slist_length(list);
			j = 0;

			/* notify wysylam */
			uins = g_malloc0(i * sizeof (uint32_t));
			slistmp = list;
			while (slistmp)
			{
				GGaduContact *k = slistmp->data;
				uins[j++] = atoi(k->id);
				slistmp = slistmp->next;
			}
			gg_notify(session, uins, i);
			g_free(uins);

			/* pingpong */
			g_timeout_add(100000, gadu_gadu_ping, NULL);

			if (ggadu_config_var_get(handler, "sound_app_file") != NULL)
				signal_emit(GGadu_PLUGIN_NAME, "sound play file", ggadu_config_var_get(handler, "sound_app_file"), "sound*");

			/* *INDENT-OFF* */
			signal_emit(GGadu_PLUGIN_NAME, "gui status changed",
					(gpointer) ((session->status & GG_STATUS_FRIENDS_MASK) ? (session->status ^ GG_STATUS_FRIENDS_MASK) : session->status), "main-gui");
			/* *INDENT-ON* */

			ggadu_config_var_set(handler, "server", g_strdup_printf("%s:%d", inet_ntoa(*((struct in_addr *) &session->server_addr)), session->port));

			ggadu_config_save(handler);
			g_slist_free(list);
		}
		break;
	case GG_EVENT_CONN_FAILED:
		{
			ggadu_config_var_set(handler, "server", NULL);
			ggadu_gadu_gadu_reconnect();
		}
		break;
	case GG_EVENT_DISCONNECT:
		{
			ggadu_gadu_gadu_reconnect();
		}
		break;
	case GG_EVENT_USERLIST:

		switch (e->event.userlist.type)
		case GG_USERLIST_GET_REPLY:
			{
				gchar *list = NULL;

				print_debug("GG_USERLIST_GET_REPLY");

				list = to_utf8("CP1250", e->event.userlist.reply);
				if (import_userlist(list))
				{
					signal_emit(GGadu_PLUGIN_NAME, "gui show message", g_strdup(_("Userlist has been imported succesfully!")), "main-gui");
				}
				g_free(list);
			}

		break;
	case GG_EVENT_MSG:

		print_debug("GG_EVENT_MSG from: %d type: %d", e->event.msg.sender, e->event.msg.msgclass);

		if ((e->event.msg.msgclass & GG_CLASS_CTCP))
		{		/* dcc part */
/*			GGaduDialog *dialog = NULL;
			gchar *tmp = NULL;
			
			print_debug("somebody want to send you a file");
			tmp = g_strdup_printf(_("%d sending you a  file"), e->event.msg.sender);
			dialog = ggadu_dialog_new_full(GGADU_DIALOG_GENERIC, tmp, "get file", (gpointer) e->event.msg.sender);

			signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
*/
			/* ZONK */

			break;
		}
		/* end of dcc part */
		msg = g_new0(GGaduMsg, 1);
		msg->id = g_strdup_printf("%d", e->event.msg.sender);
		msg->message = to_utf8("CP1250", e->event.msg.message);
		msg->class = e->event.msg.msgclass;
		msg->time = e->event.msg.time;

		if (e->event.msg.recipients_count > 0)
		{
			gint i = 0;
			uin_t *recipients_gg = e->event.msg.recipients;

			print_debug("CONFERENCE MESSAGE %d\n", e->event.msg.recipients_count);

			msg->class = GGADU_CLASS_CONFERENCE;
			msg->recipients = g_slist_append(msg->recipients, (gpointer) g_strdup_printf("%d", e->event.msg.sender));

			for (i = 0; i < e->event.msg.recipients_count; i++)
				msg->recipients = g_slist_append(msg->recipients, (gpointer) g_strdup_printf("%d", recipients_gg[i]));

		}

		GSList *list = ggadu_repo_get_as_slist("gadu-gadu", REPO_VALUE_CONTACT);
		GSList *us = list;
		gchar *line2 = NULL;

		while (us)
		{
			GGaduContact *k = (GGaduContact *) us->data;
			if (strcmp(k->id, msg->id) == 0)
				line2 = g_strdup_printf("%s", k->nick);

			us = us->next;
		}

		ggadu_gg_save_history(GGADU_HISTORY_TYPE_RECEIVE, msg, line2);
		g_free(line2);
		g_slist_free(list);

		signal_emit_full(GGadu_PLUGIN_NAME, "gui msg receive", msg, "main-gui", GGaduMsg_free);

		if (ggadu_config_var_get(handler, "sound_msg_file") != NULL)
			signal_emit(GGadu_PLUGIN_NAME, "sound play file", ggadu_config_var_get(handler, "sound_msg_file"), "sound*");

		break;

	case GG_EVENT_NOTIFY60:
		{
			print_debug("GG_EVENT_NOTIFY60");

			for (i = 0; e->event.notify60[i].uin; i++)
			{
				gchar *strIP = NULL;
				gchar *id = g_strdup_printf("%d", e->event.notify60[i].uin);
				struct in_addr ip_addr;
				GGaduContact *k = ggadu_repo_find_value("gadu-gadu", ggadu_repo_key_from_string(id));

				if (k)
				{
					ip_addr.s_addr = e->event.notify60[i].remote_ip;
					strIP = inet_ntoa(ip_addr);	/* cannot be freed it's staically allocated memory */

					if (strIP && (ggadu_strcasecmp(strIP, "0.0.0.0")))
					{
						g_free(k->ip);
						k->ip = g_strdup_printf("%s:%d", strIP, e->event.notify60[i].remote_port);
					}

					k->status = e->event.notify60[i].status;
					g_free(k->status_descr);
					k->status_descr = ggadu_convert("CP1250", "UTF-8", e->event.notify60[i].descr);

					ggadu_repo_change_value("gadu-gadu", ggadu_repo_key_from_string(id), k, REPO_VALUE_DC);

					/* Zapiszmy sobie zmiane opisu do pliku historii */

					gchar *status = NULL;
					if ((k->status == GG_STATUS_AVAIL) || (k->status == GG_STATUS_AVAIL_DESCR))
						status = g_strdup_printf("avail");
					else if ((k->status == GG_STATUS_BUSY) || (k->status == GG_STATUS_BUSY_DESCR))
						status = g_strdup_printf("busy");
					else if ((k->status == GG_STATUS_INVISIBLE) || (k->status == GG_STATUS_INVISIBLE_DESCR))
						status = g_strdup_printf("notavail");
					else if ((k->status == GG_STATUS_NOT_AVAIL) || (k->status == GG_STATUS_NOT_AVAIL_DESCR))
						status = g_strdup_printf("notavail");
					else if (k->status == GG_STATUS_BLOCKED)
						status = g_strdup_printf("blocked");
					else
						status = g_strdup_printf("unknown");

					/* Format kompatybilny z histori± Kadu ;)                       */
					/* Jednak pierw nale¿y skasowaæ [numerek].idx aby uaktualniæ ;) */

/* DUPA					gchar *line = g_strdup_printf("status,%s,%s,%s,%d,%s%s\n", k->id, k->nick, 
							    ((k->ip == NULL) ? "0.0.0.0" : k->ip), (int)time(0), status,
							    ((k->status_descr == NULL) ? "" : g_strdup_printf(",%s",k->status_descr)));
					ggadu_gg_save_history((gchar *) k->id, line);
*/

//                                      g_free(line);
					g_free(status);
				}
				else
				{
					print_debug("contact that is no in out list send us notify ?????");
				}

				g_free(id);
			}
		}
		break;
	case GG_EVENT_NOTIFY:
	case GG_EVENT_NOTIFY_DESCR:
		{
			struct gg_notify_reply *n = NULL;

			print_debug("GG_EVENT_NOTIFY");

			if (e->type == GG_EVENT_NOTIFY)
				n = e->event.notify;
			else if (e->type == GG_EVENT_NOTIFY_DESCR)
				n = e->event.notify_descr.notify;

			while (n->uin)
			{
				gchar *id = g_strdup_printf("%d", n->uin);
				GGaduContact *k = ggadu_repo_find_value("gadu-gadu", ggadu_repo_key_from_string(id));
				struct in_addr ip_addr;

				if (k)
				{
					ip_addr.s_addr = n->remote_ip;
					if (inet_ntoa(ip_addr) && (ggadu_strcasecmp(inet_ntoa(ip_addr), "0.0.0.0")))
					{
						g_free(k->ip);
						k->ip = g_strdup_printf("%s:%d", inet_ntoa(ip_addr), n->remote_port);
					}

					if (e->type == GG_EVENT_NOTIFY_DESCR)
					{
						g_free(k->status_descr);
						k->status_descr = ggadu_convert("CP1250", "UTF-8", ggadu_strchomp(e->event.notify_descr.descr));
					}

					k->status = n->status;
					ggadu_repo_change_value("gadu-gadu", ggadu_repo_key_from_string(k->id), k, REPO_VALUE_DC);

					/* Zapiszmy sobie zmiane opisu do pliku historii */

					gchar *status = NULL;
					if ((k->status == GG_STATUS_AVAIL) || (k->status == GG_STATUS_AVAIL_DESCR))
						status = g_strdup_printf("avail");
					else if ((k->status == GG_STATUS_BUSY) || (k->status == GG_STATUS_BUSY_DESCR))
						status = g_strdup_printf("busy");
					else if ((k->status == GG_STATUS_INVISIBLE) || (k->status == GG_STATUS_INVISIBLE_DESCR))
						status = g_strdup_printf("notavail");
					else if ((k->status == GG_STATUS_NOT_AVAIL) || (k->status == GG_STATUS_NOT_AVAIL_DESCR))
						status = g_strdup_printf("notavail");
					else if (k->status == GG_STATUS_BLOCKED)
						status = g_strdup_printf("blocked");
					else
						status = g_strdup_printf("unknown");

					/* Format kompatybilny z histori± Kadu ;)                       */
					/* Jednak pierw nale¿y skasowaæ [numerek].idx aby uaktualniæ ;) */

/* DUPA					gchar *line = g_strdup_printf("status,%s,%s,%s,%d,%s%s\n", k->id, k->nick, 
							    ((k->ip == NULL) ? "0.0.0.0" : k->ip), (gint)time(0), status,
							    ((k->status_descr == NULL) ? "" : g_strdup_printf(",%s",k->status_descr)));
					ggadu_gg_save_history((gchar *) k->id, line);
*/
//                                      g_free(line);
					g_free(status);
				}
				g_free(id);
				n++;
			}
		}
		break;
	case GG_EVENT_STATUS60:
	case GG_EVENT_STATUS:
		{
			gchar *id = g_strdup_printf("%d",
						    (e->type == GG_EVENT_STATUS) ? e->event.status.uin : e->event.status60.uin);
			GGaduContact *k = ggadu_repo_find_value("gadu-gadu", ggadu_repo_key_from_string(id));
			gchar *tmp = NULL;

			print_debug("GG_EVENT_STATUS");
			
			if (!k) 
			{
			    g_free(id);
			    break;
			}
			/* *INDENT-OFF* */
			
			k->status = (e->type == GG_EVENT_STATUS) ? e->event.status.status : e->event.status60.status;
			
			g_free(k->status_descr);
			tmp = ggadu_convert("CP1250", "UTF-8",((e->type == GG_EVENT_STATUS) ? e->event.status.descr : e->event.status60.descr));

			k->status_descr = ggadu_strchomp(tmp);
			
			/* *INDENT-ON* */

			ggadu_repo_change_value("gadu-gadu", ggadu_repo_key_from_string(k->id), k, REPO_VALUE_DC);

			/* Zapiszmy sobie zmiane opisu do pliku historii */

			gchar *status = NULL;
			if ((k->status == GG_STATUS_AVAIL) || (k->status == GG_STATUS_AVAIL_DESCR))
				status = g_strdup_printf("avail");
			else if ((k->status == GG_STATUS_BUSY) || (k->status == GG_STATUS_BUSY_DESCR))
				status = g_strdup_printf("busy");
			else if ((k->status == GG_STATUS_INVISIBLE) || (k->status == GG_STATUS_INVISIBLE_DESCR))
				status = g_strdup_printf("notavail");
			else if ((k->status == GG_STATUS_NOT_AVAIL) || (k->status == GG_STATUS_NOT_AVAIL_DESCR))
				status = g_strdup_printf("notavail");
			else if (k->status == GG_STATUS_BLOCKED)
				status = g_strdup_printf("blocked");
			else
				status = g_strdup_printf("unknown");

			/* Format kompatybilny z histori± Kadu ;)                       */
			/* Jednak pierw nale¿y skasowaæ [numerek].idx aby uaktualniæ ;) */

/* DUPA			gchar *line = g_strdup_printf("status,%s,%s,%s,%d,%s%s\n", k->id, k->nick, 
				    ((k->ip == NULL) ? "0.0.0.0" : k->ip), (gint)time(0), status,
				    ((k->status_descr == NULL) ? "" : g_strdup_printf(",%s",k->status_descr)));
			ggadu_gg_save_history((gchar *) k->id, line);
*/
//                      g_free(line);
			g_free(status);

			g_free(id);
		}
		break;

	case GG_EVENT_ACK:
		if (e->event.ack.status == GG_ACK_QUEUED)
			print_debug("wiadomosc bedzie dostarczona pozniej do %d.", e->event.ack.recipient);
		else
			print_debug("wiadomosc dotarla do %d.", e->event.ack.recipient);
		break;
	case GG_EVENT_PUBDIR50_SEARCH_REPLY:
		print_debug("GG_EVENT_PUBDIR50_SEARCH_REPLY");
		handle_search_event(e);
		break;
	}

	gg_free_event(e);

	if (session && prev_check != session->check)
	{
		prev_check = session->check;
		if (session->check == GG_CHECK_READ)
		{
			print_debug("GG_CHECK_READ");
			watch = g_io_add_watch(source, G_IO_IN | G_IO_ERR | G_IO_HUP, test_chan, NULL);
			return FALSE;
		}

		if (session->check == GG_CHECK_WRITE)
		{
			print_debug("GG_CHECK_WRITE");
			watch = g_io_add_watch(source, G_IO_OUT | G_IO_ERR | G_IO_HUP, test_chan, NULL);
			return FALSE;
		}
	}

	return TRUE;
}

void exit_signal_handler()
{
	if (connected)
		gg_logoff(session);

	show_error(_("Program is about to leave"));
}

gpointer user_chat_action(gpointer user_data)
{
	GSList *users = (GSList *) user_data;
	GGaduMsg *msg = NULL;

	if (!user_data)
		return NULL;

	msg = g_new0(GGaduMsg, 1);

	if (g_slist_length(users) > 1)
	{
		msg->class = GGADU_CLASS_CONFERENCE;

		while (users)
		{
			GGaduContact *k = (GGaduContact *) users->data;
			msg->id = g_strdup(k->id);
			msg->recipients = g_slist_append(msg->recipients, g_strdup(k->id));
			users = users->next;
		}
	}
	else
	{
		GGaduContact *k = (GGaduContact *) users->data;
		msg->class = GGADU_CLASS_CHAT;
		msg->id = g_strdup(k->id);
	}

	msg->message = NULL;

	signal_emit_full(GGadu_PLUGIN_NAME, "gui msg receive", msg, "main-gui", GGaduMsg_free);

	return NULL;
}

gpointer user_ask_remove_user_action(gpointer user_data)
{
	GGaduDialog *dialog;

	dialog = ggadu_dialog_new_full(GGADU_DIALOG_YES_NO, _("Are You sure You want to delete selected user(s)?"), "user remove user action", user_data);
	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");

	return NULL;
}

gpointer user_remove_user_action(gpointer user_data)
{
	GSList *users = (GSList *) user_data;

	while (users)
	{
		GGaduContact *k = (GGaduContact *) users->data;
		ggadu_repo_del_value("gadu-gadu", ggadu_repo_key_from_string(k->id));

		if (connected && session)
			gg_remove_notify(session, atoi(k->id));

		GGaduContact_free(k);
		users = users->next;
	}

	if (user_data)
	{
		signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", NULL, "main-gui");
		save_addressbook_file();
	}

	return NULL;
}

gpointer user_change_user_action(gpointer user_data)
{
	GSList *users = (GSList *) user_data;
	GGaduContact *k = (GGaduContact *) users->data;
	GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_GENERIC, _("Change contact informations"), "change user");
	ggadu_dialog_add_entry(dialog, GGADU_ID, "GG#", VAR_STR, k->id ? k->id : "", VAR_FLAG_INSENSITIVE);
	ggadu_dialog_add_entry(dialog, GGADU_NICK, _("Nick"), VAR_STR, k->nick ? k->nick : "", VAR_FLAG_SENSITIVE);
	ggadu_dialog_add_entry(dialog, GGADU_FIRST_NAME, _("First Name"), VAR_STR, k->first_name ? k->first_name : "", VAR_FLAG_SENSITIVE);
	ggadu_dialog_add_entry(dialog, GGADU_LAST_NAME, _("Last Name"), VAR_STR, k->last_name ? k->last_name : "", VAR_FLAG_SENSITIVE);
	ggadu_dialog_add_entry(dialog, GGADU_MOBILE, _("Phone"), VAR_STR, k->mobile ? k->mobile : "", VAR_FLAG_SENSITIVE);
	/* wywoluje okienko zmiany usera */
	/*ggadu_dialog_set_flags(dialog, GGADU_DIALOG_FLAG_PROGRESS); */
	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
	return NULL;
}

gpointer user_add_user_action(gpointer user_data)
{
	GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_GENERIC, _("Add contact"), "add user");
	ggadu_dialog_add_entry(dialog, GGADU_ID, "GG#", VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_NICK, _("Nick"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_FIRST_NAME, _("First Name"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_LAST_NAME, _("Last Name"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_MOBILE, _("Phone"), VAR_STR, NULL, VAR_FLAG_NONE);
	/* wywoluje okienko dodawania usera */
	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
	return NULL;
}

gpointer user_preferences_action(gpointer user_data)
{
	gchar *utf = NULL;
	GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("Gadu-gadu plugin configuration"), "update config");
	GList *statuslist_names = NULL;
	GSList *tmplist = p->statuslist;
	while (tmplist)
	{
		GGaduStatusPrototype *sp = tmplist->data;

		if ((!sp->receive_only) && (sp->status != GG_STATUS_NOT_AVAIL_DESCR) && (sp->status != GG_STATUS_NOT_AVAIL))
			statuslist_names = g_list_append(statuslist_names, sp->description);

		/* set selected as first in list */
		if (sp->status == (gint) ggadu_config_var_get(handler, "status"))
			statuslist_names = g_list_prepend(statuslist_names, sp->description);

		tmplist = tmplist->next;
	}

	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_CONFIG_ID, "GG#", VAR_INT, ggadu_config_var_get(handler, "uin"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_CONFIG_PASSWORD, _("Password"), VAR_STR, ggadu_config_var_get(handler, "password"), VAR_FLAG_PASSWORD);
	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_CONFIG_SERVER, _("Server (optional)"), VAR_STR, ggadu_config_var_get(handler, "server"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_CONFIG_PROXY,
			       _("Proxy server (optional)\n[user:pass@]host.com[:80]"), VAR_STR, ggadu_config_var_get(handler, "proxy"), VAR_FLAG_NONE);

	utf = to_utf8("ISO-8859-2", ggadu_config_var_get(handler, "reason"));

	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_CONFIG_REASON, _("Default reason"), VAR_STR, utf, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_CONFIG_HISTORY, _("Log chats to history file"), VAR_BOOL, ggadu_config_var_get(handler, "log"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_CONFIG_AUTOCONNECT, _("Autoconnect on startup"), VAR_BOOL, ggadu_config_var_get(handler, "autoconnect"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_CONFIG_AUTOCONNECT_STATUS, _("Autoconnect status"), VAR_LIST, statuslist_names, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_CONFIG_FRIENDS_MASK, _("Available only for friends"), VAR_BOOL, ggadu_config_var_get(handler, "private"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_CONFIG_DCC, _("Enable DCC"), VAR_BOOL, ggadu_config_var_get(handler, "dcc"), VAR_FLAG_NONE);

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
	g_free(utf);
	return NULL;
}

gpointer register_account(gpointer data)
{
	struct ggadu_gg_register *reg = (struct ggadu_gg_register *) data;
	struct gg_http *r = gg_register3(reg->email, reg->password, reg->token_id, reg->token, 0);
	struct gg_pubdir *p = (r ? (struct gg_pubdir *) r->data : NULL);
	gchar *uin = NULL;
	if (!r || !p || !p->success || !p->uin)
	{
		print_debug("gg_register3() failed!\n");
		signal_emit_from_thread(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Registration failed.")), "main-gui");
	}
	else
	{
		print_debug("registration process succeded: '%s'\n", r->body);
		uin = g_strdup_printf("%ld", (glong) p->uin);
		if (reg->update_config == TRUE)
		{
			ggadu_config_var_set(handler, "uin", (gpointer) atol(uin));
			ggadu_config_var_set(handler, "password", reg->password);
			ggadu_config_save(handler);
			signal_emit_from_thread(GGadu_PLUGIN_NAME, "gui show message",
						g_strdup_printf(_("Registration process succeded: UIN: %s\nSettings have been updated."), uin), "main-gui");
		}
		else
		{
			signal_emit_from_thread(GGadu_PLUGIN_NAME, "gui show message", g_strdup_printf(_("Registration process succeded: UIN: %s"), uin), "main-gui");
		}
	}

	gg_register_free(r);
	g_free(uin);
	g_free(reg->email);
	g_free(reg->password);
	g_free(reg->token_id);
	g_free(reg->token);
	g_free(reg);
	g_thread_exit(NULL);
	return NULL;
}

gpointer _register_account_action(gpointer user_data)
{
	GGaduDialog *dialog = NULL;
	GIOChannel *ch = NULL;
	gchar *token_image_path = NULL;
	struct gg_http *h = NULL;
	struct gg_token *t = NULL;

	g_static_mutex_lock(&register_mutex);
	h = gg_token(0);
	if (!h || !h->body)
	{
		print_debug("gg_token() failed\n");
		signal_emit_from_thread(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Registration failed.")), "main-gui");
		gg_token_free(h);
		g_static_mutex_unlock(&register_mutex);
		g_thread_exit(0);
		return NULL;
	}

	t = h->data;
	token_image_path = g_build_filename(g_get_tmp_dir(), "register-token.tmp", NULL);
	print_debug("Gonna write token to %s\n", token_image_path);
	ch = g_io_channel_new_file(token_image_path, "w", NULL);
	if (!ch)
	{
		print_debug("Couldnt open token image file %s for writing\n", token_image_path);
		signal_emit_from_thread(GGadu_PLUGIN_NAME, "gui show warning",
					g_strdup_printf(_("Registration failed:\ncouldn't write token image to %s"), token_image_path), "main-gui");
		g_free(token_image_path);
		gg_token_free(h);
		g_static_mutex_unlock(&register_mutex);
		g_thread_exit(NULL);
		return NULL;
	}

	/* set NULL encoding - safe for binary data */
	g_io_channel_set_encoding(ch, NULL, NULL);
	g_io_channel_write_chars(ch, h->body, h->body_size, NULL, NULL);
	g_io_channel_shutdown(ch, TRUE, NULL);
	g_io_channel_unref(ch);

	dialog = ggadu_dialog_new_full(GGADU_DIALOG_GENERIC, _("Register Gadu-Gadu account"), "register account", h);
	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_REGISTER_IMAGE, "", VAR_IMG, token_image_path, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_REGISTER_TOKEN, _("Registration code:\n(shown above)"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_REGISTER_EMAIL, _("Email:"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_REGISTER_PASSWORD, _("Password:"), VAR_STR, NULL, VAR_FLAG_PASSWORD);
	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_REGISTER_UPDATE_CONFIG, _("Update settings on success?"), VAR_BOOL, FALSE, VAR_FLAG_NONE);
	signal_emit_from_thread(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");

	g_free(token_image_path);

	g_static_mutex_unlock(&register_mutex);
	g_thread_exit(NULL);
	return NULL;
}

gpointer register_account_action(gpointer user_data)
{
	g_thread_create(_register_account_action, NULL, FALSE, NULL);
	return NULL;
}

gpointer search_action(gpointer user_data)
{
	GGaduDialog *dialog = NULL;
	GList *gender_list = NULL;

	if (!connected)
	{
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("You have to be connected to perform searching!")), "main-gui");
		return NULL;
	}

	gender_list = g_list_append(gender_list, NULL);
	gender_list = g_list_append(gender_list, _("female"));
	gender_list = g_list_append(gender_list, _("male"));

	dialog = ggadu_dialog_new(GGADU_DIALOG_GENERIC, _("Gadu-Gadu search"), "search");
	ggadu_dialog_add_entry(dialog, GGADU_SEARCH_FIRSTNAME, _("First name:"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_SEARCH_LASTNAME, _("Last name:"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_SEARCH_NICKNAME, _("Nick:"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_SEARCH_CITY, _("City:"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_SEARCH_BIRTHYEAR, _("Birthyear:"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_SEARCH_GENDER, _("Gender:"), VAR_LIST, gender_list, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_SEARCH_ID, _("GG#"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_SEARCH_ACTIVE, _("Search only for active users"), VAR_BOOL, NULL, VAR_FLAG_NONE);
	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
	g_list_free(gender_list);
	return NULL;
}

gchar *userlist_dump()
{
	GSList *list = ggadu_repo_get_as_slist("gadu-gadu", REPO_VALUE_CONTACT);
	GSList *us = list;
	gchar *dump = NULL;

	while (us)
	{
		gchar *line = NULL;
		GGaduContact *k_tmp = g_new0(GGaduContact, 1);
		GGaduContact *k = (GGaduContact *) us->data;

		k_tmp->first_name = g_strescape(k->first_name ? k->first_name : "", ggadu_iso_in_utf_str);
		k_tmp->last_name = g_strescape(k->last_name ? k->last_name : "", ggadu_iso_in_utf_str);
		k_tmp->nick = g_strescape(k->nick ? k->nick : "", ggadu_iso_in_utf_str);
		k_tmp->group = g_strescape(k->group ? k->group : "", ggadu_iso_in_utf_str);

		line = g_strdup_printf("%s;%s;%s;%s;%s;%s;%s;\r\n", k_tmp->first_name, k_tmp->last_name, k_tmp->nick, k_tmp->nick, k->mobile, k_tmp->group, k->id);

		GGaduContact_free(k_tmp);

		if (!dump)
		{
			dump = g_strdup(line);
		}
		else
		{
			gchar *tmp = dump;
			dump = g_strjoin(NULL, dump, line, NULL);
			g_free(tmp);
		}

		g_free(line);
		us = us->next;
	}
	g_slist_free(list);

	print_debug("userlist_dump");
	return dump;
}

gboolean user_exists(gchar * id)
{
	GGaduContact *k = ggadu_repo_find_value("gadu-gadu", ggadu_repo_key_from_string(id));

	if (k)
		return TRUE;

	return FALSE;
}

gboolean import_userlist(gchar * list)
{
	gchar **all, **tmp;

	if (!list)
		return FALSE;

	all = g_strsplit(list, "\r\n", 1000);
	tmp = all;
	while (*tmp)
	{
		gchar *first_name, *last_name, *nick, *comment, *mobile, *group, *uin;
		gchar **l = NULL;
		GGaduContact *k;
		/* print_debug ("pointer %p -> %p\n", tmp, *tmp); */
		l = g_strsplit(*tmp, ";", 12);
		tmp++;
		if (!l[0])
		{
			g_strfreev(l);
			continue;
		}

		first_name = l[0];
		last_name = l[1];
		nick = l[2] ? l[2] : g_strdup("unknown");
		mobile = l[4];
		group = l[5];
		uin = l[6];
		comment = l[7];
		if ((!uin) && (!mobile))
		{
			g_strfreev(l);
			continue;
		}

		if (user_exists(uin))
		{
			g_strfreev(l);
			continue;
		}

		k = g_new0(GGaduContact, 1);
		k->id = g_strdup(uin ? uin : "");
		k->first_name = g_strdup(first_name ? first_name : "");
		k->last_name = g_strdup(last_name ? last_name : "");
		k->nick = (!strlen(nick)) ? g_strconcat(first_name, " ", last_name, NULL) : g_strdup(nick);
		k->comment = g_strdup(comment ? comment : "");
		k->mobile = g_strdup(mobile ? mobile : "");
		k->group = g_strdup(group ? group : "");
		k->status = GG_STATUS_NOT_AVAIL;

		/*userlist = g_slist_append(userlist, k); */

		ggadu_repo_add_value("gadu-gadu", ggadu_repo_key_from_string(k->id), k, REPO_VALUE_CONTACT);

		if (connected && session)
			gg_add_notify(session, atoi(k->id));

		g_strfreev(l);
	}

	signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", NULL, "main-gui");

	save_addressbook_file();

	g_strfreev(all);
	return TRUE;
}

gpointer import_userlist_action(gpointer user_data)
{
	if (gg_userlist_request(session, GG_USERLIST_GET, NULL) == -1)
	{
		print_debug("userlist get error!");
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Userlist import failed!")), "main-gui");
		return NULL;
	}

	return NULL;
}

gpointer export_userlist_action(gpointer user_data)
{
	gchar *dump = NULL;
	gchar *tmp = userlist_dump();
	dump = from_utf8("CP1250", tmp);
	if (gg_userlist_request(session, GG_USERLIST_PUT, dump) == -1)
	{
		print_debug("userlist put error!\n");
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Userlist export failed!")), "main-gui");
	}
	else
	{
		signal_emit(GGadu_PLUGIN_NAME, "gui show message", g_strdup(_("Userlist export succeeded!")), "main-gui");
	}

	g_free(tmp);
	g_free(dump);
	return NULL;
}

gpointer delete_userlist_action(gpointer user_data)
{
	gchar *dump = g_strdup("");
	if (gg_userlist_request(session, GG_USERLIST_PUT, dump) == -1)
	{
		print_debug("userlist put error!\n");
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Userlist delete failed!")), "main-gui");
	}

	g_free(dump);
	return NULL;
}

gboolean test_chan_dcc_get(GIOChannel * source, GIOCondition condition, gpointer data)
{
	struct gg_event *e = NULL;
	struct gg_dcc *d = data;
	if (!(gboolean) ggadu_config_var_get(handler, "dcc"))
	{
		gadu_gadu_enable_dcc_socket(FALSE);
		return FALSE;
	}

	if (!(e = gg_dcc_watch_fd(d)))
	{
		if (d->type != GG_SESSION_DCC_SOCKET)
		{
			gg_free_dcc(d);
			print_debug("wylazimy stad albercik");
			return FALSE;
		}
	}

	switch (e->type)
	{
	case GG_EVENT_DCC_NEW:
		{
			struct gg_dcc *dcc_new = e->event.dcc_new;
			GIOChannel *ch = g_io_channel_unix_new(dcc_new->fd);
			watch_dcc_file = g_io_add_watch(ch, G_IO_ERR | G_IO_IN, test_chan_dcc, dcc_new);
			e->event.dcc_new = NULL;
			gg_event_free(e);
			break;
		}
	case GG_EVENT_DCC_ERROR:
		print_debug("GG_EVENT_DCC_ERROR");
		switch (e->event.dcc_error)
		{
		case GG_ERROR_DCC_HANDSHAKE:
			print_debug("dcc_error_handshake");
			break;
		case GG_ERROR_DCC_NET:
			print_debug("dcc_error_network");
			break;
		case GG_ERROR_DCC_REFUSED:
			print_debug("dcc_error_refused");
			break;
		case GG_ERROR_DCC_EOF:
			print_debug("dcc_error_eof");
			signal_emit(GGadu_PLUGIN_NAME, "gui show message", g_strdup(_("File received succesful")), "main-gui");
			break;
		default:
			print_debug("dcc_error_unknown");
			break;
		}

		gg_event_free(e);
		gg_dcc_free(d);
		return FALSE;
		break;
	}

	if (d->check == GG_CHECK_READ)
	{
		print_debug("GG_CHECK_READ DCC_GET\n");
		watch_dcc_file = g_io_add_watch(source, G_IO_IN | G_IO_ERR, test_chan_dcc_get, d);
		return FALSE;
	}

/*	if (d->state == GG_STATE_READING_FILE_HEADER)
	{
		print_debug("GG_CHECK_READ DCC_GET\n");
		watch_dcc_file = g_io_add_watch(source, G_IO_IN | G_IO_OUT| G_IO_ERR, test_chan_dcc_get, d);
		return FALSE;
	}
*/
	if (d->check == GG_CHECK_WRITE)
	{
		print_debug("GG_CHECK_WRITE DCC_GET\n");
		watch_dcc_file = g_io_add_watch(source, G_IO_OUT | G_IO_ERR, test_chan_dcc_get, d);
		return FALSE;
	}

	return TRUE;
}

gboolean test_chan_dcc(GIOChannel * source, GIOCondition condition, gpointer data)
{
	struct gg_event *e = NULL;
	struct gg_dcc *d = data;
	static gint prev_check = -1;

	if (!(gboolean) ggadu_config_var_get(handler, "dcc"))
	{
		gg_free_dcc(d);
		return FALSE;
	}

	if (!(e = gg_dcc_watch_fd(d)))
	{
		if (d->type != GG_SESSION_DCC_SOCKET)
		{
			gg_free_dcc(d);
			print_debug("wylazimy staq albercik\n");
			return FALSE;
		}
	}

	switch (e->type)
	{
	case GG_EVENT_DCC_CLIENT_ACCEPT:
		{
			print_debug("GG_EVENT_DCC_CLIENT_ACCEPT %ld %ld %ld\n", (guint32) d->uin, (guint32) d->peer_uin, ggadu_config_var_get(handler, "uin"));
			gg_event_free(e);
		}
		break;
	case GG_EVENT_DCC_CALLBACK:
		print_debug("GG_EVENT_DCC_CALLBACK");
		gg_dcc_set_type(d, GG_SESSION_DCC_SEND);
		gg_event_free(e);
		break;
	case GG_EVENT_DCC_NEED_FILE_INFO:
		print_debug("GG_EVENT_DCC_NEED_FILE_INFO");
		gg_dcc_fill_file_info(d, dcc_send_request_filename);
		gg_event_free(e);
		break;
	case GG_EVENT_DCC_NEED_FILE_ACK:	/* dotyczy odbierania plikow */
		{
			GGaduDialog *dialog = NULL;
			GGaduContact *k;
			gchar *p = NULL;
			gchar *tmp = NULL;
			gchar *idtmp = g_strdup_printf("%d", d->peer_uin);

			if (!(k = ggadu_repo_find_value("gadu-gadu", ggadu_repo_key_from_string(idtmp))))
			{
				g_free(idtmp);
				g_free(tmp);
				gg_dcc_free(d);
				return FALSE;
			}
			print_debug("GG_EVENT_DCC_NEED_FILE_ACK");
/*
	    gg_dcc_set_type (d, GG_SESSION_DCC_GET);
*/
			for (p = d->file_info.filename; *p; p++)
				if (*p < 32 || *p == '\\' || *p == '/')
					*p = '_';
			if (d->file_info.filename[0] == '.')
				d->file_info.filename[0] = '_';

			tmp = g_strdup_printf(_("%s (%d) wants to send You a file: %s (%d bytes)"), k->nick, d->peer_uin, d->file_info.filename, d->file_info.size);
			dialog = ggadu_dialog_new_full(GGADU_DIALOG_GENERIC, tmp, "get file", (gpointer) d);
			signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
			g_free(tmp);
			g_free(idtmp);

			return FALSE;
		}
		gg_event_free(e);
		break;
	case GG_EVENT_DCC_DONE:
		{
			gint state = d->state;
			gchar *filename = g_strdup(d->file_info.filename);
			print_debug("GG_EVENT_DCC_DONE");
			gg_event_free(e);
			gg_dcc_free(d);
			if (state == GG_STATE_GETTING_FILE)
				signal_emit(GGadu_PLUGIN_NAME, "gui show message", g_strdup_printf(_("File %s received succesful"), filename), "main-gui");
			else
				signal_emit(GGadu_PLUGIN_NAME, "gui show message", g_strdup(_("File sent succesful")), "main-gui");
			g_free(filename);
			return FALSE;
		}
		gg_event_free(e);
		break;
	case GG_EVENT_DCC_ERROR:
		print_debug("GG_EVENT_DCC_ERROR\n");
		switch (e->event.dcc_error)
		{
		case GG_ERROR_DCC_HANDSHAKE:
			print_debug("dcc_error_handshake\n");
			if (d->state == GG_STATE_READING_FILE_ACK)
				signal_emit(GGadu_PLUGIN_NAME, "gui show message", g_strdup(_("File refused")), "main-gui");
			break;
		case GG_ERROR_DCC_NET:
			print_debug("dcc_error_network\n");
			break;
		case GG_ERROR_DCC_REFUSED:
			print_debug("dcc_error_refused\n");
			signal_emit(GGadu_PLUGIN_NAME, "gui show message", g_strdup(_("File refused")), "main-gui");
			break;
		default:
			print_debug("dcc_error_unknown\n");
			break;
		}

		gg_event_free(e);
		gg_dcc_free(d);
		return FALSE;
		break;
	}

	if (prev_check != d->check)
	{
		prev_check = d->check;
		if (d->check == GG_CHECK_READ)
		{
			print_debug("GG_CHECK_READ DCC\n");
			watch_dcc_file = g_io_add_watch(source, G_IO_IN | G_IO_ERR, test_chan_dcc, d);
			return FALSE;
		}

		if (d->check == GG_CHECK_WRITE)
		{
			print_debug("GG_CHECK_WRITE DCC\n");
			watch_dcc_file = g_io_add_watch(source, G_IO_OUT | G_IO_ERR, test_chan_dcc, d);
			return FALSE;
		}
	}


	return TRUE;
}

gpointer send_file_action(gpointer user_data)
{
	GSList *users = (GSList *) user_data;
	GGaduContact *k = (GGaduContact *) users->data;
	GGaduDialog *dialog = NULL;
	gint port;
	gchar **addr_arr = NULL;
	gchar *tmp;
	if (!connected || !k->ip || g_str_has_prefix(k->ip, "0.0.0.0"))
	{
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("You cannot send file to this person")), "main-gui");
		return NULL;
	}

	addr_arr = g_strsplit(k->ip, ":", 2);
	if (!addr_arr[0] || !addr_arr[1])
	{
		g_strfreev(addr_arr);
		return NULL;
	}

	port = atoi(addr_arr[1]);
	g_strfreev(addr_arr);
	if (port < 1)
	{
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("You cannot send file to this person")), "main-gui");
		return NULL;
	}

	tmp = g_strdup_printf(_("Sending File to %s"), k->ip);
	dialog = ggadu_dialog_new_full(GGADU_DIALOG_GENERIC, tmp, "send file", (gpointer) atoi((char *) k->id));
	g_free(tmp);

	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_CONTACT, NULL, VAR_NULL, k, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_GADU_GADU_SELECTED_FILE, _("Select File :"), VAR_FILE_CHOOSER, NULL, VAR_FLAG_NONE);
	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
	return NULL;
}


GGaduMenu *build_plugin_menu()
{
	GGaduMenu *root = ggadu_menu_create();
	GGaduMenu *item_gg = ggadu_menu_add_item(root, "Gadu-Gadu", NULL, NULL);
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Add Contact"), user_add_user_action, NULL));
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item("", NULL, NULL));
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Search for friends"), search_action, NULL));
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Preferences"), user_preferences_action, NULL));
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item("", NULL, NULL));
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Import userlist"), import_userlist_action, NULL));
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Export userlist"), export_userlist_action, NULL));
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Delete userlist"), delete_userlist_action, NULL));
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item("", NULL, NULL));
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Register account"), register_account_action, NULL));
	return root;
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 						PLUGIN STUFF
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
static void handle_sighup()
{
	if (session && session->state != GG_STATE_IDLE)
	{
		print_debug("disconected");
		gg_free_session(session);
		session = NULL;
	}

	signal(SIGHUP, handle_sighup);
}

GSList *status_init()
{
	GSList *list = NULL;
	GGaduStatusPrototype *sp;
	sp = g_new0(GGaduStatusPrototype, 9);
	sp->status = GG_STATUS_AVAIL;
	sp->description = g_strdup(_("Available"));
	sp->image = g_strdup("gadu-gadu-online.png");
	sp->receive_only = FALSE;
	list = g_slist_append(list, sp);
	sp++;
	sp->status = GG_STATUS_AVAIL_DESCR;
	sp->description = g_strdup(_("Available with description"));
	sp->image = g_strdup("gadu-gadu-online-descr.png");
	sp->receive_only = FALSE;
	list = g_slist_append(list, sp);
	sp++;
	sp->status = GG_STATUS_BUSY;
	sp->description = g_strdup(_("Busy"));
	sp->image = g_strdup("gadu-gadu-away.png");
	sp->receive_only = FALSE;
	list = g_slist_append(list, sp);
	sp++;
	sp->status = GG_STATUS_BUSY_DESCR;
	sp->description = g_strdup(_("Busy with description"));
	sp->image = g_strdup("gadu-gadu-away-descr.png");
	sp->receive_only = FALSE;
	list = g_slist_append(list, sp);
	sp++;
	sp->status = GG_STATUS_INVISIBLE;
	sp->description = g_strdup(_("Invisible"));
	sp->image = g_strdup("gadu-gadu-invisible.png");
	sp->receive_only = FALSE;
	list = g_slist_append(list, sp);
	sp++;
	sp->status = GG_STATUS_INVISIBLE_DESCR;
	sp->description = g_strdup(_("Invisible with description"));
	sp->image = g_strdup("gadu-gadu-invisible-descr.png");
	sp->receive_only = FALSE;
	list = g_slist_append(list, sp);
	sp++;
	sp->status = GG_STATUS_NOT_AVAIL;
	sp->description = g_strdup(_("Offline"));
	sp->image = g_strdup("gadu-gadu-offline.png");
	sp->receive_only = FALSE;
	list = g_slist_append(list, sp);
	sp++;
	sp->status = GG_STATUS_NOT_AVAIL_DESCR;
	sp->description = g_strdup(_("Offline with description"));
	sp->image = g_strdup("gadu-gadu-offline-descr.png");
	sp->receive_only = FALSE;
	list = g_slist_append(list, sp);
	sp++;
	sp->status = GG_STATUS_BLOCKED;
	sp->description = g_strdup(_("Blocked"));
	sp->image = g_strdup("gadu-gadu-invisible.png");
	sp->receive_only = TRUE;
	list = g_slist_append(list, sp);
	sp++;
	return list;
}


GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
	gchar *path = NULL;

	print_debug("%s : initialize", GGadu_PLUGIN_NAME);
	signal(SIGHUP, handle_sighup);
#if GGADU_DEBUG
	gg_debug_level = 255;
#endif
	GGadu_PLUGIN_ACTIVATE(conf_ptr);	/* wazne zeby wywolac to makro w tym miejscu */
	handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("Gadu-Gadu(c) protocol"));
	ggadu_config_var_add(handler, "uin", VAR_INT);
	ggadu_config_var_add(handler, "password", VAR_STR);
	ggadu_config_var_add(handler, "proxy", VAR_STR);
	ggadu_config_var_add(handler, "server", VAR_STR);
	ggadu_config_var_add_with_default(handler, "log", VAR_BOOL, (gpointer) TRUE);
	ggadu_config_var_add(handler, "autoconnect", VAR_BOOL);
	ggadu_config_var_add_with_default(handler, "status", VAR_INT, (gpointer) 1);
	ggadu_config_var_add(handler, "reason", VAR_STR);
	ggadu_config_var_add(handler, "private", VAR_BOOL);
	ggadu_config_var_add(handler, "dcc", VAR_BOOL);
	/* todo */
	ggadu_config_var_add(handler, "dcc_dir", VAR_STR);
	ggadu_config_var_add(handler, "ignore_unknown_sender", VAR_BOOL);

	if (g_getenv("HOME_ETC"))
	{
		gchar *gg2_path = NULL;
		gchar *gg2_dir = g_build_filename(g_getenv("HOME_ETC"), "gg2", NULL);
		this_configdir = g_build_filename(g_getenv("HOME_ETC"), "gg", NULL);

		path = g_build_filename(gg2_dir, "gadu_gadu", NULL);
		gg2_path = g_build_filename(gg2_dir, "gadu_gadu", NULL);
		if (!g_file_test(path, G_FILE_TEST_EXISTS))
		{
			g_free(path);
			path = g_build_filename(this_configdir, "config", NULL);

			ggadu_config_set_filename((GGaduPlugin *) handler, gg2_path);
			if (!ggadu_config_read_from_file(handler, path))
				g_warning(_("Unable to read configuration file for plugin %s"), "gadu-gadu");

		}
		else if (!ggadu_config_read_from_file(handler, path))
		{
			g_warning(_("Unable to read configuration file for plugin %s"), "gadu-gadu");
		}
		else
		{
			ggadu_config_set_filename((GGaduPlugin *) handler, path);
		}

		g_free(gg2_path);
	}
	else
	{
		gchar *gg2_path = NULL;
		gchar *gg2_dir = g_build_filename(g_get_home_dir(), ".gg2", NULL);
		this_configdir = g_build_filename(g_get_home_dir(), ".gg", NULL);

		path = g_build_filename(gg2_dir, "gadu_gadu", NULL);
		gg2_path = g_build_filename(gg2_dir, "gadu_gadu", NULL);

		if (!g_file_test(path, G_FILE_TEST_EXISTS))
		{
			g_free(path);
			path = g_build_filename(this_configdir, "config", NULL);

			ggadu_config_set_filename((GGaduPlugin *) handler, gg2_path);
			if (!ggadu_config_read_from_file(handler, path))
				g_warning(_("Unable to read configuration file for plugin %s"), "gadu-gadu");
		}
		else if (!ggadu_config_read_from_file(handler, path))
		{
			g_warning(_("Unable to read configuration file for plugin %s"), "gadu-gadu");
		}
		else
		{
			ggadu_config_set_filename((GGaduPlugin *) handler, path);
		}

		g_free(gg2_path);
	}

	g_free(path);
/* reserved for DEVEL branch 
#if GGADU_DEBUG
    path = g_build_filename (this_configdir, "config_test", NULL);

    mkdir (this_configdir, 0700);

    ggadu_config_set_filename ((GGaduPlugin *) handler, path);

    if (!ggadu_config_read (handler))
      {
	  g_free (path);
#endif
#if GGADU_DEBUG
      }
#endif
*/
	register_signal_receiver((GGaduPlugin *) handler, (signal_func_ptr) my_signal_receive);
	ggadu_repo_add("gadu-gadu");

	return handler;
}


void start_plugin()
{
	print_debug("%s : start_plugin\n", GGadu_PLUGIN_NAME);
	p = g_new0(GGaduProtocol, 1);
	p->display_name = g_strdup("Gadu-Gadu");
	p->protocol_uri = g_strdup("gg://");
	p->img_filename = g_strdup("gadu-gadu.png");
	p->statuslist = status_init();
	p->offline_status = g_slist_append(p->offline_status, (gint *) GG_STATUS_NOT_AVAIL);
	p->offline_status = g_slist_append(p->offline_status, (gint *) GG_STATUS_NOT_AVAIL_DESCR);
	p->away_status = g_slist_append(p->away_status, (gint *) GG_STATUS_BUSY);
	p->away_status = g_slist_append(p->away_status, (gint *) GG_STATUS_BUSY_DESCR);
	p->online_status = g_slist_append(p->online_status, (gint *) GG_STATUS_AVAIL);
	p->online_status = g_slist_append(p->online_status, (gint *) GG_STATUS_AVAIL_DESCR);
	handler->protocol = p;
	ggadu_repo_add_value("_protocols_", p->display_name, p, REPO_VALUE_PROTOCOL);
	signal_emit(GGadu_PLUGIN_NAME, "gui register protocol", p, "main-gui");
	/* try to register menu for this plugin in GUI */
	menu_pluginmenu = build_plugin_menu();
	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_pluginmenu, "main-gui");
	CHANGE_STATUS_SIG = register_signal(handler, "change status");
	CHANGE_STATUS_DESCR_SIG = register_signal(handler, "change status descr");
	SEND_MESSAGE_SIG = register_signal(handler, "send message");
	ADD_USER_SIG = register_signal(handler, "add user");
	CHANGE_USER_SIG = register_signal(handler, "change user");
	UPDATE_CONFIG_SIG = register_signal(handler, "update config");
	GET_USER_SIG = register_signal(handler, "get user");
	SEARCH_SIG = register_signal(handler, "search");
	EXIT_SIG = register_signal(handler, "exit");
	ADD_USER_SEARCH_SIG = register_signal(handler, "add user search");
	GET_CURRENT_STATUS_SIG = register_signal(handler, "get current status");
	SEND_FILE_SIG = register_signal(handler, "send file");
	GET_FILE_SIG = register_signal(handler, "get file");
	GET_USER_MENU_SIG = register_signal(handler, "get user menu");
	REGISTER_ACCOUNT = register_signal(handler, "register account");
	USER_REMOVE_USER_SIG = register_signal(handler, "user remove user action");

	load_addressbook_file("ISO-8859-2");
	signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", NULL, "main-gui");
	test();			/* ?! */
	if (ggadu_config_var_get(handler, "autoconnect") && !connected)
	{
		gchar *cp = NULL;
		gint status = (ggadu_config_var_check(handler, "status") ? (gint) ggadu_config_var_get(handler, "status") : GG_STATUS_AVAIL);
		if (ggadu_config_var_get(handler, "private"))
			status |= GG_STATUS_FRIENDS_MASK;
		cp = to_cp("ISO-8859-2", ggadu_config_var_get(handler, "reason"));
		gadu_gadu_login((ggadu_config_var_check(handler, "reason")) ? cp : _("no reason"), status);
		g_free(cp);
	}
}

void my_signal_receive(gpointer name, gpointer signal_ptr)
{
	GGaduSignal *signal = (GGaduSignal *) signal_ptr;
	print_debug("%s : receive signal %d %s\n", GGadu_PLUGIN_NAME, signal->name, g_quark_to_string(signal->name));
	if (signal->name == EXIT_SIG)
	{
/*
	exit_signal_handler();
*/
		return;
	}

	if (signal->name == GET_USER_MENU_SIG)
	{
		GGaduMenu *umenu = ggadu_menu_create();

		ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("Chat"), user_chat_action, NULL));
		if ((gboolean) ggadu_config_var_get(handler, "dcc"))
			ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("Send File"), send_file_action, NULL));

		ggadu_menu_add_user_menu_extensions(umenu, handler);
		ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("Edit"), user_change_user_action, NULL));
		ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("Remove"), user_ask_remove_user_action, NULL));
		ggadu_menu_add_submenu(umenu, ggadu_menu_new_item("", NULL, NULL));
		ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("Add New"), user_add_user_action, NULL));
		ggadu_menu_print(umenu, NULL);
		signal->data_return = umenu;
	}

	if (signal->name == GET_FILE_SIG)
	{
		GGaduDialog *d = signal->data;
		struct gg_dcc *dcc = d->user_data;

		if (ggadu_dialog_get_response(d) == GGADU_OK)
		{
			GIOChannel *ch;
			gchar *dest_path = NULL;

			ch = g_io_channel_unix_new(dcc->fd);

			dest_path = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S, dcc->file_info.filename, NULL);
			dcc->file_fd = open(dest_path, O_WRONLY | O_CREAT, 0600);
			g_free(dest_path);

			if (dcc->file_fd == -1)
			{
				signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Unable to create file")), "main-gui");
			}

			if (dcc->check == GG_CHECK_READ)
			{
				print_debug("GG_CHECK_READ DCC\n");
				watch_dcc_file = g_io_add_watch(ch, G_IO_IN | G_IO_ERR, test_chan_dcc_get, dcc);
				return;
			}

			if (dcc->check == GG_CHECK_WRITE)
			{
				print_debug("GG_CHECK_WRITE DCC\n");
				watch_dcc_file = g_io_add_watch(ch, G_IO_OUT | G_IO_ERR, test_chan_dcc_get, dcc);
				return;
			}

		}
		else
		{
			gg_dcc_free(dcc);
		}

		GGaduDialog_free(d);
		return;
	}

	if (signal->name == SEND_FILE_SIG)
	{
		GGaduDialog *d = signal->data;
		GSList *tmplist = ggadu_dialog_get_entries(d);
		gint uin = (gint) ggadu_config_var_get(handler, "uin");
		struct gg_dcc *dcc_file_session = NULL;
		gchar **addr_arr = NULL;
		gint port = 0;
		guint32 ip = 0;
		gchar *filename = NULL;
		GGaduContact *k = NULL;
		GIOChannel *dcc_channel_file = NULL;

		if (ggadu_dialog_get_response(d) != GGADU_OK)
		{
			GGaduDialog_free(d);
			return;
		}

		while (tmplist)
		{
			GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
			switch (kv->key)
			{
			case GGADU_GADU_GADU_SELECTED_FILE:
				filename = kv->value;
				break;
			case GGADU_GADU_GADU_CONTACT:
				k = kv->value;
				break;
			}

			tmplist = tmplist->next;
		}

		if (!filename || strlen(filename) <= 0)
			return;

		addr_arr = g_strsplit(k->ip, ":", 2);
		if (!addr_arr[0] || !addr_arr[1])
		{
			g_strfreev(addr_arr);
			return;
		}

		ip = inet_addr(addr_arr[0]);
		port = atoi(addr_arr[1]);

		print_debug("SEND TO IP : %s %d\n", addr_arr[0], port);

		g_strfreev(addr_arr);

		if (port <= 10)
		{
			gg_dcc_request(session, (guint32) d->user_data);
			dcc_send_request_filename = filename;
		}
		else
		{
			dcc_file_session = gg_dcc_send_file(ip, port, uin, atoi(k->id));
			if (!dcc_file_session)
				return;
			gg_dcc_fill_file_info(dcc_file_session, filename);
			dcc_channel_file = g_io_channel_unix_new(dcc_file_session->fd);
			watch_dcc_file = g_io_add_watch(dcc_channel_file, G_IO_OUT | G_IO_IN | G_IO_ERR, test_chan_dcc, dcc_file_session);
		}

		GGaduDialog_free(d);
		return;
	}


	if (signal->name == ADD_USER_SIG)
	{
		GGaduDialog *dialog = signal->data;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			GGaduContact *k = g_new0(GGaduContact, 1);	/* do not free */
			GSList *kvlist = ggadu_dialog_get_entries(dialog);
			while (kvlist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) kvlist->data;
				switch ((gint) kv->key)
				{
				case GGADU_ID:
					k->id = g_strdup(kv->value);
					break;
				case GGADU_NICK:
					k->nick = g_strdup(kv->value ? (gchar *) kv->value : "");
					break;
				case GGADU_FIRST_NAME:
					k->first_name = g_strdup(kv->value ? (gchar *) kv->value : "");
					break;
				case GGADU_LAST_NAME:
					k->last_name = g_strdup(kv->value ? (gchar *) kv->value : "");
					break;
				case GGADU_MOBILE:
					k->mobile = g_strdup(kv->value ? (gchar *) kv->value : "");
					break;
				}
				kvlist = kvlist->next;
			}

			/* initial status for added person */
			k->status = GG_STATUS_NOT_AVAIL;
			k->group = g_strdup("");
			ggadu_repo_add_value("gadu-gadu", ggadu_repo_key_from_string(k->id), k, REPO_VALUE_CONTACT);

			if ((connected == TRUE) && (session != NULL) & (k != NULL) && (k->id != NULL))
				gg_add_notify(session, atoi(k->id));

			save_addressbook_file();
			signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", NULL, "main-gui");
		}
		GGaduDialog_free(dialog);
		return;
	}

	if (signal->name == GET_USER_SIG)
	{
		/* dupa */
		GGaduContact *k = NULL;
		gchar *id = (gchar *)signal->data;
		if (id && (k = ggadu_repo_find_value("gadu-gadu", ggadu_repo_key_from_string(id))))
		{
		    /* return copy of GGaduContact */
		    signal->data_return = GGaduContact_copy(k);
		}
		
	}

	if (signal->name == CHANGE_USER_SIG)
	{
		GGaduDialog *dialog = signal->data;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			gchar *id = NULL;
			GGaduContact *k = NULL;
			GSList *kvlist = ggadu_dialog_get_entries(dialog);

			/* figure out contact ID */
			while (kvlist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) kvlist->data;
				switch ((gint) kv->key)
				{
				case GGADU_ID:
					id = g_strdup(kv->value);
					break;
				}
				kvlist = kvlist->next;
			}

			/* change if exist */
			if ((k = ggadu_repo_find_value("gadu-gadu", ggadu_repo_key_from_string(id))))
			{
				kvlist = ggadu_dialog_get_entries(dialog);
				while (kvlist)
				{
					GGaduKeyValue *kv = (GGaduKeyValue *) kvlist->data;
					switch ((gint) kv->key)
					{
					case GGADU_NICK:
						g_free(k->nick);
						k->nick = g_strdup(kv->value ? (gchar *) kv->value : "");
						break;
					case GGADU_FIRST_NAME:
						g_free(k->first_name);
						k->first_name = g_strdup(kv->value ? (gchar *) kv->value : "");
						break;
					case GGADU_LAST_NAME:
						g_free(k->last_name);
						k->last_name = g_strdup(kv->value ? (gchar *) kv->value : "");
						break;
					case GGADU_MOBILE:
						g_free(k->mobile);
						k->mobile = g_strdup(kv->value ? (gchar *) kv->value : "");
						break;
					}
					kvlist = kvlist->next;
				}

				ggadu_repo_change_value("gadu-gadu", ggadu_repo_key_from_string(id), k, REPO_VALUE_DC);
				save_addressbook_file();
				signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", NULL, "main-gui");
			}
			g_free(id);
		}
		GGaduDialog_free(dialog);
		return;
	}

	if (signal->name == ADD_USER_SEARCH_SIG)
	{
		GGaduContact *k = signal->data;
		/* initial status for added person */
		k->status = GG_STATUS_NOT_AVAIL;
		ggadu_repo_add_value("gadu-gadu", ggadu_repo_key_from_string(k->id), k, REPO_VALUE_CONTACT);
		signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", NULL, "main-gui");
		save_addressbook_file();

		if (connected && session)
			gg_add_notify(session, atoi(k->id));

		return;
	}

	if (signal->name == CHANGE_STATUS_SIG)
	{
		GGaduStatusPrototype *sp = signal->data;

		if (!sp)
			return;

		if (sp->status != GG_STATUS_NOT_AVAIL)
		{
			/* descriptions, call dialogbox */
			if (ggadu_is_status_descriptive(sp->status))
			{
				GGaduDialog *dialog = NULL;
				gchar *reason_c = NULL;

				if (ggadu_get_protocol_status_description(p))
				{
					reason_c = g_strdup(ggadu_get_protocol_status_description(p));
				}
				else
				{
					reason_c = to_utf8("ISO-8859-2", ggadu_config_var_get(handler, "reason"));
				}

				dialog = ggadu_dialog_new_full(GGADU_DIALOG_GENERIC, _("Enter status description"), "change status descr", sp);
				ggadu_dialog_add_entry(dialog, 0, _("Description:"), VAR_STR, (gpointer) reason_c, VAR_FLAG_FOCUS);
				signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
				g_free(reason_c);
			}
			else
			{
				gint _status = sp->status;
				if (ggadu_config_var_get(handler, "private"))
					_status |= GG_STATUS_FRIENDS_MASK;
				/* if not connected, login. else, just change status */
				if (!connected)
				{
					connect_count = 0;
					gadu_gadu_login(NULL, sp->status);
				}
				else if (gg_change_status(session, _status) == -1)
				{
					signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Unable to change status")), "main-gui");
					print_debug("zjebka podczas change_status %d\n", sp->status);
				}
				else
				{
					signal_emit(GGadu_PLUGIN_NAME, "gui status changed", (gpointer) sp->status, "main-gui");
				}
			}
		}
		else
		{
			/* nie czekamy az serwer powie good bye tylko sami sie rozlaczamy */
			ggadu_gadu_gadu_disconnect();
		}

		return;
	}

	if (signal->name == CHANGE_STATUS_DESCR_SIG)
	{
		GGaduDialog *dialog = signal->data;

		if (ggadu_dialog_get_response(dialog) == GGADU_OK && dialog->user_data)
		{
			GGaduStatusPrototype *sp = dialog->user_data;
			GGaduKeyValue *kv = NULL;
			if (ggadu_dialog_get_entries(dialog))
			{
				gchar *desc_utf = NULL, *desc_cp = NULL, *desc_iso = NULL;
				gint _status = sp->status;

				if (ggadu_config_var_get(handler, "private"))
					_status |= GG_STATUS_FRIENDS_MASK;

				kv = (GGaduKeyValue *) ggadu_dialog_get_entries(dialog)->data;

				print_debug(" %d %d\n ", GG_STATUS_INVISIBLE_DESCR, sp->status);

				desc_utf = kv->value;
				/* do sesji */
				desc_cp = from_utf8("CP1250", desc_utf);
				/* do konfiga */
				desc_iso = from_utf8("ISO-8859-2", desc_utf);

				ggadu_config_var_set(handler, "reason", desc_iso);
				ggadu_set_protocol_status_description(p, desc_utf);

				if (!connected)
				{
					connect_count = 0;
					gadu_gadu_login(desc_cp, _status);
				}
				else if (!gg_change_status_descr(session, _status, desc_cp))
					signal_emit(GGadu_PLUGIN_NAME, "gui status changed", (gpointer) sp->status, "main-gui");

				g_free(desc_cp);
				g_free(desc_iso);
				ggadu_config_save(handler);
			}

			if (sp->status == GG_STATUS_NOT_AVAIL_DESCR)
			{
				ggadu_gadu_gadu_disconnect();
			}
		}
		GGaduDialog_free(dialog);
		return;
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
				case GGADU_GADU_GADU_CONFIG_ID:
					print_debug("changing var setting uin to %d\n", kv->value);
					ggadu_config_var_set(handler, "uin", kv->value);
					break;
				case GGADU_GADU_GADU_CONFIG_PASSWORD:
					print_debug("changing var setting password to %s\n", kv->value);
					ggadu_config_var_set(handler, "password", kv->value);
					break;
				case GGADU_GADU_GADU_CONFIG_SERVER:
					print_debug("changing var setting server to %s\n", kv->value);
					ggadu_config_var_set(handler, "server", kv->value);
					break;
				case GGADU_GADU_GADU_CONFIG_PROXY:
					print_debug("changing var setting proxy to %s\n", kv->value);
					ggadu_config_var_set(handler, "proxy", kv->value);
					break;
				case GGADU_GADU_GADU_CONFIG_HISTORY:
					print_debug("changing var setting log to %d\n", kv->value);
					ggadu_config_var_set(handler, "log", kv->value);
					break;
				case GGADU_GADU_GADU_CONFIG_DCC:
					print_debug("changing var setting dcc to %d", kv->value);
					ggadu_config_var_set(handler, "dcc", kv->value);
					gadu_gadu_enable_dcc_socket((gboolean) ggadu_config_var_get(handler, "dcc"));
					break;
				case GGADU_GADU_GADU_CONFIG_AUTOCONNECT:
					print_debug("changing var setting autoconnect to %d", kv->value);
					ggadu_config_var_set(handler, "autoconnect", kv->value);
					break;
				case GGADU_GADU_GADU_CONFIG_AUTOCONNECT_STATUS:
					{
						/* change string description to int value depending on current locales so it cannot be hardcoded eh.. */
						GSList *statuslist_tmp = p->statuslist;
						gchar *value_txt = ((GSList *) kv->value)->data;
						gint val = -1;
						while (statuslist_tmp)
						{
							GGaduStatusPrototype *sp = (GGaduStatusPrototype *) statuslist_tmp->data;
							if (!ggadu_strcasecmp(sp->description, value_txt))
							{
								val = sp->status;
							}
							statuslist_tmp = statuslist_tmp->next;
						}

						print_debug("changing var setting status to %d", val);
						ggadu_config_var_set(handler, "status", (gpointer) val);
					}
					break;
				case GGADU_GADU_GADU_CONFIG_REASON:
					{
						gchar *utf = NULL;
						print_debug("changing derault reason %s", kv->value);
						utf = from_utf8("ISO-8859-2", kv->value);
						ggadu_config_var_set(handler, "reason", utf);
						g_free(utf);
					}
					break;
				case GGADU_GADU_GADU_CONFIG_FRIENDS_MASK:
					print_debug("changing var setting private to %d", kv->value);
					ggadu_config_var_set(handler, "private", kv->value);
					break;
				}
				tmplist = tmplist->next;
			}
			ggadu_config_save(handler);
		}
		GGaduDialog_free(dialog);
		return;
	}

	if (signal->name == SEND_MESSAGE_SIG)
	{
		GGaduMsg *msg = signal->data;
		if (connected && msg)
		{
			gchar *message = NULL;
			message = from_utf8("CP1250", msg->message);
			message = insert_cr(message);
			print_debug("%s", message);
			msg->time = time(NULL);
			if ((msg->class == GGADU_CLASS_CONFERENCE) && (msg->recipients != NULL))
			{
				gint i = 0;
				uin_t *recipients = g_malloc(g_slist_length(msg->recipients) * sizeof (uin_t));
				GSList *tmp = msg->recipients;
				while (tmp)
				{
					if (tmp->data != NULL)	/* ? */
						recipients[i++] = atoi((gchar *) tmp->data);
					tmp = tmp->next;
				}

				if (gg_send_message_confer(session, GG_CLASS_CHAT, g_slist_length(msg->recipients), recipients, message) == -1)
				{
					signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Unable to send message")), "main-gui");
					print_debug("Unable to send send message %s", msg->id);
				}
				else if (ggadu_config_var_get(handler, "log"))
				{
					GSList *tmp = msg->recipients;
					while (tmp)
					{
						GSList *list = ggadu_repo_get_as_slist("gadu-gadu", REPO_VALUE_CONTACT);
						GSList *us = list;
						gchar *line2 = NULL;

						while (us)
						{
							GGaduContact *k = (GGaduContact *) us->data;
							if (strcmp(k->id, msg->id) == 0)
								line2 = g_strdup_printf("%s", k->nick);

							us = us->next;
						}

						ggadu_gg_save_history(GGADU_HISTORY_TYPE_SEND, msg, line2);

						g_free(line2);
						g_slist_free(list);
						tmp = tmp->next;
					}
				}
			}
			else
			{
				gint class = GG_CLASS_MSG;
				if (msg->class == GGADU_CLASS_CHAT)
					class = GG_CLASS_CHAT;
				if ((msg->id == NULL) || (gg_send_message(session, class, atoi(msg->id), message) == -1))
				{
					signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Unable to send message")), "main-gui");
					print_debug("Unable to send message %s\n", msg->id);
				}
				else if (ggadu_config_var_get(handler, "log"))
				{
					GSList *list = ggadu_repo_get_as_slist("gadu-gadu", REPO_VALUE_CONTACT);
					GSList *us = list;
					gchar *line2 = NULL;

					while (us)
					{
						GGaduContact *k = (GGaduContact *) us->data;
						if (strcmp(k->id, msg->id) == 0)
							line2 = g_strdup_printf("%s", k->nick);

						us = us->next;
					}

					ggadu_gg_save_history(GGADU_HISTORY_TYPE_SEND, msg, line2);

					g_free(line2);
					g_slist_free(list);
				}
			}

			g_free(message);
			/* and more ? GGaduMsg_free is invoked anyway ? */
		} else {
			signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Unable to send message\nYou are not connected to Gadu-Gadu service")), "main-gui");
			print_debug("Unable to send send message, Not connected");
		}
	}

	if (signal->name == SEARCH_SIG)
	{
		GGaduDialog *dialog = signal->data;
		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			gg_pubdir50_t req;
			GSList *tmplist = ggadu_dialog_get_entries(dialog);

			if (!(req = gg_pubdir50_new(GG_PUBDIR50_SEARCH)))
			{
				GGaduDialog_free(dialog);
				return;
			}

			while (tmplist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
				switch (kv->key)
				{
				case GGADU_SEARCH_FIRSTNAME:
					if (kv->value && *(gchar *) kv->value)
					{
						gchar *first_name = to_cp("UTF-8", kv->value);
						gg_pubdir50_add(req, GG_PUBDIR50_FIRSTNAME, first_name);
						g_free(first_name);
					}
					break;
				case GGADU_SEARCH_LASTNAME:
					if (kv->value && *(gchar *) kv->value)
					{
						gchar *last_name = to_cp("UTF-8", kv->value);
						gg_pubdir50_add(req, GG_PUBDIR50_LASTNAME, last_name);
						g_free(last_name);
					}
					break;
				case GGADU_SEARCH_NICKNAME:
					if (kv->value && *(gchar *) kv->value)
					{
						gchar *nick_name = to_cp("UTF-8", kv->value);
						gg_pubdir50_add(req, GG_PUBDIR50_NICKNAME, nick_name);
						g_free(nick_name);
					}
					break;
				case GGADU_SEARCH_CITY:
					if (kv->value && *(gchar *) kv->value)
					{
						gchar *city = to_cp("UTF-8", kv->value);
						gg_pubdir50_add(req, GG_PUBDIR50_CITY, city);
						g_free(city);
					}
					break;
				case GGADU_SEARCH_BIRTHYEAR:
					if (kv->value && *(gchar *) kv->value)
						gg_pubdir50_add(req, GG_PUBDIR50_BIRTHYEAR, kv->value);
					break;
				case GGADU_SEARCH_GENDER:
					if (((GSList *) kv->value)->data)
					{
						gchar *val_txt = ((GSList *) kv->value)->data;
						if (!ggadu_strcasecmp(val_txt, _("female")))
							gg_pubdir50_add(req, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_FEMALE);
						if (!ggadu_strcasecmp(val_txt, _("male")))
							gg_pubdir50_add(req, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_MALE);
					};
					break;
				case GGADU_SEARCH_ACTIVE:
					if (kv->value)
						gg_pubdir50_add(req, GG_PUBDIR50_ACTIVE, GG_PUBDIR50_ACTIVE_TRUE);
					break;
				case GGADU_SEARCH_ID:
					if (kv->value)
						gg_pubdir50_add(req, GG_PUBDIR50_UIN, kv->value);
					break;
				}
				tmplist = tmplist->next;
			}

			if (gg_pubdir50(session, req) == -1)
			{
				signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Error! Cannot perform search!")), "main-gui");
			}

			gg_pubdir50_free(req);
		}
		GGaduDialog_free(dialog);
		return;
	}

	if (signal->name == GET_CURRENT_STATUS_SIG)
	{
		if (session)
		{
			gint s = ((session->status & GG_STATUS_FRIENDS_MASK) ? (session->status ^ GG_STATUS_FRIENDS_MASK) : session->status);
			signal->data_return = ggadu_find_status_prototype(p, s);
		}
		else
			signal->data_return = ggadu_find_status_prototype(p, GG_STATUS_NOT_AVAIL);
	}

	if (signal->name == REGISTER_ACCOUNT)
	{
		GGaduDialog *dialog = signal->data;
		GSList *tmplist = ggadu_dialog_get_entries(dialog);
		struct gg_http *h = (struct gg_http *) dialog->user_data;
		struct gg_token *t = h ? h->data : NULL;
		struct ggadu_gg_register *reg = NULL;
		gchar *reg_email = NULL;
		gchar *reg_password = NULL;
		gchar *reg_token = NULL;
		gboolean reg_update = FALSE;
		gchar *token_image_path = g_build_filename(g_get_tmp_dir(), "register-token.tmp", NULL);

		if (g_file_test(token_image_path, G_FILE_TEST_IS_REGULAR))
			unlink(token_image_path);

		g_free(token_image_path);

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			while (tmplist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
				switch (kv->key)
				{
				case GGADU_GADU_GADU_REGISTER_EMAIL:
					if (kv->value && *(gchar *) kv->value)
					{
						print_debug("email: %s\n", kv->value);
						reg_email = to_cp("UTF8", kv->value);
					}
					break;
				case GGADU_GADU_GADU_REGISTER_PASSWORD:
					if (kv->value && *(gchar *) kv->value)
					{
						print_debug("password: %s\n", kv->value);
						reg_password = to_cp("UTF8", kv->value);
					}
					break;
				case GGADU_GADU_GADU_REGISTER_TOKEN:
					if (kv->value && *(gchar *) kv->value)
					{
						print_debug("token: %s\n", kv->value);
						reg_token = to_cp("UTF8", kv->value);
					}
					break;
				case GGADU_GADU_GADU_REGISTER_UPDATE_CONFIG:
					print_debug("update config?: %d\n", kv->value);
					reg_update = (gboolean) kv->value;
					break;
				}
				tmplist = tmplist->next;
			}

			if (!reg_email || !reg_password || !reg_token)
			{
				signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("You have to fill in all fields")), "main-gui");
				g_free(reg_email);
				g_free(reg_password);
				g_free(reg_token);
				gg_token_free(h);
				GGaduDialog_free(dialog);
				return;
			}

			reg = g_new0(struct ggadu_gg_register, 1);
			reg->email = reg_email;
			reg->password = reg_password;
			reg->token_id = t ? g_strdup(t->tokenid) : NULL;
			reg->token = reg_token;
			reg->update_config = reg_update;
			g_thread_create(register_account, (gpointer) reg, FALSE, NULL);
		}

		gg_token_free(h);
		GGaduDialog_free(dialog);
		return;
	}

	if (signal->name == USER_REMOVE_USER_SIG)
	{
		GGaduDialog *d = signal->data;
		if (ggadu_dialog_get_response(d) == GGADU_OK)
		{
			user_remove_user_action(d->user_data);
		}
		g_slist_free(d->user_data);
		return;
	}

}

void destroy_plugin()
{
	ggadu_config_save(handler);
	print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
	if (menu_pluginmenu)
	{
		signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", menu_pluginmenu, "main-gui");
		ggadu_menu_free(menu_pluginmenu);
	}

	ggadu_repo_del("gadu-gadu");
	ggadu_repo_del_value("_protocols_", p);
	signal_emit(GGadu_PLUGIN_NAME, "gui unregister protocol", p, "main-gui");
	g_free(this_configdir);
}


void load_addressbook_file(gchar * encoding)
{
	FILE *fp;
	GGaduContact *k = NULL;
	gchar *line;
	gchar *path;
	path = g_build_filename(this_configdir, "userlist", NULL);
	fp = fopen(path, "r");
	g_free(path);
	if (!fp)
	{
		g_warning(_("I still cannot open contacts files! Exiting..."));
		return;
	}

	line = g_malloc0(1024);
	while (fgets(line, 1023, fp))
	{
		gchar *buf = NULL;
		gchar **l;
		gchar *first_name, *last_name, *nick, *nick2 /*, *comment */ , *mobile, *group, *uin;
		if (line[0] == '#' || !ggadu_strcasecmp(g_strstrip(line), ""))
			continue;
		buf = to_utf8(encoding, line);
		l = g_strsplit(buf, ";", 11);
		g_free(buf);
		if (!l[0] || !l[6])
		{		/* ZONK */
			g_strfreev(l);
			continue;
		}
		first_name = l[0];
		last_name = l[1];
		nick = l[2];
		nick2 = l[3];
    		mobile = l[4];
		group = l[5];
		uin = l[6];
		/* comment = l[6]; */
		if ((!uin || !*uin) && (!mobile || !*mobile))
		{
			g_strfreev(l);
			continue;
		}

		k = g_new0(GGaduContact, 1);
		k->id = g_strdup(uin ? uin : "");
		k->first_name = g_strdup(first_name);
		k->last_name = g_strdup(last_name);

		print_debug("'%s' '%s' '%s' '%s'", uin, nick, nick2, mobile);

		if (strlen(nick2) == 0)
			k->nick = (strlen(nick) == 0) ? g_strconcat(first_name, " ", last_name, NULL) : g_strdup(nick);
		else
			k->nick = g_strdup(nick2);

		if ((strlen(k->nick) == 0) || (!ggadu_strcasecmp(k->nick, " ")))
		{
			g_free(k->nick);
			k->nick = g_strdup(k->id);
		}

		/* k->comment = g_strdup (comment); */
		k->mobile = g_strdup(mobile);
		k->group = g_strdup(group);
		k->status = GG_STATUS_NOT_AVAIL;
		ggadu_repo_add_value("gadu-gadu", ggadu_repo_key_from_string(k->id), k, REPO_VALUE_CONTACT);
		g_strfreev(l);
	}

	g_free(line);
	fclose(fp);
}

void save_addressbook_file()
{
	GIOChannel *ch = NULL;
	gchar *path = NULL;
	gchar *dir = NULL;

	path = g_build_filename(this_configdir, "userlist", NULL);

	dir = g_path_get_dirname(path);
	if (strcmp(dir, ".") && !g_file_test(dir, G_FILE_TEST_EXISTS) && !g_file_test(dir, G_FILE_TEST_IS_DIR))
	{
		mkdir(dir, 0700);
	}
	else if (!g_file_test(dir, G_FILE_TEST_IS_DIR))
	{
		g_warning("Unable to open/create directory %s\n", dir);
		g_free(dir);
		g_free(path);
		return;
	}
	g_free(dir);

	ch = g_io_channel_new_file(path, "w", NULL);
	if (ch)
	{
		if (g_io_channel_set_encoding(ch, "ISO-8859-2", NULL) != G_IO_STATUS_ERROR)
		{
			gchar *temp = userlist_dump();

			if (temp != NULL)
				g_io_channel_write_chars(ch, temp, -1, NULL, NULL);

			g_free(temp);
		}
		g_io_channel_shutdown(ch, TRUE, NULL);
		g_io_channel_unref(ch);
	}
	g_free(path);

}
