/* $Id: tlen_plugin.c,v 1.59 2004/02/14 13:12:47 thrulliq Exp $ */

/* 
 * Tlen plugin for GNU Gadu 2 
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

#include <libtlen/libtlen.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "ggadu_conf.h"
#include "signals.h"
#include "menu.h"
#include "support.h"
#include "tlen_plugin.h"
#include "dialog.h"
#include "repo.h"

GGaduPlugin *handler;
GGaduProtocol *p;

static gchar *this_configdir = NULL;

struct tlen_session *session = NULL;

static GSList *userlist = NULL;

static GSList *search_results = NULL;

GIOChannel *source_chan = NULL;
gboolean connected = FALSE;
static guint tag = 0;

gchar *description = NULL;

gint watch = 0;

gpointer loginstatus;

GGadu_PLUGIN_INIT("tlen", GGADU_PLUGIN_TYPE_PROTOCOL);


/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 						PRIVATE STUFF
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

gint user_in_userlist(gpointer ula, gpointer cona)
{
	GSList *ul = ula;
	GGaduContact *k1;
	GGaduContact *k2 = cona;

	while (ul)
	{
		k1 = ul->data;
		if (!ggadu_strcasecmp(k1->id, k2->id))
			return 1;
		ul = ul->next;
	}

	return 0;
}

gboolean ping(gpointer data)
{
	if (!connected)
		return FALSE;
	tlen_ping(session);
	print_debug("TLEN PING sent!\n");
	return TRUE;
}

void ggadu_tlen_save_history(gchar * to, gchar * txt)
{
	gchar *dir = g_build_filename(this_configdir, "history", NULL);
	gchar *path = g_build_filename(this_configdir, "history", (to ? to : "UNKOWN"), NULL);

	if (!g_file_test(dir, G_FILE_TEST_IS_DIR))
		mkdir(dir, 0700);

	write_line_to_file(path, txt, "ISO-8859-2");

	g_free(path);
	g_free(dir);
}

gpointer user_view_history_action(gpointer user_data)
{
	gsize length, terminator;
	GIOChannel *ch = NULL;
	gchar *line = NULL;
	gchar *path = NULL;
	GString *hist_buf = NULL;
	GSList *users = (GSList *) user_data;
	GGaduContact *k = (users) ? (GGaduContact *) users->data : NULL;

	if (!k)
		return NULL;

	path = g_build_filename(this_configdir, "history", k->id, NULL);
	ch = g_io_channel_new_file(path, "r", NULL);
	g_free(path);

	if (!ch)
		return NULL;

	hist_buf = g_string_new(NULL);

	g_io_channel_set_encoding(ch, "ISO-8859-2", NULL);

	while (g_io_channel_read_line(ch, &line, &length, &terminator, NULL) != G_IO_STATUS_EOF)
	{
		if (line != NULL)
			g_string_append(hist_buf, line);
	}

	g_io_channel_shutdown(ch, TRUE, NULL);

	signal_emit(GGadu_PLUGIN_NAME, "gui show window with text", hist_buf->str, "main-gui");

	/* zwonic ten hist_buf */
	g_string_free(hist_buf, TRUE);

	return NULL;
}


void handle_search_item(struct tlen_pubdir *item)
{
	GGaduContact *k = g_new0(GGaduContact, 1);

	const gchar *id;
/*
	const gchar * first_name;
	const gchar * last_name;
	const gchar * nick;
	const gchar * city;
*/
	const gchar *age;
	int status = item->status;

	/* konwersja na UTF8 */
	id = to_utf8("ISO-8859-2", item->id);
	if ((item->firstname) != NULL)
		k->first_name = to_utf8("ISO-8859-2", item->firstname);
	if ((item->lastname) != NULL)
		k->last_name = to_utf8("ISO-8859-2", item->lastname);
	if ((item->nick) != NULL)
		k->nick = to_utf8("ISO-8859-2", item->nick);
	if ((item->city) != NULL)
		k->city = to_utf8("ISO-8859-2", item->city);

	age = g_strdup_printf("%d", item->age);

	k->id = g_strdup_printf("%s@tlen.pl", (id) ? id : "?");
	k->age = (age) ? g_strdup(age) : NULL;
	k->status = (status) ? status : TLEN_STATUS_UNAVAILABLE;

	search_results = g_slist_append(search_results, k);
}

gboolean updatewatch(struct tlen_session *sess)
{
	static int fd = 0;

	if ((sess->fd != fd) || (sess->state != 0))
	{
		if (tag)
		{
			if (g_source_remove(tag) == TRUE)
			{
				g_io_channel_unref(source_chan);
			}
			else
				return FALSE;
		}

		if ((source_chan = g_io_channel_unix_new(sess->fd)) == NULL)
			return FALSE;

		{
			GIOCondition cond;
			switch (sess->check)
			{
			case (TLEN_CHECK_READ | TLEN_CHECK_WRITE):
				cond = G_IO_IN | G_IO_OUT;
				break;
			case TLEN_CHECK_READ:
				cond = G_IO_IN;
				break;
			case TLEN_CHECK_WRITE:
				cond = G_IO_OUT;
				break;
			}
			/* sprawdzanie wyst±pienia warunków b³êdu */
			cond |= G_IO_ERR;
			cond |= G_IO_HUP;

			if ((tag = g_io_add_watch(source_chan, cond, test_chan, NULL)) == 0)
			{
				g_io_channel_unref(source_chan);
				return FALSE;
			}
		}
	}
	return TRUE;
}

gboolean test_chan(GIOChannel * source, GIOCondition condition, gpointer data)
{
	struct tlen_event *e;
	GGaduContact *k;
	GGaduNotify *notify;
	GGaduMsg *msg;
	GSList *l = userlist;

/*
	if (condition & G_IO_ERR || condition & G_IO_HUP) {
		connected = FALSE;
		tlen_presence(session,TLEN_STATUS_UNAVAILABLE,""); 
		return FALSE;
	}
*/
	tlen_watch_fd(session);

	if (session->error)
	{
		print_debug("Because of libtlen error, connection is terminating;");
		switch (session->error)
		{
		case TLEN_ERROR_UNAUTHORIZED:
			signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Unauthorized")), "main-gui");

			g_source_remove(watch);
			connected = FALSE;
			tag = 0;

			print_debug("libtlen error: Unauthorized\n");
			return FALSE;
			break;

		case TLEN_ERROR_BADRESPONSE:
			signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Bad response from server")),
				    "main-gui");
			print_debug("libtlen error: Bad response from server\n");
			break;

		case TLEN_ERROR_MALLOC:
			signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Memory allocation error")),
				    "main-gui");
			print_debug("libtlen error: Memory allocation error\n");
			break;

		case TLEN_ERROR_NETWORK:
			signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Network error")), "main-gui");
			print_debug("libtlen error: Network error\n");
			break;

		case TLEN_ERROR_OTHER:
			signal_emit(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(_("Unknown error")), "main-gui");
			print_debug("libtlen error: Unknown error\n");
			break;
		}
		if (updatewatch(session) == FALSE)
			print_debug("ooops, updatewatch() failed !!\n");
		tlen_presence(session, TLEN_STATUS_UNAVAILABLE, NULL);
		connected = FALSE;
		tlen_freesession(session);
		session = FALSE;
		signal_emit(GGadu_PLUGIN_NAME, "gui disconnected", NULL, "main-gui");
		return FALSE;
	}

	while ((e = tlen_getevent(session)) != NULL)
	{
		gchar *desc_utf8 = NULL;

		print_debug("%d", e->type);
		switch (e->type)
		{
		case TLEN_EVENT_AUTHORIZED:
			tlen_getroster(session);
			connected = TRUE;

			/* pingpong */
			g_timeout_add(100000, ping, NULL);
			break;

		case TLEN_EVENT_GOTROSTERITEM:
			if (e->roster->jid == NULL)
			{
				print_debug("%s \t\t B£¡D PODCZAS GOTROSTERITEM!\n", GGadu_PLUGIN_NAME);
				signal_emit(GGadu_PLUGIN_NAME, "gui show warning",
					    g_strdup(_("Error while GETROSTERITEM")), "main-gui");
				break;
			}

			k = g_new0(GGaduContact, 1);
			k->id = g_strdup(e->roster->jid);

			if (e->roster->name)
			{
				k->nick = to_utf8("ISO-8859-2", e->roster->name);
			}
			else
			{
				k->nick = g_strdup(e->roster->jid);
			}

			k->status = TLEN_STATUS_UNAVAILABLE;
			if (!user_in_userlist(userlist, k))
			{
				userlist = g_slist_append(userlist, k);
				ggadu_repo_add_value("tlen", k->id, k, REPO_VALUE_CONTACT);
			}
			break;

		case TLEN_EVENT_ENDROSTER:
			tlen_presence(session, (int) loginstatus, "");
			signal_emit(GGadu_PLUGIN_NAME, "gui status changed", loginstatus, "main-gui");
			signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
			break;

		case TLEN_EVENT_SUBSCRIBE:
			k = g_new0(GGaduContact, 1);
			k->id = g_strdup(e->subscribe->jid);
			tlen_accept_subscribe(session, k->id);
/*
			signal_emit(GGadu_PLUGIN_NAME, "auth request", k, "main-gui");
*/
			break;

		case TLEN_EVENT_SUBSCRIBED:
			k = g_new0(GGaduContact, 1);
			k->id = g_strdup(e->subscribe->jid);
/*
			signal_emit(GGadu_PLUGIN_NAME, "auth request accepted", k, "main-gui");
*/
			break;

		case TLEN_EVENT_UNSUBSCRIBE:
			k = g_new0(GGaduContact, 1);
			k->id = g_strdup(e->subscribe->jid);
			tlen_accept_unsubscribe(session, k->id);
/*
			signal_emit(GGadu_PLUGIN_NAME, "unauth request", k, "main-gui");
*/
			break;

		case TLEN_EVENT_UNSUBSCRIBED:
			k = g_new0(GGaduContact, 1);
			k->id = g_strdup(e->subscribe->jid);
/*
			signal_emit(GGadu_PLUGIN_NAME, "unauth request accepted", k, "main-gui");
*/
			break;


			/*
			 * Cholerka, czemu to nie dziala? Zachowuje sie tak jakby libtlen
			 * w ogole nie rozpoznawal tego eventu :/
			 * 
			 */
#if 0
		case TLEN_EVENT_NOTIFY:
			{
				gchar *notifytype;

				print_debug("otrzyma³e¶ powiadomienie!\n");
				print_debug("od: %s\n", e->message->from);

				switch (e->notify->type)
				{
				case TLEN_NOTIFY_TYPING:
					{
						notifytype = "pisze";
						/* jak bedzie support to sie doda powiadamianie */
						break;
					}
				case TLEN_NOTIFY_NOTTYPING:
					{
						notifytype = "przestal pisac";
						/* jak bedzie support to sie doda powiadamianie */
						break;
					}
				case TLEN_NOTIFY_SOUNDALERT:
					{
						notifytype = "powiadomienie d¼wiêkowe";
						/* na razie tak jak w tlenie - informacja mesgiem. Potem sie doda informowanie przez dzwiek */
						msg = g_new0(GGaduMsg, 1);

						msg->id = g_strdup_printf("%s", e->notify->from);

						msg->message =
							to_utf8("ISO-8859-2", "Rozmówca wys³a³ Ci alert d¼wiêkowy!");

						msg->class = TLEN_CHAT;
						signal_emit(GGadu_PLUGIN_NAME, "gui msg receive", msg, "main-gui");

						if (ggadu_config_var_get(handler, "log"))
						{
							gchar *line = g_strdup_printf("\n:: %s (%s) ::\n%s\n", msg->id,
										      get_timestamp(msg->time),
										      msg->message);
							ggadu_tlen_save_history(msg->id, line);
							g_free(line);
						}
						break;
					}
				}
				print_debug("typ: %s\n", notifytype);

				break;
			}
#endif /* 0 */

		case TLEN_EVENT_MESSAGE:
			print_debug("masz wiadomo¶æ!\n");
			print_debug("od: %s\n", e->message->from);
			print_debug("tre¶æ: %s\n", e->message->body);

			msg = g_new0(GGaduMsg, 1);

			msg->id = g_strdup_printf("%s", e->message->from);

			msg->message = to_utf8("ISO-8859-2", e->message->body);

			msg->class = e->message->type;
			signal_emit(GGadu_PLUGIN_NAME, "gui msg receive", msg, "main-gui");

			if (ggadu_config_var_get(handler, "log"))
			{
				gchar *line =
					g_strdup_printf("\n:: %s (%s) ::\n%s\n", msg->id, get_timestamp(msg->time),
							msg->message);
				ggadu_tlen_save_history(msg->id, line);
				g_free(line);
			}

			break;

		case TLEN_EVENT_PRESENCE:
			notify = g_new0(GGaduNotify, 1);
			notify->id = g_strdup_printf("%s", e->presence->from);
			notify->status = e->presence->status;

			print_debug("STATUS IN EVENT: %d\n", e->presence->status);

			desc_utf8 = ggadu_convert("ISO-8859-2", "UTF-8", e->presence->description);
			set_userlist_status(notify, desc_utf8, userlist);

			while (l)
			{
				GGaduContact *k = (GGaduContact *) l->data;

				if (!ggadu_strcasecmp(k->id, notify->id))
				{
					ggadu_repo_change_value("tlen", k->id, k, REPO_VALUE_DC);
				}
				l = l->next;
			}

			/* natomiast desc_utf8 nie mozna zwolnic, bo mamy takie glupie API */
			GGaduNotify_free(notify);

/*
			signal_emit(GGadu_PLUGIN_NAME,"gui notify",notify,"main-gui");
*/
			break;

		case TLEN_EVENT_GOTSEARCHITEM:
			handle_search_item(e->pubdir);
			break;

		case TLEN_EVENT_ENDSEARCH:
			if (search_results)
			{
				signal_emit(GGadu_PLUGIN_NAME, "gui show search results", search_results, "main-gui");
				search_results = NULL;
			}
			else
				signal_emit(GGadu_PLUGIN_NAME, "gui show message",
					    g_strdup(_("No users have been found!")), "main-gui");
			break;
		}
		tlen_freeevent(e);

	}

	if (updatewatch(session) == FALSE)
		print_debug("ooops, updatewatch() failed !!\n");

	return TRUE;
}

gpointer ggadu_tlen_login(gpointer data)
{
	gchar *login, *password;
#ifndef GGADU_DEBUG
	tlen_setdebug(0);
#else
	tlen_setdebug(1);
#endif

	if (!session)
	{
		session = tlen_init();
	}
	else
	{
		if (connected)
		{
			tlen_presence(session, TLEN_STATUS_UNAVAILABLE, "");
			g_io_channel_shutdown(source_chan, TRUE, NULL);
		}
		tlen_freesession(session);
		session = tlen_init();
	}

	login = ggadu_config_var_get(handler, "login");
	password = ggadu_config_var_get(handler, "password");

	if ((!login || !*login) || (!password || !*password))
	{
		user_preferences_action(NULL);
		/* warning pokazujemy pozniej, bo jest modalny! */
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning",
			    g_strdup(_("You have to enter your login and password first!")), "main-gui");
		signal_emit(GGadu_PLUGIN_NAME, "gui disconnected", NULL, "main-gui");
		return NULL;
	}

	print_debug("loguje sie to tlen_plugin %s %s\n", login, password);


	tlen_set_auth(session, login, password);
	tlen_set_hub_blocking(session, 0);
	tlen_login(session);
	if (updatewatch(session) == FALSE)
		print_debug("ooops, updatewatch() failed !!\n");

	loginstatus = data;

	return NULL;
}

void wyjdz_signal_handler()
{
	show_error(_("Program is about to leave"));
}

gpointer user_info_action(gpointer user_data)
{
	GGaduContact *k = user_data;
	if (k)
	{
		print_debug(", -----------------\n");
		print_debug("| id     :  %s\n", k->id);
		print_debug("| nick   :  %s\n", k->nick);
		print_debug("| status :  %d\n", k->status);
		print_debug("` -----------------\n");
	}
	return NULL;
}

gpointer user_chat_action(gpointer user_data)
{
	GSList *users = (GSList *) user_data;

	while (users)
	{
		GGaduContact *k = (GGaduContact *) users->data;
		GGaduMsg *msg = g_new0(GGaduMsg, 1);

		msg->id = k->id;
		msg->message = NULL;
		msg->class = TLEN_CHAT;

		signal_emit(GGadu_PLUGIN_NAME, "gui msg receive", msg, "main-gui");

		users = users->next;
	}

	return NULL;
}

gpointer user_remove_user_action(gpointer user_data)
{
	GSList *users = (GSList *) user_data;

	while (users)
	{
		GGaduContact *k = (GGaduContact *) users->data;

		userlist = g_slist_remove(userlist, k);
		ggadu_repo_del_value("tlen", k->id);
		tlen_request_unsubscribe(session, k->id);
		tlen_removecontact(session, k->id);

		GGaduContact_free(k);

		users = users->next;
	}

	signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");

	return NULL;
}

gpointer user_add_user_action(gpointer user_data)
{
	GGaduDialog *dialog = ggadu_dialog_new1(GGADU_DIALOG_GENERIC, "Add contact", "add user");
	ggadu_dialog_add_entry1(dialog, GGADU_TLEN_UIN, "Tlen ID", VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry1(dialog, GGADU_TLEN_NICK, _("Nick"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry1(dialog, GGADU_TLEN_GROUP, _("Group"), VAR_STR, NULL, VAR_FLAG_NONE);

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");

	return NULL;
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 						PLUGIN STUFF
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
	gchar *path = NULL;
	GGadu_PLUGIN_ACTIVATE(conf_ptr);	/* wazne zeby wywolac to makro w tym miejscu */
	print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);

	handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, "Tlen protocol");

	register_signal_receiver((GGaduPlugin *) handler, (signal_func_ptr) my_signal_receive);

	if (g_getenv("CONFIG_DIR") || g_getenv("HOME_ETC"))
		this_configdir =
			g_build_filename(g_get_home_dir(),
					 g_getenv("CONFIG_DIR") ? g_getenv("CONFIG_DIR") : g_getenv("HOME_ETC"), "tlen",
					 NULL);
	else
		this_configdir = g_build_filename(g_get_home_dir(), ".tlen", NULL);

	mkdir(this_configdir, 0700);

	path = g_build_filename(this_configdir, "config", NULL);
	ggadu_config_set_filename((GGaduPlugin *) handler, path);
	g_free(path);

	ggadu_config_var_add(handler, "login", VAR_STR);
	ggadu_config_var_add(handler, "password", VAR_STR);
	ggadu_config_var_add(handler, "log", VAR_BOOL);
	ggadu_config_var_add(handler, "autoconnect", VAR_BOOL);
	ggadu_config_var_add(handler, "autoconnect_status", VAR_INT);

	ggadu_config_read(handler);

	ggadu_repo_add("tlen");

	return handler;
}

GSList *status_init()
{
	GSList *list = NULL;
	GGaduStatusPrototype *sp;

	sp = g_new0(GGaduStatusPrototype, 8);

	sp->status = TLEN_STATUS_AVAILABLE;
	sp->description = g_strdup(_("Available"));
	sp->image = g_strdup("tlen-online.png");
	list = g_slist_append(list, sp);
	sp++;

	sp->status = TLEN_STATUS_AWAY;
	sp->description = g_strdup(_("Away"));
	sp->image = g_strdup("tlen-away.png");
	list = g_slist_append(list, sp);
	sp++;

	sp->status = TLEN_STATUS_CHATTY;
	sp->description = g_strdup(_("Chatty"));
	sp->image = g_strdup("tlen-chat.png");
	list = g_slist_append(list, sp);
	sp++;

	sp->status = TLEN_STATUS_EXT_AWAY;
	sp->description = g_strdup(_("Very busy"));
	sp->image = g_strdup("tlen-beback.png");
	list = g_slist_append(list, sp);
	sp++;

	sp->status = TLEN_STATUS_DND;
	sp->description = g_strdup(_("DND"));
	sp->image = g_strdup("tlen-occupied.png");
	list = g_slist_append(list, sp);
	sp++;

	sp->status = TLEN_STATUS_INVISIBLE;
	sp->description = g_strdup(_("Invisible"));
	sp->image = g_strdup("tlen-invisible.png");
	list = g_slist_append(list, sp);
	sp++;

	sp->status = TLEN_STATUS_UNAVAILABLE;
	sp->description = g_strdup(_("Unavailable"));
	sp->image = g_strdup("tlen-offline.png");
	list = g_slist_append(list, sp);
	sp++;

	sp->status = TLEN_STATUS_DESC;
	sp->description = g_strdup(_("Set description ..."));
	sp->image = g_strdup("tlen-desc.png");
	list = g_slist_append(list, sp);
	sp++;

	return list;
}

void free_search_results()
{
	GSList *tmplist = search_results;

	while (tmplist)
	{
		if (tmplist->data != NULL)
			GGaduContact_free((GGaduContact *) tmplist->data);
		tmplist = tmplist->next;
	}
	g_slist_free(search_results);
	search_results = NULL;
}

gpointer search_action(gpointer user_data)
{
	GGaduDialog *dialog = NULL;
	GList *gender_list = NULL;

	if (!connected)
	{
		signal_emit(GGadu_PLUGIN_NAME, "gui show warning",
			    g_strdup(_("You have to be connected to perform searching!")), "main-gui");
		return NULL;
	}

/*
	if (search_results)
		free_search_results();
*/
	gender_list = g_list_append(gender_list, NULL);
	gender_list = g_list_append(gender_list, _("female"));
	gender_list = g_list_append(gender_list, _("male"));

	dialog = ggadu_dialog_new1(GGADU_DIALOG_GENERIC, _("Tlen search"), "search");

	ggadu_dialog_add_entry1(dialog, GGADU_SEARCH_FIRSTNAME, _("First name:"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry1(dialog, GGADU_SEARCH_LASTNAME, _("Last name:"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry1(dialog, GGADU_SEARCH_NICKNAME, _("Nick:"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry1(dialog, GGADU_SEARCH_CITY, _("City:"), VAR_STR, NULL, VAR_FLAG_NONE);
	ggadu_dialog_add_entry1(dialog, GGADU_SEARCH_GENDER, _("Gender:"), VAR_LIST, gender_list, VAR_FLAG_NONE);
	ggadu_dialog_add_entry1(dialog, GGADU_SEARCH_ID, _("@tlen.pl"), VAR_STR, NULL, VAR_FLAG_NONE);

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");

	g_list_free(gender_list);

	return NULL;
}

gpointer user_preferences_action(gpointer user_data)
{
	GGaduDialog *dialog = ggadu_dialog_new1(GGADU_DIALOG_CONFIG,_("Tlen plugin configuration"),"update config");
	GSList *statuslist_names = NULL;
	GSList *tmplist = p->statuslist;

	while (tmplist)
	{
		GGaduStatusPrototype *sp = tmplist->data;
		if ((!sp->receive_only) && (sp->status != TLEN_STATUS_UNAVAILABLE) && (sp->status != TLEN_STATUS_DESC))
			statuslist_names = g_slist_append(statuslist_names, sp->description);

		if (sp->status == (gint) ggadu_config_var_get(handler, "autoconnect_status"))
			statuslist_names = g_slist_prepend(statuslist_names, sp->description);

		tmplist = tmplist->next;
	}

	ggadu_dialog_add_entry1(dialog, GGADU_TLEN_UIN, _("Tlen login"), VAR_STR,
			       ggadu_config_var_get(handler, "login"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry1(dialog, GGADU_TLEN_PASSWORD, _("Password"), VAR_STR,
			       ggadu_config_var_get(handler, "password"), VAR_FLAG_PASSWORD);
	ggadu_dialog_add_entry1(dialog, GGADU_TLEN_LOG, _("Log chats to history file"), VAR_BOOL,
			       ggadu_config_var_get(handler, "log"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry1(dialog, GGADU_TLEN_AUTOCONNECT, _("Autoconnect on startup"), VAR_BOOL,
			       ggadu_config_var_get(handler, "autoconnect"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry1(dialog, GGADU_TLEN_AUTOCONNECT_STATUS, _("Autoconnect status"), VAR_LIST,
			       statuslist_names, VAR_FLAG_NONE);

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");

	g_slist_free(statuslist_names);

	return NULL;
}


GGaduMenu *build_tlen_menu()
{
	GGaduMenu *root = ggadu_menu_create();
	GGaduMenu *item_tl = ggadu_menu_add_item(root, "_Tlen", NULL, NULL);

	ggadu_menu_add_submenu(item_tl, ggadu_menu_new_item(_("Add Contact"), user_add_user_action, NULL));
	ggadu_menu_add_submenu(item_tl, ggadu_menu_new_item(_("Preferences"), user_preferences_action, NULL));
	ggadu_menu_add_submenu(item_tl, ggadu_menu_new_item(_("Search for friends"), search_action, NULL));

	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", root, "main-gui");

	return root;
}


void start_plugin()
{
	print_debug("%s : start_plugin\n", GGadu_PLUGIN_NAME);

	p = g_new0(GGaduProtocol, 1);
	p->display_name = g_strdup("Tlen");
	p->img_filename = g_strdup("tlen.png");
	p->statuslist = status_init();
	p->offline_status = g_slist_append(p->offline_status, (gint *) TLEN_STATUS_UNAVAILABLE);
	p->away_status = g_slist_append(p->away_status, (gint *) TLEN_STATUS_AWAY);
	p->online_status = g_slist_append(p->online_status, (gint *) TLEN_STATUS_AVAILABLE);

	handler->protocol = p;

	ggadu_repo_add_value("_protocols_", p->display_name, p, REPO_VALUE_PROTOCOL);

	signal_emit(GGadu_PLUGIN_NAME, "gui register protocol", p, "main-gui");

	register_signal(handler, "change status");
	register_signal(handler, "change status descr");
	register_signal(handler, "send message");
	register_signal(handler, "add user");
	register_signal(handler, "change user");
	register_signal(handler, "update config");
	register_signal(handler, "search");
	register_signal(handler, "add user search");
	register_signal(handler, "get current status");

	build_tlen_menu();

	if (ggadu_config_var_get(handler, "autoconnect") && !connected)
		ggadu_tlen_login(ggadu_config_var_get(handler, "autoconnect_status") ? (gpointer)
				 ggadu_config_var_get(handler,
						      "autoconnect_status") : (gpointer) TLEN_STATUS_AVAILABLE);
}

void my_signal_receive(gpointer name, gpointer signal_ptr)
{
	GGaduSignal *signal = (GGaduSignal *) signal_ptr;
	print_debug("%s : receive signal %d\n", GGadu_PLUGIN_NAME, signal->name);

	if (signal->name == g_quark_from_static_string("wyjdz"))
	{
		wyjdz_signal_handler();
	}

	if (signal->name == g_quark_from_static_string("get user menu"))
	{
		GGaduMenu *umenu = ggadu_menu_create();
		GGaduMenu *listmenu = NULL;

		ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("Chat"), user_chat_action, NULL));
		ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("View History"), user_view_history_action, NULL));

		listmenu = ggadu_menu_new_item(_("List"), NULL, NULL);
		ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Add"), user_add_user_action, NULL));
		ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Remove"), user_remove_user_action, NULL));

		ggadu_menu_add_submenu(umenu, listmenu);

		ggadu_menu_print(umenu, NULL);

		signal->data_return = umenu;
	}


	if (signal->name == g_quark_from_static_string("change status"))
	{
		GGaduStatusPrototype *sp = signal->data;

		if (connected && sp)
		{
			if (sp->status == TLEN_STATUS_UNAVAILABLE)
			{
				g_source_remove(watch);
				tlen_presence(session, TLEN_STATUS_UNAVAILABLE, NULL);
				connected = FALSE;
				tlen_freesession(session);
				session = FALSE;
				if (g_source_remove(tag) == TRUE)
					g_io_channel_unref(source_chan);

				tag = 0;

				signal_emit(GGadu_PLUGIN_NAME, "gui disconnected", NULL, "main-gui");
				signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", NULL, "main-gui");
			}
			else if (sp->status == TLEN_STATUS_DESC)
			{
				GGaduDialog *dialog = NULL;
				GGaduStatusPrototype *_sp;
				GSList *tmp = NULL;
				gchar *desc_utf = NULL;

				/* Wyszukiwanie StatusPrototype :-D */

				tmp = handler->protocol->statuslist;

				if (tmp != NULL)
				{
					while (tmp)
					{
						GGaduStatusPrototype *sp = tmp->data;

						if ((sp) && (sp->status == session->status))
						{
							_sp = sp;
							break;
						}

						tmp = tmp->next;
					}
				}
				else
				{
					print_debug("Something went wrong, statuslist is NULL!!!\n");
				}
				/* end */

				dialog = ggadu_dialog_new1_full(GGADU_DIALOG_GENERIC,_("Enter status description"),"change status descr", _sp);
				desc_utf = to_utf8("ISO-8859-2", description);
				ggadu_dialog_add_entry1(dialog, TLEN_STATUS_DESC, _("Description"), VAR_STR,
						       desc_utf, VAR_FLAG_NONE);
				g_free(desc_utf);

				signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui");
				signal_emit(GGadu_PLUGIN_NAME, "gui status changed", (gpointer) _sp->status,
					    "main-gui");
			}
			else
			{
				tlen_presence(session, sp->status, description);
				signal_emit(GGadu_PLUGIN_NAME, "gui status changed", (gpointer) sp->status, "main-gui");
			}
		}
		else if (sp && sp->status != TLEN_STATUS_UNAVAILABLE && sp->status != TLEN_STATUS_DESC)
			ggadu_tlen_login((gpointer) sp->status);
		return;
	}

	if (signal->name == g_quark_from_static_string("update config"))
	{
		GGaduDialog *dialog = signal->data;
		GSList *tmplist = ggadu_dialog_get_entries(dialog);

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			while (tmplist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;

				switch (kv->key)
				{
				case GGADU_TLEN_UIN:
					print_debug("changing var setting uin to %s\n", kv->value);
					ggadu_config_var_set(handler, "login", kv->value);
					break;
				case GGADU_TLEN_PASSWORD:
					print_debug("changing var setting password to %s\n", kv->value);
					ggadu_config_var_set(handler, "password", kv->value);
					break;
				case GGADU_TLEN_LOG:
					print_debug("changing var setting log to %d\n", kv->value);
					ggadu_config_var_set(handler, "log", kv->value);
					break;
				case GGADU_TLEN_AUTOCONNECT:
					print_debug("changing var setting autoconnect to %d\n", kv->value);
					ggadu_config_var_set(handler, "autoconnect", kv->value);
					break;
				case GGADU_TLEN_AUTOCONNECT_STATUS:
					{
						GSList *statuslist_tmp = p->statuslist;
						gint val = -1;

						while (statuslist_tmp)
						{
							GGaduStatusPrototype *sp =
								(GGaduStatusPrototype *) statuslist_tmp->data;
							if (!ggadu_strcasecmp
							    (sp->description, ((GSList *) kv->value)->data))
							{
								val = sp->status;
							}
							statuslist_tmp = statuslist_tmp->next;
						}

						print_debug("changing var setting autoconnect_status to %d\n", val);
						ggadu_config_var_set(handler, "autoconnect_status", (gpointer) val);
					}
					break;
				}
				tmplist = tmplist->next;
			}
			ggadu_config_save(handler);
		}
		GGaduDialog_free(dialog);
		return;
	}

	if (signal->name == g_quark_from_static_string("change status descr"))
	{
		GGaduDialog *d = signal->data;
		GGaduStatusPrototype *sp = d->user_data;

		if (ggadu_dialog_get_response(d) == GGADU_OK)
		{
			if (connected == FALSE)
			{
				ggadu_tlen_login((gpointer) TLEN_STATUS_AVAILABLE);
			}
			else if (connected && sp)
			{
				if (ggadu_dialog_get_entries(d))
				{
					/* w kv->value jest opis, w utf8 */
					GGaduKeyValue *kv = (GGaduKeyValue *) d->optlist->data;

					/* zwolnij obecny opis */
					g_free(description);
					/* zaalokuj pamiec na nowy */
					description = from_utf8("ISO-8859-2", kv->value);

					/* ustaw nowy opis w sesji */
					tlen_presence(session, sp->status, description);
				}
			}
		}
		GGaduDialog_free(d);
		return;
	}

	if (signal->name == g_quark_from_static_string("add user"))
	{
		GGaduDialog *dialog = signal->data;
		GGaduContact *k = g_new0(GGaduContact, 1);

		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			GSList *kvlist = ggadu_dialog_get_entries(dialog);

			while (kvlist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) kvlist->data;

				switch ((gint) kv->key)
				{
				case GGADU_TLEN_UIN:
					k->id = g_strdup((gchar *) kv->value);
					break;

				case GGADU_TLEN_NICK:
					k->nick = g_strdup((gchar *) kv->value);
					break;

				case GGADU_TLEN_GROUP:
					k->group = g_strdup((gchar *) kv->value);
					break;
				}

				kvlist = kvlist->next;
			}
			userlist = g_slist_append(userlist, k);
			ggadu_repo_add_value("tlen", k->id, k, REPO_VALUE_CONTACT);

			tlen_addcontact(session, k->nick, k->id, k->group);

			tlen_request_subscribe(session, k->id);

			signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
		}
		GGaduDialog_free(dialog);
	}

	if (signal->name == g_quark_from_static_string("add user search"))
	{
		GGaduContact *k = signal->data;

		userlist = g_slist_append(userlist, k);
		ggadu_repo_add_value("tlen", k->id, k, REPO_VALUE_CONTACT);
		tlen_addcontact(session, k->nick, k->id, k->group);

		tlen_request_subscribe(session, k->id);
		signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");

		return;
	}

	if (signal->name == g_quark_from_static_string("send message"))
	{
		GGaduMsg *msg = signal->data;
		if (connected && msg)
		{
			gchar *message = from_utf8("ISO-8859-2", msg->message);

			if (msg->class == GGADU_CLASS_CHAT)
				msg->class = TLEN_CHAT;
			else
				msg->class = TLEN_MESSAGE;

			switch (msg->class)
			{
			case TLEN_CHAT:
				if (!tlen_sendmsg(session, msg->id, message, TLEN_CHAT))
					print_debug("zjebka podczas send message %s\n", msg->id, TLEN_CHAT);
				else if (ggadu_config_var_get(handler, "log"))
				{
					gchar *line = g_strdup_printf(_("\n:: Me (%s) ::\n%s\n"), get_timestamp(0),
								      msg->message);
					ggadu_tlen_save_history(msg->id, line);
					g_free(line);
				}

				break;
			case TLEN_MESSAGE:
				print_debug("Inside 'send message' handler: TLEN_MESSAGE!\n");
				if (!tlen_sendmsg(session, msg->id, message, TLEN_MESSAGE))
					print_debug("zjebka podczas send message %s\n", msg->id, TLEN_MESSAGE);
				else if (ggadu_config_var_get(handler, "log"))
				{
					gchar *line = g_strdup_printf(_("\n:: Me (%s) ::\n%s\n"), get_timestamp(0),
								      msg->message);
					ggadu_tlen_save_history(msg->id, line);
					g_free(line);
				}

				break;
			}

			g_free(message);
		}
	}

	if (signal->name == g_quark_from_static_string("search"))
	{
		GGaduDialog *dialog = signal->data;
		GSList *tmplist = ggadu_dialog_get_entries(dialog);
		struct tlen_pubdir *req = NULL;

		if ((ggadu_dialog_get_response(dialog) == GGADU_OK) ||
		    (ggadu_dialog_get_response(dialog) == GGADU_NONE))
		{
			if (!(req = tlen_new_pubdir()))
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
						req->firstname = g_strdup(kv->value);
					break;
				case GGADU_SEARCH_LASTNAME:
					if (kv->value && *(gchar *) kv->value)
						req->lastname = g_strdup(kv->value);
					break;
				case GGADU_SEARCH_NICKNAME:
					if (kv->value && *(gchar *) kv->value)
						req->nick = g_strdup(kv->value);
					break;
				case GGADU_SEARCH_CITY:
					if (kv->value && *(gchar *) kv->value)
						req->city = g_strdup(kv->value);
					break;
				case GGADU_SEARCH_GENDER:
					if (((GSList *) kv->value)->data)
					{
						gchar *val = ((GSList *) kv->value)->data;
						if (!ggadu_strcasecmp(val, _("female")))
							req->gender = 2;
						if (!ggadu_strcasecmp(val, _("male")))
							req->gender = 1;
					};
					break;
				case GGADU_SEARCH_ID:
					if (kv->value)
						req->id = g_strdup(kv->value);
					break;
				}

				tmplist = tmplist->next;
			}

			if (!(tlen_search(session, req)))
				signal_emit(GGadu_PLUGIN_NAME, "gui show warning",
					    g_strdup(_("Error! Cannot perform search!")), "main-gui");
			tlen_free_pubdir(req);
		}
		GGaduDialog_free(dialog);
	}

	if (signal->name == g_quark_from_static_string("get current status"))
	{
		if (session)
			signal->data_return = (gpointer) session->status;
		else
			signal->data_return = (gpointer) TLEN_STATUS_UNAVAILABLE;
	}
}

void destroy_plugin()
{
	print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
	ggadu_repo_del("tlen");
	ggadu_repo_del_value("_protocols_", p->display_name);
	signal_emit(GGadu_PLUGIN_NAME, "gui unregister userlist menu", NULL, "main-gui");
	signal_emit(GGadu_PLUGIN_NAME, "gui unregister protocol", p, "main-gui");
	g_free(this_configdir);
}
