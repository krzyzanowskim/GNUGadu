/* $Id: gadu_gadu_plugin.c,v 1.89 2003/11/16 09:16:55 krzyzak Exp $ */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
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

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "support.h"
#include "menu.h"
#include "gadu_gadu_plugin.h"
#include "dialog.h"
#include "repo.h"
#include "perl_embed.h"

extern GGaduConfig *config;

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
static GSList *userlist = NULL;

GGadu_PLUGIN_INIT ("gadu-gadu", GGADU_PLUGIN_TYPE_PROTOCOL);


/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 						PRIVATE STUFF
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
void test ()
{
}


void ggadu_gadu_gadu_disconnect ()
{
    GSList *tmplist = userlist;

    connected = FALSE;

    if (watch > 0)
	g_source_remove (watch);

    if (source_chan)
	g_io_channel_unref (source_chan);

    gg_logoff (session);

    gg_free_session (session);

    session = NULL;

//    ggadu_repo_disable_notification ();

    while (tmplist)
      {
	  GGaduContact *k = tmplist->data;
	  if (k->status != GG_STATUS_NOT_AVAIL)
	    {
		print_debug ("\t%s\n", k->id);
		k->status = GG_STATUS_NOT_AVAIL;
		ggadu_repo_change_value ("gadu-gadu", k->id, k, REPO_VALUE_DC);
	    }
	  tmplist = tmplist->next;
      }

//    ggadu_repo_enable_notification ();

    signal_emit (GGadu_PLUGIN_NAME, "gui disconnected", NULL, "main-gui");

}

void ggadu_gadu_gadu_disconnect_msg (gchar * txt)
{
    ggadu_gadu_gadu_disconnect ();
    print_debug ("%s\n", txt);
    signal_emit (GGadu_PLUGIN_NAME, "gui show warning", g_strdup ((txt) ? txt : _("Connection failed!")), "main-gui");
}

gchar *insert_cr (gchar * txt)
{
    gchar *in = txt;
    gchar *out = g_malloc0 (strlen (txt) * 2);
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

    start = g_try_realloc (start, strlen (start) + 1);

    return start;
}

void gadu_gadu_enable_dcc_socket (gboolean state)
{

    if ((state == TRUE) && (!dcc_session_get) && ((gboolean) config_var_get (handler, "dcc")))
      {
	  GIOChannel *dcc_channel_get = NULL;
	  gint cond = G_IO_IN;

	  dcc_session_get = gg_dcc_socket_create ((int) config_var_get (handler, "uin"), 0);
	  gg_dcc_ip = inet_addr ("255.255.255.255");
	  gg_dcc_port = dcc_session_get->port;

	  dcc_channel_get = g_io_channel_unix_new (dcc_session_get->fd);
	  cond = (dcc_session_get->check == GG_CHECK_READ) ? G_IO_IN : G_IO_OUT;
	  watch_dcc_file = g_io_add_watch (dcc_channel_get, G_IO_ERR | cond, test_chan_dcc_get, dcc_session_get);

      }
    else if (state == FALSE)
      {
	  g_source_remove (watch_dcc_file);
	  gg_free_dcc (dcc_session_get);
	  dcc_session_get = NULL;
	  gg_dcc_ip = 0;
	  gg_dcc_port = 0;
      }
}

gpointer gadu_gadu_login (gpointer desc, gint status)
{
    struct gg_login_params p;
    gchar *serveraddr = (gchar *) config_var_get (handler, "server");
    gchar **serv_addr = NULL;

    if (connected)
      {
	  gg_logoff (session);
	  gg_free_session (session);
	  connected = FALSE;
	  return FALSE;
      }

    gadu_gadu_enable_dcc_socket (TRUE);

    memset (&p, 0, sizeof (p));

    p.server_port = GG_DEFAULT_PORT;
    if (serveraddr == NULL)
	serveraddr = g_strdup ("217.17.41.85");
    else
      {
	  serv_addr = g_strsplit (serveraddr, ":", 2);

	  if (serv_addr)
	    {
		serveraddr = serv_addr[0];
		if (serv_addr[1] != NULL)
		    p.server_port = g_strtod (serv_addr[1], NULL);
		else
		    p.server_port = GG_DEFAULT_PORT;
	    }
      }

    /* jesli jest proxy zerzniete z ekg */
    if (config_var_check (handler, "proxy"))
      {
	  char **auth = array_make ((gchar *) config_var_get (handler, "proxy"), "@", 0, 0, 0);
	  char **proxy_userpass = NULL;
	  char **proxy_hostport = NULL;

	  gg_proxy_enabled = 1;

	  if (auth[0] && auth[1])
	    {
		proxy_userpass = array_make (auth[0], ":", 0, 0, 0);
		proxy_hostport = array_make (auth[1], ":", 0, 0, 0);
	    }
	  else
	      proxy_hostport = array_make (auth[0], ":", 0, 0, 0);

	  gg_proxy_host = g_strdup (proxy_hostport[0]);
	  gg_proxy_port = (proxy_hostport[1]) ? atoi (proxy_hostport[1]) : 8080;

	  array_free (proxy_hostport);
	  array_free (proxy_userpass);
	  array_free (auth);

      }

    p.uin = (int) config_var_get (handler, "uin");
    p.password = (gchar *) config_var_get (handler, "password");
    p.async = 1;
    p.status = status;
    p.status_descr = desc;
    /* ZONK this is to remove just to work with libgadu 1.4 */
    p.protocol_version = GG_DEFAULT_PROTOCOL_VERSION;

    if ((gint) config_var_get (handler, "private") == 1)
	p.status |= GG_STATUS_FRIENDS_MASK;

    if (serveraddr != NULL)
	p.server_addr = inet_addr (serveraddr);

    if (!p.uin || (!p.password || !*p.password))
      {
	  user_preferences_action (NULL);
	  signal_emit (GGadu_PLUGIN_NAME, "gui show warning",
		       g_strdup (_("You have to enter your GG# and password first!")), "main-gui");
	  ggadu_gadu_gadu_disconnect ();
	  return NULL;
      }


    print_debug ("loguje sie GG# %d do serwera %s %d, status %d\n", (gint) config_var_get (handler, "uin"), serveraddr,
		 p.server_port, status);

    g_free (serveraddr);

    if (!(session = gg_login (&p)))
      {
	  print_debug ("%s \t\t connection failed\n", GGadu_PLUGIN_NAME);
	  ggadu_gadu_gadu_disconnect_msg (NULL);
	  return NULL;
      }

    /* do tego chyba przydal by sie jakis nasz wrapper choc tak naprawde nie jest potrzebny */
    source_chan = g_io_channel_unix_new (session->fd);
    watch = g_io_add_watch (source_chan, G_IO_IN | G_IO_ERR | G_IO_HUP, test_chan, NULL);
    return NULL;
}

gboolean gadu_gadu_ping (gpointer data)
{
    if (connected == FALSE)
	return FALSE;

    gg_ping (session);
    print_debug ("PING! send!\n");

    return TRUE;
}

void handle_search_event (struct gg_event *e)
{
    gg_pubdir50_t res = e->event.pubdir50;
    gint count, i;
    GSList *list = NULL;
    GDate *cur_date;

    cur_date = g_date_new ();
    g_date_set_time (cur_date, time (NULL));

    if ((count = gg_pubdir50_count (res)) < 1)
      {
	  signal_emit (GGadu_PLUGIN_NAME, "gui show message", g_strdup (_("No users have been found!")), "main-gui");
	  return;
      }

    for (i = 0; i < count; i++)
      {
	  GGaduContact *k = g_new0 (GGaduContact, 1);
	  const gchar *uin = gg_pubdir50_get (res, i, GG_PUBDIR50_UIN);
	  const gchar *first_name = gg_pubdir50_get (res, i, GG_PUBDIR50_FIRSTNAME);
	  const gchar *nick = gg_pubdir50_get (res, i, GG_PUBDIR50_NICKNAME);
	  const gchar *status = gg_pubdir50_get (res, i, GG_PUBDIR50_STATUS);
	  const gchar *city = gg_pubdir50_get (res, i, GG_PUBDIR50_CITY);
	  const gchar *birthyear = gg_pubdir50_get (res, i, GG_PUBDIR50_BIRTHYEAR);
	  gint b_year;

	  k->id = g_strdup ((uin) ? uin : "?");

	  if (birthyear != NULL)
	    {
		b_year = ((cur_date->year) - atoi (birthyear));
		k->age = (b_year <= 99) ? g_strdup_printf ("%d", b_year) : NULL;
	    }
	  else
	      k->age = NULL;

	  if (first_name != NULL)
	      to_utf8 ("CP1250", first_name, k->first_name);

	  if (nick != NULL)
	      to_utf8 ("CP1250", nick, k->nick);

	  if (city != NULL)
	      to_utf8 ("CP1250", city, k->city);

	  k->status = (status) ? atoi (status) : GG_STATUS_NOT_AVAIL;

	  list = g_slist_append (list, k);
      }
    signal_emit (GGadu_PLUGIN_NAME, "gui show search results", list, "main-gui");
}

void ggadu_gg_save_history (gchar * to, gchar * txt)
{

    if (config_var_get (handler, "log"))
      {
	  gchar *dir = g_build_filename (this_configdir, "history", NULL);
	  gchar *path = g_build_filename (this_configdir, "history", to, NULL);

	  if (!g_file_test (dir, G_FILE_TEST_IS_DIR))
	      mkdir (dir, 0700);

	  write_line_to_file (path, txt, "CP1250");

	  g_free (path);
	  g_free (dir);
      }
}

gboolean test_chan (GIOChannel * source, GIOCondition condition, gpointer data)
{
    struct gg_event *e = NULL;
    GSList *slistmp = NULL;
    uint32_t *uins;
    GGaduNotify *notify = NULL;
    GGaduMsg *msg = NULL;
    gint i, j;
    GSList *l = userlist;
    static gint prev_check = GG_CHECK_READ;

    /* w przypadku bledu/utraty polaczenia postap tak jak w przypadku disconnect */
    if (!(e = gg_watch_fd (session)) || (condition & G_IO_ERR) ||
	((condition & G_IO_HUP) && (session->state != GG_STATE_CONNECTING_GG)))
      {
	  connected = FALSE;

	  if (++connect_count < 3)
	    {
		ggadu_gadu_gadu_disconnect ();
		gadu_gadu_login (NULL, GG_STATUS_AVAIL);
	    }
	  else
	    {
		gchar *txt = g_strdup (_("Disconnected"));
		ggadu_gadu_gadu_disconnect_msg (txt);
		connect_count = 0;
		g_free (txt);
	    }

	  return FALSE;
      }

    switch (e->type)
      {
      case GG_EVENT_NONE:
	  print_debug ("GG_EVENT_NONE!!\n");
	  break;

      case GG_EVENT_PONG:
	  gg_ping (session);
	  print_debug ("GG_EVENT_PONG!\n");
	  break;

      case GG_EVENT_CONN_SUCCESS:
	  print_debug ("polaczono!\n");
	  connected = TRUE;

	  /* notify wysylam */
	  slistmp = userlist;
	  i = 0;

	  while (slistmp)
	    {
		i++;
		slistmp = slistmp->next;
	    }

	  uins = g_malloc0 (i * sizeof (uint32_t));
	  slistmp = userlist;
	  j = 0;

	  while (slistmp)
	    {
		GGaduContact *k = slistmp->data;

		uins[j++] = atoi (k->id);
		slistmp = slistmp->next;
	    }

	  gg_notify (session, uins, i);
	  g_free (uins);

	  /* pingpong */
	  g_timeout_add (100000, gadu_gadu_ping, NULL);

	  if (config_var_get (handler, "sound_app_file") != NULL)
	      signal_emit (GGadu_PLUGIN_NAME, "sound play file", config_var_get (handler, "sound_app_file"), "sound*");

	  signal_emit (GGadu_PLUGIN_NAME, "gui status changed",
		       (gpointer) ((session->status & GG_STATUS_FRIENDS_MASK) ? session->
				   status ^ GG_STATUS_FRIENDS_MASK : session->status), "main-gui");
	  break;

      case GG_EVENT_CONN_FAILED:
	  print_debug ("nie udalo sie polaczyc\n");
	  ggadu_gadu_gadu_disconnect_msg (_("Connection failed"));
	  break;

      case GG_EVENT_DISCONNECT:
	  ggadu_gadu_gadu_disconnect_msg (_("Disconnected"));
	  gadu_gadu_login (NULL, GG_STATUS_AVAIL);
	  break;

      case GG_EVENT_MSG:
	  print_debug ("you have message!\n");
	  print_debug ("from: %d\n", e->event.msg.sender);
	  print_debug ("Type: %d\n", e->event.msg.msgclass);

	  if (e->event.msg.msgclass == GG_CLASS_CHAT)
	    {
		print_debug ("body: %s\n", e->event.msg.message);
	    }
	  else
	      /* dcc part */
	  if ((e->event.msg.msgclass == GG_CLASS_CTCP))
	    {
		struct gg_dcc *d = NULL;
		gchar *idtmp = g_strdup_printf ("%d", e->event.msg.sender);
		gchar **addr_arr = NULL;
		gpointer key = NULL,index = NULL;
		GGaduContact *k = NULL, *ktmp = NULL;
		GIOChannel *ch = NULL;

		print_debug ("somebody want to send us a file\n");

		//k = ggadu_repo_find_value ("gadu-gadu", idtmp);
  		index = ggadu_repo_value_first ("gadu-gadu", REPO_VALUE_CONTACT, (gpointer *)&key);
		while (index) {
			ktmp = ggadu_repo_find_value ("gadu-gadu", key);
			if (ktmp && (!ggadu_strcasecmp(ktmp->id,idtmp))) k = ktmp;
			index = ggadu_repo_value_next ("gadu-gadu", REPO_VALUE_CONTACT, (gpointer *)&key, index);
		}

		if (k == NULL) return TRUE;

		addr_arr = g_strsplit (k->ip, ":", 2);

		if (!addr_arr[0] || !addr_arr[1])
		    return TRUE;

		d = gg_dcc_get_file (inet_addr (addr_arr[0]), atoi (addr_arr[1]),
				     (gint) config_var_get (handler, "uin"), e->event.msg.sender);

		ch = g_io_channel_unix_new (d->fd);
		watch_dcc_file = g_io_add_watch (ch, G_IO_ERR | G_IO_IN | G_IO_OUT, test_chan_dcc, d);

		g_free (idtmp);
		g_free (addr_arr[0]);
		g_free (addr_arr[1]);
		break;
	    }
	  /* end of dcc part */

	  msg = g_new0 (GGaduMsg, 1);
	  msg->id = g_strdup_printf ("%d", e->event.msg.sender);
	  to_utf8 ("CP1250", e->event.msg.message, msg->message);
	  msg->class = e->event.msg.msgclass;
	  msg->time = e->event.msg.time;

	  if (e->event.msg.recipients_count > 0)
	    {
		gint i = 0;
		uin_t *recipients_gg = e->event.msg.recipients;

		print_debug ("CONFERENCE MESSAGE %d\n", e->event.msg.recipients_count);

		msg->class = GGADU_CLASS_CONFERENCE;
		msg->recipients =
		    g_slist_append (msg->recipients, (gpointer) g_strdup_printf ("%d", e->event.msg.sender));

		for (i = 0; i < e->event.msg.recipients_count; i++)
		    msg->recipients =
			g_slist_append (msg->recipients, (gpointer) g_strdup_printf ("%d", recipients_gg[i]));

	    }

	  {
	      gchar *line = g_strdup_printf (":: %s (%s)::\n%s\n\n", msg->id, get_timestamp (msg->time), msg->message);
	      print_debug ("%s\n", line);
	      ggadu_gg_save_history (msg->id, line);
	      g_free (line);
	  }

	  signal_emit (GGadu_PLUGIN_NAME, "gui msg receive", msg, "main-gui");

	  if (config_var_get (handler, "sound_msg_file") != NULL)
	      signal_emit (GGadu_PLUGIN_NAME, "sound play file", config_var_get (handler, "sound_msg_file"), "sound*");
	  break;

      case GG_EVENT_NOTIFY60:
      	  {
      	  int i;
	  
	  print_debug("GG_EVENT_NOTIFY60\n");
	  
          for (i = 0; e->event.notify60[i].uin; i++)
		{
		gchar *strIP = NULL;
		gchar *desc_utf8 = NULL;
		struct in_addr ip_addr;
		
		ip_addr.s_addr = e->event.notify60[i].remote_ip;

		notify = g_new0 (GGaduNotify, 1);
		notify->id = g_strdup_printf ("%d", e->event.notify60[i].uin);
		notify->status = e->event.notify60[i].status;

		strIP = inet_ntoa (ip_addr);	/* cannot be freed it staically allocated memory */
		
		if (strIP && (ggadu_strcasecmp (strIP, "0.0.0.0")))
		    notify->ip = g_strdup_printf ("%s:%d", strIP, e->event.notify60[i].remote_port);

		ggadu_convert ("CP1250", "UTF-8", e->event.notify60[i].descr, desc_utf8);

		set_userlist_status (notify, desc_utf8, userlist);

		l = userlist;

		while (l)
		  {
		      GGaduContact *k = (GGaduContact *) l->data;

		      if (!ggadu_strcasecmp (k->id, notify->id))
			  ggadu_repo_change_value ("gadu-gadu", k->id, k, REPO_VALUE_DC);

		      l = l->next;
		  }


		}
		
      	  }
      	  break;
	  
      case GG_EVENT_NOTIFY:
      case GG_EVENT_NOTIFY_DESCR:
          {
	  print_debug("GG_EVENT_NOTIFY\n");
	  struct gg_notify_reply *n = NULL;
	  
	  if (e->type == GG_EVENT_NOTIFY)
	  	n = e->event.notify;
	  else if (e->type == GG_EVENT_NOTIFY_DESCR)
	  	n = e->event.notify_descr.notify;

	  while (n->uin)
	    {
		struct in_addr ip_addr;
		gchar *strIP = NULL;
		gchar *desc_utf8 = NULL;

		ip_addr.s_addr = n->remote_ip;

		notify = g_new0 (GGaduNotify, 1);
		notify->id = g_strdup_printf ("%d", n->uin);
		notify->status = n->status;

		strIP = inet_ntoa (ip_addr);	/* cannot be freed it staically allocated memory */

		if (strIP && (ggadu_strcasecmp (strIP, "0.0.0.0")))
		    notify->ip = g_strdup_printf ("%s:%d", strIP, n->remote_port);

		if (e->type == GG_EVENT_NOTIFY_DESCR)
			ggadu_convert ("CP1250", "UTF-8", e->event.notify_descr.descr, desc_utf8);
		
		set_userlist_status (notify, desc_utf8, userlist);

		l = userlist;

		while (l)
		  {
		      GGaduContact *k = (GGaduContact *) l->data;

		      if (!ggadu_strcasecmp (k->id, notify->id))
			  ggadu_repo_change_value ("gadu-gadu", k->id, k, REPO_VALUE_DC);

		      l = l->next;
		  }

		n++;
	    }
 	  }
	  break;

      case GG_EVENT_STATUS:
	  {
	      gchar *desc_utf8 = NULL;

	      print_debug ("%s : GG_EVENT_STATUS : %d %s\n", GGadu_PLUGIN_NAME, e->event.status.uin,
			   e->event.status.descr);

	      ggadu_convert ("CP1250", "UTF-8", e->event.status.descr, desc_utf8);

	      notify = g_new0 (GGaduNotify, 1);
	      notify->id = g_strdup_printf ("%d", e->event.status.uin);
	      notify->status = e->event.status.status;

	      set_userlist_status (notify, desc_utf8, userlist);

	      while (l)
		{
		    GGaduContact *k = (GGaduContact *) l->data;

		    if (!ggadu_strcasecmp (k->id, notify->id))
			ggadu_repo_change_value ("gadu-gadu", k->id, k, REPO_VALUE_DC);

		    l = l->next;
		}
	  }
	  break;

      case GG_EVENT_ACK:

	  if (e->event.ack.status == GG_ACK_QUEUED)
	      print_debug ("wiadomosc bedzie dostarczona pozniej do %d.\n", e->event.ack.recipient);
	  else
	      print_debug ("wiadomosc dotarla do %d.\n", e->event.ack.recipient);

	  break;
      case GG_EVENT_PUBDIR50_SEARCH_REPLY:
	  handle_search_event (e);
	  break;
      }

    gg_free_event (e);

    if (session && prev_check != session->check)
      {
	  prev_check = session->check;
	  if (session->check == GG_CHECK_READ)
	    {
		print_debug ("GG_CHECK_READ\n");
		watch = g_io_add_watch (source, G_IO_IN | G_IO_ERR | G_IO_HUP, test_chan, NULL);
		return FALSE;
	    }

	  if (session->check == GG_CHECK_WRITE)
	    {
		watch = g_io_add_watch (source, G_IO_OUT | G_IO_ERR | G_IO_HUP, test_chan, NULL);
		print_debug ("GG_CHECK_WRITE\n");
		return FALSE;
	    }
      }

    return TRUE;
}

void exit_signal_handler ()
{
    if (connected)
	gg_logoff (session);

    show_error (_("Program is about to leave"));
}

gpointer user_chat_action (gpointer user_data)
{
    GSList *users = (GSList *) user_data;
    GGaduMsg *msg = g_new0 (GGaduMsg, 1);

    if (!user_data)
	return NULL;

    if (g_slist_length (users) > 1)
      {
	  msg->class = GGADU_CLASS_CONFERENCE;

	  while (users)
	    {
		GGaduContact *k = (GGaduContact *) users->data;
		msg->id = k->id;
		msg->recipients = g_slist_append (msg->recipients, k->id);
		users = users->next;
	    }
      }
    else
      {
	  GGaduContact *k = (GGaduContact *) users->data;
	  msg->class = GGADU_CLASS_CHAT;
	  msg->id = k->id;
      }

    msg->message = NULL;

    signal_emit (GGadu_PLUGIN_NAME, "gui msg receive", msg, "main-gui");

    return NULL;
}

gpointer user_view_history_action (gpointer user_data)
{
    gsize length, terminator;
    GIOChannel *ch = NULL;
    gchar *line = NULL;
    gchar *path = NULL;
    gchar *utf8str = NULL;
    GString *hist_buf = g_string_new (NULL);
    GSList *users = (GSList *) user_data;
    GGaduContact *k = (users) ? (GGaduContact *) users->data : NULL;

    if (!k)
	return NULL;

    path = g_build_filename (this_configdir, "history", k->id, NULL);
    ch = g_io_channel_new_file (path, "r", NULL);
    g_free (path);

    if (!ch)
	return NULL;

    g_io_channel_set_encoding (ch, "CP1250", NULL);

    while (g_io_channel_read_line (ch, &line, &length, &terminator, NULL) != G_IO_STATUS_EOF)
      {
	  if (line != NULL)
	      g_string_append (hist_buf, line);
      }

    g_io_channel_shutdown (ch, TRUE, NULL);

    signal_emit (GGadu_PLUGIN_NAME, "gui show window with text", hist_buf->str, "main-gui");
    g_free (utf8str);

    /* zwonic ten hist_buf */
    g_string_free (hist_buf, TRUE);

    return NULL;
}

gpointer user_remove_user_action (gpointer user_data)
{
    GSList *users = (GSList *) user_data;

    while (users)
      {
	  GGaduContact *k = (GGaduContact *) users->data;
	  userlist = g_slist_remove (userlist, k);
	  ggadu_repo_del_value ("gadu-gadu", k->id);

	  if (connected && session)
	      gg_remove_notify (session, atoi (k->id));

	  GGaduContact_free (k);
	  users = users->next;
      }

    if ((user_data) && (userlist))
      {
	  signal_emit (GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
	  save_addressbook_file (userlist);
      }

    return NULL;
}

gpointer user_change_user_action (gpointer user_data)
{
    GSList *optlist = NULL;
    GSList *users = (GSList *) user_data;
    GGaduContact *k = (GGaduContact *) users->data;

    ggadu_dialog_add_entry (&optlist, GGADU_ID, "GG#", VAR_STR, k->id, VAR_FLAG_INSENSITIVE);
    ggadu_dialog_add_entry (&optlist, GGADU_NICK, _("Nick"), VAR_STR, k->nick, VAR_FLAG_SENSITIVE);
    ggadu_dialog_add_entry (&optlist, GGADU_FIRST_NAME, _("First Name"), VAR_STR, k->first_name, VAR_FLAG_SENSITIVE);
    ggadu_dialog_add_entry (&optlist, GGADU_LAST_NAME, _("Last Name"), VAR_STR, k->last_name, VAR_FLAG_SENSITIVE);
    ggadu_dialog_add_entry (&optlist, GGADU_MOBILE, _("Phone"), VAR_STR, k->mobile, VAR_FLAG_SENSITIVE);

    /* wywoluje okienko zmiany usera */
    signal_emit (GGadu_PLUGIN_NAME, "gui change user window", optlist, "main-gui");

    return NULL;

}

gpointer user_add_user_action (gpointer user_data)
{
    GSList *optlist = NULL;

    ggadu_dialog_add_entry (&optlist, GGADU_ID, "GG#", VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&optlist, GGADU_NICK, _("Nick"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&optlist, GGADU_FIRST_NAME, _("First Name"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&optlist, GGADU_LAST_NAME, _("Last Name"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&optlist, GGADU_MOBILE, _("Phone"), VAR_STR, NULL, VAR_FLAG_NONE);

    /* wywoluje okienko dodawania usera */
    signal_emit (GGadu_PLUGIN_NAME, "gui add user window", optlist, "main-gui");

    return NULL;
}

gpointer user_preferences_action (gpointer user_data)
{
    gchar *utf = NULL;
    GGaduDialog *d = ggadu_dialog_new ();
    GSList *statuslist_names = NULL;
    GSList *tmplist = p->statuslist;


    while (tmplist)
      {
	  GGaduStatusPrototype *sp = tmplist->data;

	  if ((!sp->receive_only) && (sp->status != GG_STATUS_NOT_AVAIL_DESCR) && (sp->status != GG_STATUS_NOT_AVAIL))
	      statuslist_names = g_slist_append (statuslist_names, sp->description);

	  if (sp->status == (gint) config_var_get (handler, "status"))
	      statuslist_names = g_slist_prepend (statuslist_names, sp->description);

	  tmplist = tmplist->next;
      }


    ggadu_dialog_set_title (d, _("Gadu-gadu plugin configuration"));
    ggadu_dialog_set_type (d, GGADU_DIALOG_CONFIG);
    ggadu_dialog_callback_signal (d, "update config");
    ggadu_dialog_add_entry (&(d->optlist), GGADU_GADU_GADU_CONFIG_ID, "GG#", VAR_INT, config_var_get (handler, "uin"),
			    VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_GADU_GADU_CONFIG_PASSWORD, _("Password"), VAR_STR,
			    config_var_get (handler, "password"), VAR_FLAG_PASSWORD);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_GADU_GADU_CONFIG_SERVER, _("Server (optional)"), VAR_STR,
			    config_var_get (handler, "server"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_GADU_GADU_CONFIG_PROXY,
			    _("Proxy server (optional)\n[user:pass@]host.com[:80]"), VAR_STR, config_var_get (handler,
													      "proxy"),
			    VAR_FLAG_NONE);

    to_utf8 ("ISO-8859-2", config_var_get (handler, "reason"), utf);

    ggadu_dialog_add_entry (&(d->optlist), GGADU_GADU_GADU_CONFIG_REASON, _("Default reason"), VAR_STR, utf,
			    VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_GADU_GADU_CONFIG_HISTORY, _("Log chats to history file"), VAR_BOOL,
			    config_var_get (handler, "log"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_GADU_GADU_CONFIG_AUTOCONNECT, _("Autoconnect on startup"), VAR_BOOL,
			    config_var_get (handler, "autoconnect"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_GADU_GADU_CONFIG_AUTOCONNECT_STATUS, _("Autoconnect status"), VAR_LIST,
			    statuslist_names, VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_GADU_GADU_CONFIG_FRIENDS_MASK, _("Available only for friends"),
			    VAR_BOOL, config_var_get (handler, "private"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_GADU_GADU_CONFIG_DCC, _("Enable DCC"), VAR_BOOL,
			    config_var_get (handler, "dcc"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_GADU_GADU_CONFIG_SOUND_APP_FILE, _("Sound file (app)"),
			    VAR_FILE_CHOOSER, config_var_get (handler, "sound_app_file"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_GADU_GADU_CONFIG_SOUND_CHAT_FILE, _("Sound file (chat)"),
			    VAR_FILE_CHOOSER, config_var_get (handler, "sound_chat_file"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_GADU_GADU_CONFIG_SOUND_MSG_FILE, _("Sound file (msg)"),
			    VAR_FILE_CHOOSER, config_var_get (handler, "sound_msg_file"), VAR_FLAG_NONE);

    signal_emit (GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");

    return NULL;
}

gpointer search_action (gpointer user_data)
{
    GGaduDialog *d = NULL;
    GList *gender_list = NULL;

    if (!connected)
      {
	  signal_emit (GGadu_PLUGIN_NAME, "gui show warning",
		       g_strdup (_("You have to be connected to perform searching!")), "main-gui");
	  return NULL;
      }

    gender_list = g_list_append (gender_list, NULL);
    gender_list = g_list_append (gender_list, _("female"));
    gender_list = g_list_append (gender_list, _("male"));

    d = ggadu_dialog_new ();

    ggadu_dialog_set_title (d, _("Gadu-Gadu search"));
    ggadu_dialog_callback_signal (d, "search");
    ggadu_dialog_add_entry (&(d->optlist), GGADU_SEARCH_FIRSTNAME, _("First name:"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_SEARCH_LASTNAME, _("Last name:"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_SEARCH_NICKNAME, _("Nick:"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_SEARCH_CITY, _("City:"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_SEARCH_BIRTHYEAR, _("Birthyear:"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_SEARCH_GENDER, _("Gender:"), VAR_LIST, gender_list, VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_SEARCH_ID, _("GG#"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_SEARCH_ACTIVE, _("Search only for active users"), VAR_BOOL, NULL,
			    VAR_FLAG_NONE);

    signal_emit (GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");

    g_list_free (gender_list);

    return NULL;
}

gchar *userlist_dump (GSList * list)
{
    gchar *dump = NULL;
    GSList *us = list;

    while (us)
      {
	  gchar *line = NULL;
	  GGaduContact *k = (GGaduContact *) us->data;

	  line =
	      g_strdup_printf ("%s;%s;%s;%s;%s;%s;%s\r\n", k->first_name, k->last_name, k->nick, k->nick, k->mobile,
			       k->group, k->id);

	  if (!dump)
	      dump = g_strdup (line);
	  else
	    {
		gchar *tmp = dump;
		dump = g_strjoin (NULL, dump, line, NULL);
		g_free (tmp);
	    }

	  g_free (line);
	  us = us->next;
      }

    print_debug ("userlist_dump\n");
    return dump;
}

gboolean user_exists (gchar * id)
{
    GSList *tmp = userlist;

    while (tmp)
      {
	  GGaduContact *k = tmp->data;
	  if (!ggadu_strcasecmp (k->id, id))
	    {
		return TRUE;
	    }
	  tmp = tmp->next;
      }
    return FALSE;
}

void import_userlist (gchar * list)
{
    gchar **all, **tmp;
    all = g_strsplit (list, "\r\n", 1000);
    tmp = all;

    while (*tmp)
      {
	  gchar *first_name, *last_name, *nick, *comment, *mobile, *group, *uin;
	  gchar **l = NULL;
	  GGaduContact *k;
	  print_debug ("pointer %p -> %p\n", tmp, *tmp);
	  l = g_strsplit (*tmp, ";", 12);
	  tmp++;

	  if (!l[0])
	      continue;

	  first_name = l[0];
	  last_name = l[1];
	  nick = l[2];
	  mobile = l[4];
	  group = l[5];
	  uin = l[6];
	  comment = l[7];

	  print_debug ("dupa %s %p %p\n", uin, uin, mobile);
	  if ((!uin) && (!mobile))
	      continue;

	  if (user_exists (uin))
	      continue;

	  k = g_new0 (GGaduContact, 1);

	  k->id = uin ? g_strdup (uin) : g_strdup ("");

	  k->first_name = g_strdup (first_name);
	  k->last_name = g_strdup (last_name);
	  k->nick = (!strlen (nick)) ? g_strconcat (first_name, " ", last_name, NULL) : g_strdup (nick);
	  k->comment = g_strdup (comment);
	  k->mobile = g_strdup (mobile);
	  k->group = g_strdup (group);
	  k->status = GG_STATUS_NOT_AVAIL;

	  userlist = g_slist_append (userlist, k);
	  ggadu_repo_add_value ("gadu-gadu", k->id, k, REPO_VALUE_CONTACT);

	  if (connected && session)
	      gg_add_notify (session, atoi (k->id));

	  g_strfreev (l);
	  print_debug ("dupa end %s\n", uin);
      }

    signal_emit (GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
    save_addressbook_file (userlist);

    g_strfreev (all);
    g_free (list);
}

gboolean handle_userlist (GIOChannel * source, GIOCondition condition, gpointer data)
{
    struct gg_http *h = data;

    if (!h)
	return FALSE;

    if ((condition & G_IO_ERR))
      {
	  print_debug ("Condition with error\n");
	  goto abort;
      }

    if (condition & G_IO_IN || condition & G_IO_OUT)
      {

	  if (h->callback (h) == -1 || h->state == GG_STATE_ERROR)
	    {
		print_debug ("error ocurred\n");
		goto abort;
	    }

	  if (h->state == GG_STATE_CONNECTING)
	    {
		return TRUE;
	    }

	  if (h->state == GG_STATE_READING_HEADER)
	    {
		g_io_add_watch (source, G_IO_IN | G_IO_ERR, handle_userlist, h);
		return FALSE;
	    }

	  if (h->state != GG_STATE_DONE)
	      return TRUE;

	  if (h->type == GG_SESSION_USERLIST_GET)
	    {
		if (h->data)
		  {
		      gchar *list;
		      to_utf8 ("CP1250", h->data, list);
		      import_userlist (list);
		      signal_emit (GGadu_PLUGIN_NAME, "gui show message",
				   g_strdup (_("Userlist has been imported succesfully!")), "main-gui");
		  }
		else
		    signal_emit (GGadu_PLUGIN_NAME, "gui show warning", g_strdup (_("Server returned empty userlist!")),
				 "main-gui");

	    }
	  else if (h->type == GG_SESSION_USERLIST_PUT)
	    {
		if (!h->user_data)
		  {
		      if (h->data)
			  signal_emit (GGadu_PLUGIN_NAME, "gui show message",
				       g_strdup (_("Userlist export succeeded!")), "main-gui");
		      else
			  signal_emit (GGadu_PLUGIN_NAME, "gui show warning", g_strdup (_("Userlist export failed!")),
				       "main-gui");
		  }
		else
		  {
		      if (h->data)
			  signal_emit (GGadu_PLUGIN_NAME, "gui show message",
				       g_strdup (_("Userlist delete succeeded!")), "main-gui");
		      else
			  signal_emit (GGadu_PLUGIN_NAME, "gui show warning", g_strdup (_("Userlist delete failed!")),
				       "main-gui");
		  }
	    }
      }

  abort:
    print_debug ("shutting down channel\n");
    g_io_channel_shutdown (source, FALSE, NULL);
    g_io_channel_unref (source);

    h->destroy (h);

    return FALSE;
}

gpointer import_userlist_action (gpointer user_data)
{
    struct gg_http *h;
    GIOChannel *chan;

    if (!(h = gg_userlist_get ((gint) config_var_get (handler, "uin"), config_var_get (handler, "password"), 1)))
      {
	  print_debug ("userlist get error!\n");
	  return NULL;
      }

    chan = g_io_channel_unix_new (h->fd);
    g_io_add_watch (chan, G_IO_IN | G_IO_OUT | G_IO_ERR, handle_userlist, h);

    return NULL;
}

gpointer export_userlist_action (gpointer user_data)
{
    struct gg_http *h;
    gchar *dump, *tmp = userlist_dump (userlist);
    GIOChannel *chan;

    from_utf8 ("CP1250", tmp, dump);
    g_free (tmp);

    if (!(h = gg_userlist_put ((gint) config_var_get (handler, "uin"), config_var_get (handler, "password"), dump, 1)))
      {
	  print_debug ("userlist put error!\n");
	  return NULL;
      }

    chan = g_io_channel_unix_new (h->fd);
    g_io_add_watch (chan, G_IO_IN | G_IO_OUT | G_IO_ERR, handle_userlist, h);
    g_free (dump);

    return NULL;
}

gpointer delete_userlist_action (gpointer user_data)
{
    struct gg_http *h;
    GIOChannel *chan;
    gchar *dump = g_strdup ("");

    if (!(h = gg_userlist_put ((gint) config_var_get (handler, "uin"), config_var_get (handler, "password"), dump, 1)))
      {
	  print_debug ("userlist put error!\n");
	  return NULL;
      }

    h->user_data = (gchar *) 2;

    chan = g_io_channel_unix_new (h->fd);
    g_io_add_watch (chan, G_IO_IN | G_IO_OUT | G_IO_ERR, handle_userlist, h);
    g_free (dump);

    return NULL;
}

gboolean test_chan_dcc_get (GIOChannel * source, GIOCondition condition, gpointer data)
{
    struct gg_event *e = NULL;
    struct gg_dcc *d = data;

    if (!(gboolean) config_var_get (handler, "dcc"))
      {
	  gadu_gadu_enable_dcc_socket (FALSE);
	  return FALSE;
      }


    if (!(e = gg_dcc_watch_fd (d)))
      {
	  if (d->type != GG_SESSION_DCC_SOCKET)
	    {
		gg_free_dcc (d);
		print_debug ("qylazimy staq albercik\n");
		return FALSE;
	    }
      }

    switch (e->type)
      {

      case GG_EVENT_DCC_NEW:
	  {
	      GIOChannel *ch = NULL;
	      struct gg_dcc *dcc_new = e->event.dcc_new;

	      print_debug ("GG_EVENT_DCC_NEW %ld\n", (guint32) d->uin);
	      ch = g_io_channel_unix_new (dcc_new->fd);
	      watch_dcc_file = g_io_add_watch (ch, G_IO_ERR | G_IO_IN | G_IO_OUT, test_chan_dcc, dcc_new);
	  }
	  e->event.dcc_new = NULL;
	  gg_event_free (e);
	  break;

      case GG_EVENT_DCC_ERROR:
	  print_debug ("GG_EVENT_DCC_ERROR\n");

	  switch (e->event.dcc_error)
	    {
	    case GG_ERROR_DCC_HANDSHAKE:
		print_debug ("dcc_error_handshake\n");
		break;
	    case GG_ERROR_DCC_NET:
		print_debug ("dcc_error_network\n");
		break;
	    case GG_ERROR_DCC_REFUSED:
		print_debug ("dcc_error_refused\n");
		break;
	    default:
		print_debug ("dcc_error_unknown\n");
		break;
	    }

	  gg_event_free (e);
	  /* gg_dcc_free (d);              WHYYYYYYYYYYYYYYY
	     return FALSE; */
	  break;
      }

    if (d->check == GG_CHECK_READ)
      {
	  print_debug ("GG_CHECK_READ DCC_GET\n");
	  watch_dcc_file = g_io_add_watch (source, G_IO_IN | G_IO_ERR, test_chan_dcc_get, d);
	  return FALSE;
      }

    if (d->check == GG_CHECK_WRITE)
      {
	  print_debug ("GG_CHECK_WRITE DCC_GET\n");
	  watch_dcc_file = g_io_add_watch (source, G_IO_OUT | G_IO_ERR, test_chan_dcc_get, d);
	  return FALSE;
      }

    return TRUE;
}


gboolean test_chan_dcc (GIOChannel * source, GIOCondition condition, gpointer data)
{
    struct gg_event *e = NULL;
    struct gg_dcc *d = data;
    static gint prev_check = -1;

    if (!(gboolean) config_var_get (handler, "dcc"))
      {
	  gg_free_dcc (d);
	  return FALSE;
      }


    if (!(e = gg_dcc_watch_fd (d)))
      {
	  if (d->type != GG_SESSION_DCC_SOCKET)
	    {
		gg_free_dcc (d);
		print_debug ("qylazimy staq albercik\n");
		return FALSE;
	    }
      }

    switch (e->type)
      {
      case GG_EVENT_DCC_CLIENT_ACCEPT:
	  print_debug ("GG_EVENT_DCC_CLIENT_ACCEPT %ld %ld %ld\n", (guint32) d->uin, (guint32) d->peer_uin,
		       config_var_get (handler, "uin"));
	  gg_event_free (e);
	  break;

      case GG_EVENT_DCC_CALLBACK:
	  print_debug ("GG_EVENT_DCC_CALLBACK\n");
	  gg_dcc_set_type (d, GG_SESSION_DCC_SEND);
	  gg_event_free (e);
	  break;

      case GG_EVENT_DCC_NEED_FILE_INFO:
	  print_debug ("GG_EVENT_DCC_NEED_FILE_INFO\n");
	  gg_dcc_fill_file_info (d, dcc_send_request_filename);
	  gg_event_free (e);
	  break;

      case GG_EVENT_DCC_NEED_FILE_ACK:	/* dotyczy odbierania plikow */
	  {
	      /* struct gg_file_info file_info = d->file_info; */
	      gchar *dest_path = NULL;
	      gchar *p = NULL;

	      print_debug ("GG_EVENT_DCC_NEED_FILE_ACK");
	      //gg_dcc_set_type (d, GG_SESSION_DCC_GET);

	      for (p = d->file_info.filename; *p; p++)
		  if (*p < 32 || *p == '\\' || *p == '/')
		      *p = '_';

	      if (d->file_info.filename[0] == '.')
		  d->file_info.filename[0] = '_';

	      dest_path = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S, d->file_info.filename, NULL);
	      d->file_fd = open (dest_path, O_WRONLY | O_CREAT, 0600);
	      print_debug ("GG_EVENT_DCC_NEED_FILE_ACK %s\n", dest_path);


	  }
	  gg_event_free (e);
	  break;

      case GG_EVENT_DCC_DONE:
	  {
	      gint state = d->state;
	      gchar *filename = g_strdup (d->file_info.filename);

	      print_debug ("GG_EVENT_DCC_DONE\n");
	      gg_event_free (e);
	      gg_dcc_free (d);

	      if (state == GG_STATE_GETTING_FILE)
		{
		    signal_emit (GGadu_PLUGIN_NAME, "gui show message",
				 g_strdup_printf (_("File %s received succesful"), filename), "main-gui");
		    g_free (filename);
		}
	      else
		  signal_emit (GGadu_PLUGIN_NAME, "gui show message", g_strdup (_("File sent succesful")), "main-gui");

	      return FALSE;
	  }
	  break;

      case GG_EVENT_DCC_ERROR:
	  print_debug ("GG_EVENT_DCC_ERROR\n");

	  switch (e->event.dcc_error)
	    {
	    case GG_ERROR_DCC_HANDSHAKE:
		print_debug ("dcc_error_handshake\n");
		if (d->state == GG_STATE_READING_FILE_ACK)
		  {
		      signal_emit (GGadu_PLUGIN_NAME, "gui show message", g_strdup (_("File refused")), "main-gui");
		      gg_event_free (e);
		      gg_dcc_free (d);
		      return FALSE;
		  }
		break;
	    case GG_ERROR_DCC_NET:
		print_debug ("dcc_error_network\n");
		break;
	    case GG_ERROR_DCC_REFUSED:
		print_debug ("dcc_error_refused\n");
		signal_emit (GGadu_PLUGIN_NAME, "gui show message", g_strdup (_("File refused")), "main-gui");
		break;
	    default:
		print_debug ("dcc_error_unknown\n");
		break;
	    }

	  gg_event_free (e);
	  /* gg_dcc_free (d);           WHYYYYYYYYYYY
	     return FALSE; */
	  break;
      }

    if (prev_check != d->check)
      {
	  prev_check = d->check;

	  if (d->check == GG_CHECK_READ)
	    {
		print_debug ("GG_CHECK_READ DCC\n");
		watch_dcc_file = g_io_add_watch (source, G_IO_IN | G_IO_ERR, test_chan_dcc, d);
		return FALSE;
	    }

	  if (d->check == GG_CHECK_WRITE)
	    {
		print_debug ("GG_CHECK_WRITE DCC\n");
		watch_dcc_file = g_io_add_watch (source, G_IO_OUT | G_IO_ERR, test_chan_dcc, d);
		return FALSE;
	    }
      }

    return TRUE;
}

gpointer send_file_action (gpointer user_data)
{
    GSList *users = (GSList *) user_data;
    GGaduContact *k = (GGaduContact *) users->data;
    GGaduDialog *d = NULL;
    gint port;
    gchar **addr_arr = NULL;

    if (!connected || !k->ip || g_str_has_prefix (k->ip, "0.0.0.0"))
      {
	  signal_emit (GGadu_PLUGIN_NAME, "gui show warning", g_strdup (_("You cannot send file to this person")),
		       "main-gui");
	  return NULL;
      }

    addr_arr = g_strsplit (k->ip, ":", 2);

    if (!addr_arr[0] || !addr_arr[1])
	return NULL;

    port = atoi (addr_arr[1]);

    if (port < 1)
      {
	  g_free (addr_arr[0]);
	  g_free (addr_arr[1]);
	  signal_emit (GGadu_PLUGIN_NAME, "gui show warning", g_strdup (_("You cannot send file to this person")),
		       "main-gui");
	  return NULL;
      }

    d = ggadu_dialog_new ();

    ggadu_dialog_set_title (d, g_strdup_printf (_("Sending File to %s"), k->ip));
    ggadu_dialog_callback_signal (d, "send file");
    ggadu_dialog_add_entry (&(d->optlist), GGADU_GADU_GADU_CONTACT, NULL, VAR_NULL, k, VAR_FLAG_NONE);
    ggadu_dialog_add_entry (&(d->optlist), GGADU_GADU_GADU_SELECTED_FILE, _("Select File :"), VAR_FILE_CHOOSER, NULL,
			    VAR_FLAG_NONE);

    d->user_data = (gpointer) atoi ((char *) k->id);

    signal_emit (GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");

    return NULL;
}


GGaduMenu *build_plugin_menu ()
{
    GGaduMenu *root = ggadu_menu_create ();
    GGaduMenu *item_gg = ggadu_menu_add_item (root, "Gadu-Gadu", NULL, NULL);

    ggadu_menu_add_submenu (item_gg, ggadu_menu_new_item (_("Add Contact"), user_add_user_action, NULL));
    ggadu_menu_add_submenu (item_gg, ggadu_menu_new_item (_("Preferences"), user_preferences_action, NULL));
    ggadu_menu_add_submenu (item_gg, ggadu_menu_new_item (_("Search for friends"), search_action, NULL));
    ggadu_menu_add_submenu (item_gg, ggadu_menu_new_item ("", NULL, NULL));
    ggadu_menu_add_submenu (item_gg, ggadu_menu_new_item (_("Import userlist"), import_userlist_action, NULL));
    ggadu_menu_add_submenu (item_gg, ggadu_menu_new_item (_("Export userlist"), export_userlist_action, NULL));
    ggadu_menu_add_submenu (item_gg, ggadu_menu_new_item (_("Delete userlist"), delete_userlist_action, NULL));

    return root;
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 						PLUGIN STUFF
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
static void handle_sighup ()
{
    if (session && session->state != GG_STATE_IDLE)
      {
	  print_debug ("disconected");
	  gg_free_session (session);
	  session = NULL;
      }

    signal (SIGHUP, handle_sighup);
}

GGaduPlugin *initialize_plugin (gpointer conf_ptr)
{
    gchar *path = NULL;

    print_debug ("%s : initialize\n", GGadu_PLUGIN_NAME);

    signal (SIGHUP, handle_sighup);

#if GGADU_DEBUG
    gg_debug_level = 255;
#endif

    GGadu_PLUGIN_ACTIVATE (conf_ptr);	/* wazne zeby wywolac to makro w tym miejscu */

    handler = (GGaduPlugin *) register_plugin (GGadu_PLUGIN_NAME, _("Gadu-Gadu(c) protocol"));

    config_var_add (handler, "uin", VAR_INT);
    config_var_add (handler, "password", VAR_STR);
    config_var_add (handler, "proxy", VAR_STR);
    config_var_add (handler, "server", VAR_STR);
    config_var_add (handler, "sound_msg_file", VAR_STR);
    config_var_add (handler, "sound_chat_file", VAR_STR);
    config_var_add (handler, "sound_app_file", VAR_STR);
    config_var_add (handler, "log", VAR_BOOL);
    config_var_add (handler, "autoconnect", VAR_BOOL);
    config_var_add (handler, "status", VAR_INT);
    config_var_add (handler, "reason", VAR_STR);
    config_var_add (handler, "private", VAR_BOOL);
    config_var_add (handler, "dcc", VAR_BOOL);


    if (g_getenv ("CONFIG_DIR"))
	this_configdir = g_build_filename (g_get_home_dir (), g_getenv ("CONFIG_DIR"), "gg", NULL);
    else
	this_configdir = g_build_filename (g_get_home_dir (), ".gg", NULL);

#if GGADU_DEBUG
    path = g_build_filename (this_configdir, "config_test", NULL);

    mkdir (this_configdir, 0700);

    set_config_file_name ((GGaduPlugin *) handler, path);

    if (!config_read (handler))
      {
	  g_free (path);
#endif
	  path = g_build_filename (this_configdir, "config", NULL);

	  set_config_file_name ((GGaduPlugin *) handler, path);

	  if (!config_read (handler))
	      g_warning (_("Unable to read configuration file for plugin %s"), "gadu-gadu");
#if GGADU_DEBUG
      }
#endif

    register_signal_receiver ((GGaduPlugin *) handler, (signal_func_ptr) my_signal_receive);

    ggadu_repo_add ("gadu-gadu");

    return handler;
}

GSList *status_init ()
{
    GSList *list = NULL;
    GGaduStatusPrototype *sp;

    sp = g_new0 (GGaduStatusPrototype, 9);

    sp->status = GG_STATUS_AVAIL;
    sp->description = g_strdup (_("Available"));
    sp->image = g_strdup ("gadu-gadu-online.png");
    sp->receive_only = FALSE;
    list = g_slist_append (list, sp);
    sp++;

    sp->status = GG_STATUS_AVAIL_DESCR;
    sp->description = g_strdup (_("Available with description"));
    sp->image = g_strdup ("gadu-gadu-online-descr.png");
    sp->receive_only = FALSE;
    list = g_slist_append (list, sp);
    sp++;

    sp->status = GG_STATUS_BUSY;
    sp->description = g_strdup (_("Busy"));
    sp->image = g_strdup ("gadu-gadu-away.png");
    sp->receive_only = FALSE;
    list = g_slist_append (list, sp);
    sp++;

    sp->status = GG_STATUS_BUSY_DESCR;
    sp->description = g_strdup (_("Busy with description"));
    sp->image = g_strdup ("gadu-gadu-away-descr.png");
    sp->receive_only = FALSE;
    list = g_slist_append (list, sp);
    sp++;

    sp->status = GG_STATUS_INVISIBLE;
    sp->description = g_strdup (_("Invisible"));
    sp->image = g_strdup ("gadu-gadu-invisible.png");
    sp->receive_only = FALSE;
    list = g_slist_append (list, sp);
    sp++;

    sp->status = GG_STATUS_INVISIBLE_DESCR;
    sp->description = g_strdup (_("Invisible with description"));
    sp->image = g_strdup ("gadu-gadu-invisible-descr.png");
    sp->receive_only = FALSE;
    list = g_slist_append (list, sp);
    sp++;

    sp->status = GG_STATUS_NOT_AVAIL_DESCR;
    sp->description = g_strdup (_("Offline with description"));
    sp->image = g_strdup ("gadu-gadu-offline-descr.png");
    sp->receive_only = FALSE;
    list = g_slist_append (list, sp);
    sp++;

    sp->status = GG_STATUS_NOT_AVAIL;
    sp->description = g_strdup (_("Offline"));
    sp->image = g_strdup ("gadu-gadu-offline.png");
    sp->receive_only = FALSE;
    list = g_slist_append (list, sp);
    sp++;

    sp->status = GG_STATUS_BLOCKED;
    sp->description = g_strdup (_("Blocked"));
    sp->image = g_strdup ("gadu-gadu-invisible.png");
    sp->receive_only = TRUE;
    list = g_slist_append (list, sp);
    sp++;

    return list;
}

void start_plugin ()
{

    print_debug ("%s : start_plugin\n", GGadu_PLUGIN_NAME);

    p = g_new0 (GGaduProtocol, 1);
    p->display_name = g_strdup ("Gadu-Gadu");
    p->img_filename = g_strdup ("gadu-gadu.png");
    p->statuslist = status_init ();
    p->offline_status = g_slist_append (p->offline_status, (gint *) GG_STATUS_NOT_AVAIL);
    p->offline_status = g_slist_append (p->offline_status, (gint *) GG_STATUS_NOT_AVAIL_DESCR);
    p->away_status = g_slist_append (p->away_status, (gint *) GG_STATUS_BUSY);
    p->online_status = g_slist_append (p->online_status, (gint *) GG_STATUS_AVAIL);
    p->online_status = g_slist_append (p->online_status, (gint *) GG_STATUS_AVAIL_DESCR);

    ggadu_repo_add_value ("_protocols_", p->display_name, p, REPO_VALUE_PROTOCOL);

    signal_emit (GGadu_PLUGIN_NAME, "gui register protocol", p, "main-gui");

	/** try to register menu for this plugin in GUI **/
    menu_pluginmenu = build_plugin_menu ();

    signal_emit (GGadu_PLUGIN_NAME, "gui register menu", menu_pluginmenu, "main-gui");

    CHANGE_STATUS_SIG = register_signal (handler, "change status");
    CHANGE_STATUS_DESCR_SIG = register_signal (handler, "change status descr");
    SEND_MESSAGE_SIG = register_signal (handler, "send message");
    ADD_USER_SIG = register_signal (handler, "add user");
    CHANGE_USER_SIG = register_signal (handler, "change user");
    UPDATE_CONFIG_SIG = register_signal (handler, "update config");
    SEARCH_SIG = register_signal (handler, "search");
    EXIT_SIG = register_signal (handler, "exit");
    ADD_USER_SEARCH_SIG = register_signal (handler, "add user search");
    GET_CURRENT_STATUS_SIG = register_signal (handler, "get current status");
    SEND_FILE_SIG = register_signal (handler, "send file");
    GET_USER_MENU_SIG = register_signal (handler, "get user menu");

    load_contacts ("ISO-8859-2");

    signal_emit (GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");

    test ();

    if (config_var_get (handler, "autoconnect") && !connected)
	gadu_gadu_login ((config_var_check (handler, "reason")) ? config_var_get (handler, "reason") : _("no reason"),
			 (config_var_check (handler, "status")) ? (gint) config_var_get (handler,
											 "status") : GG_STATUS_AVAIL);

}

void my_signal_receive (gpointer name, gpointer signal_ptr)
{
    GGaduSignal *signal = (GGaduSignal *) signal_ptr;

    print_debug ("%s : receive signal %d %s\n", GGadu_PLUGIN_NAME, signal->name, g_quark_to_string (signal->name));

    if (signal->name == EXIT_SIG)
      {
	  /* exit_signal_handler(); */
	  return;
      }
      
    if (signal->name == GET_USER_MENU_SIG)
      {
	  GGaduMenu *umenu = ggadu_menu_create ();
	  GGaduMenu *listmenu = NULL;
	  GGaduPluginExtension *ext = NULL;

	  ggadu_menu_add_submenu (umenu, ggadu_menu_new_item (_("Chat"), user_chat_action, NULL));
	  
	  if ((ext = ggadu_find_extension(handler,GGADU_PLUGIN_EXTENSION_USER_MENU_TYPE)))
	  	ggadu_menu_add_submenu (umenu, ggadu_menu_new_item ((gpointer)ext->txt, ext->callback, NULL));
	  
	  ggadu_menu_add_submenu (umenu, ggadu_menu_new_item (_("View History"), user_view_history_action, NULL));

	  if ((gboolean) config_var_get (handler, "dcc"))
	      ggadu_menu_add_submenu (umenu, ggadu_menu_new_item (_("Send File"), send_file_action, NULL));

	  listmenu = ggadu_menu_new_item (_("Contact"), NULL, NULL);
	  ggadu_menu_add_submenu (listmenu, ggadu_menu_new_item (_("Add"), user_add_user_action, NULL));
	  ggadu_menu_add_submenu (listmenu, ggadu_menu_new_item (_("Remove"), user_remove_user_action, NULL));
	  ggadu_menu_add_submenu (listmenu, ggadu_menu_new_item (_("Info"), user_change_user_action, NULL));

	  ggadu_menu_add_submenu (umenu, listmenu);

	  ggadu_menu_print (umenu, NULL);

	  signal->data_return = umenu;
      }

    if (signal->name == SEND_FILE_SIG)
      {
	  GGaduDialog *d = signal->data;
	  GSList *tmplist = d->optlist;

	  gint uin = (gint) config_var_get (handler, "uin");
	  struct gg_dcc *dcc_file_session = NULL;
	  gchar **addr_arr = NULL;
	  gint port = 0;
	  guint32 ip = 0;
	  gchar *filename = NULL;
	  GGaduContact *k = NULL;
	  GIOChannel *dcc_channel_file = NULL;

	  if (d->response != GGADU_OK)
	      return;


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

	  if (!filename || strlen (filename) <= 0)
	      return;

	  addr_arr = g_strsplit (k->ip, ":", 2);

	  if (!addr_arr[0] || !addr_arr[1])
	      return;

	  ip = inet_addr (addr_arr[0]);
	  port = atoi (addr_arr[1]);

	  print_debug ("SEND TO IP : %s %d\n", addr_arr[0], port);

	  if (port <= 10)
	    {
		gg_dcc_request (session, (guint32) d->user_data);
		dcc_send_request_filename = filename;

	    }
	  else
	    {

		dcc_file_session = gg_dcc_send_file (ip, port, uin, atoi (k->id));

		if (!dcc_file_session)
		    return;

		gg_dcc_fill_file_info (dcc_file_session, filename);

		dcc_channel_file = g_io_channel_unix_new (dcc_file_session->fd);
		watch_dcc_file =
		    g_io_add_watch (dcc_channel_file, G_IO_OUT | G_IO_IN | G_IO_ERR, test_chan_dcc, dcc_file_session);
	    }

	  g_free (addr_arr[0]);
	  g_free (addr_arr[1]);

	  return;
      }


    if (signal->name == ADD_USER_SIG)
      {
	  GGaduContact *k = g_new0 (GGaduContact, 1);
	  GSList *kvlist = (GSList *) signal->data;

	  while (kvlist)
	    {
		GGaduKeyValue *kv = (GGaduKeyValue *) kvlist->data;

		switch ((gint) kv->key)
		  {
		  case GGADU_ID:
		      k->id = kv->value;
		      break;

		  case GGADU_NICK:
		      k->nick = g_strdup ((gchar *) kv->value);
		      g_free (kv->value);
		      break;

		  case GGADU_FIRST_NAME:
		      k->first_name = g_strdup ((gchar *) kv->value);
		      g_free (kv->value);
		      break;

		  case GGADU_LAST_NAME:
		      k->last_name = g_strdup ((gchar *) kv->value);
		      g_free (kv->value);
		      break;

		  case GGADU_MOBILE:
		      k->mobile = g_strdup ((gchar *) kv->value);
		      g_free (kv->value);
		      break;
		  }

		g_free (kv->description);
		kvlist = kvlist->next;
	    }

	  /* initial status for added person */
	  k->status = GG_STATUS_NOT_AVAIL;

	  g_slist_free (kvlist);

	  userlist = g_slist_append (userlist, k);
	  ggadu_repo_add_value ("gadu-gadu", k->id, k, REPO_VALUE_CONTACT);
	  signal_emit (GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
	  save_addressbook_file (userlist);

	  if ((connected == TRUE) && (session != NULL) & (k != NULL) && (k->id != NULL))
	      gg_add_notify (session, atoi (k->id));

	  return;
      }

    if (signal->name == ADD_USER_SEARCH_SIG)
      {
	  GGaduContact *k = signal->data;

	  /* initial status for added person */
	  k->status = GG_STATUS_NOT_AVAIL;

	  userlist = g_slist_append (userlist, k);
	  ggadu_repo_add_value ("gadu-gadu", k->id, k, REPO_VALUE_CONTACT);
	  signal_emit (GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
	  save_addressbook_file (userlist);

	  if (connected && session)
	      gg_add_notify (session, atoi (k->id));

	  return;
      }


    if (signal->name == CHANGE_USER_SIG)
      {
	  GGaduContact *k = g_new0 (GGaduContact, 1);
	  GSList *kvlist = (GSList *) signal->data;
	  GSList *ulisttmp = userlist;

	  while (kvlist)
	    {
		GGaduKeyValue *kv = (GGaduKeyValue *) kvlist->data;

		switch ((gint) kv->key)
		  {
		  case GGADU_ID:
		      k->id = kv->value;
		      break;

		  case GGADU_NICK:
		      k->nick = g_strdup ((gchar *) kv->value);
		      g_free (kv->value);
		      break;

		  case GGADU_FIRST_NAME:
		      k->first_name = g_strdup ((gchar *) kv->value);
		      g_free (kv->value);
		      break;

		  case GGADU_LAST_NAME:
		      k->last_name = g_strdup ((gchar *) kv->value);
		      g_free (kv->value);
		      break;

		  case GGADU_MOBILE:
		      k->mobile = g_strdup ((gchar *) kv->value);
		      g_free (kv->value);
		      break;
		  }
		g_free (kv->description);
		kvlist = kvlist->next;
	    }

	  g_slist_free (kvlist);

	  /* odnajduje id do zmiany */
	  while (ulisttmp)
	    {
		GGaduContact *ktmp = (GGaduContact *) ulisttmp->data;

		if (!ggadu_strcasecmp (ktmp->id, k->id))
		  {
		      /* zmiana danych */
		      ggadu_repo_del_value ("gadu-gadu", ktmp->id);
		      ktmp->id = k->id;
		      ktmp->first_name = k->first_name;
		      ktmp->last_name = k->last_name;
		      ktmp->nick = k->nick;
		      ktmp->mobile = k->mobile;
		      ggadu_repo_add_value ("gadu-gadu", ktmp->id, ktmp, REPO_VALUE_CONTACT);
		      break;
		  }
		ulisttmp = ulisttmp->next;
	    }

	  save_addressbook_file (userlist);
	  signal_emit (GGadu_PLUGIN_NAME, "gui send userlist", userlist, "main-gui");
	  return;
      }


    if (signal->name == CHANGE_STATUS_SIG)
      {
	  GGaduStatusPrototype *sp = signal->data;

	  if (sp == NULL)
		return;

	  if (sp->status != GG_STATUS_NOT_AVAIL)
	    {
	      /* descriptions, call dialogbox */
	      if (sp->status == GG_STATUS_AVAIL_DESCR || sp->status == GG_STATUS_BUSY_DESCR ||
		  sp->status == GG_STATUS_NOT_AVAIL_DESCR || sp->status == GG_STATUS_INVISIBLE_DESCR)
		{
		    GGaduDialog *d = ggadu_dialog_new ();
		    gchar *reason_c = NULL;

		    to_utf8 ("ISO-8859-2", config_var_get (handler, "reason"), reason_c);

		    ggadu_dialog_set_title (d, _("Enter status description"));
		    ggadu_dialog_callback_signal (d, "change status descr");
		    ggadu_dialog_add_entry (&(d->optlist), 0, _("Description:"), VAR_STR, (gpointer) reason_c,
					    VAR_FLAG_FOCUS);

		    d->user_data = sp;
		    signal_emit (GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");
		}
	      else
		{
		    gint _status = sp->status;
		    if (config_var_get (handler, "private"))
			_status |= GG_STATUS_FRIENDS_MASK;

		    /* if not connected, login. else, just change status */
		    if (!connected)
		      {
			connect_count = 0;
			gadu_gadu_login (NULL, sp->status);
		      }
		    else if (gg_change_status (session, _status) == -1)
		      {
			  signal_emit (GGadu_PLUGIN_NAME, "gui show warning",
				       g_strdup (_("Unable to change status")), "main-gui");
			  print_debug ("zjebka podczas change_status %d\n", sp->status);
		      }
		    else
		      {
			  signal_emit (GGadu_PLUGIN_NAME, "gui status changed", (gpointer) sp->status,
				       "main-gui");
		      }
		}
	    }
	  else
	    {
	      /* nie czekamy az serwer powie good bye tylko sami sie rozlaczamy */
	      ggadu_gadu_gadu_disconnect ();
	    }

	  return;
      }

    if (signal->name == CHANGE_STATUS_DESCR_SIG)
      {
	  GGaduDialog *d = signal->data;
	  GGaduStatusPrototype *sp = d->user_data;

	  if (d->response == GGADU_OK && sp)
	    {
		      GGaduKeyValue *kv = NULL;
		      if (d->optlist)
			{
			    gchar *desc_utf = NULL, *desc_cp = NULL, *desc_iso = NULL;
			    gint _status = sp->status;

			    if (config_var_get (handler, "private"))
				_status |= GG_STATUS_FRIENDS_MASK;

			    kv = (GGaduKeyValue *) d->optlist->data;

			    print_debug (" %d %d\n ", GG_STATUS_INVISIBLE_DESCR, sp->status);

			    desc_utf = kv->value;
			    /* do sesji */
			    from_utf8 ("CP1250", desc_utf, desc_cp);
			    /* do konfiga */
			    from_utf8 ("ISO-8859-2", desc_utf, desc_iso);
			    config_var_set (handler, "reason", desc_iso);

			    if (!connected)
			    {
				connect_count = 0;
				gadu_gadu_login (desc_cp, _status);
			    }
			    else if (!gg_change_status_descr (session, _status, desc_cp))
				signal_emit (GGadu_PLUGIN_NAME, "gui status changed", (gpointer) sp->status,
					     "main-gui");

			    g_free (desc_cp);
			    g_free (desc_iso);

			    config_save(handler);
			}

		      if (sp->status == GG_STATUS_NOT_AVAIL_DESCR)
			{
			    ggadu_gadu_gadu_disconnect ();
			}
	    }
	  GGaduDialog_free (d);
	  return;
      }


    if (signal->name == UPDATE_CONFIG_SIG)
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
			case GGADU_GADU_GADU_CONFIG_ID:
			    print_debug ("changing var setting uin to %d\n", kv->value);
			    config_var_set (handler, "uin", kv->value);
			    break;
			case GGADU_GADU_GADU_CONFIG_PASSWORD:
			    print_debug ("changing var setting password to %s\n", kv->value);
			    config_var_set (handler, "password", kv->value);
			    break;
			case GGADU_GADU_GADU_CONFIG_SERVER:
			    print_debug ("changing var setting server to %s\n", kv->value);
			    config_var_set (handler, "server", kv->value);
			    break;
			case GGADU_GADU_GADU_CONFIG_PROXY:
			    print_debug ("changing var setting proxy to %s\n", kv->value);
			    config_var_set (handler, "proxy", kv->value);
			    break;
			case GGADU_GADU_GADU_CONFIG_SOUND_CHAT_FILE:
			    print_debug ("changing var setting sound_chat_file to %s\n", kv->value);
			    config_var_set (handler, "sound_chat_file", kv->value);
			    break;
			case GGADU_GADU_GADU_CONFIG_SOUND_MSG_FILE:
			    print_debug ("changing var setting sound_msg_file to %s\n", kv->value);
			    config_var_set (handler, "sound_msg_file", kv->value);
			    break;
			case GGADU_GADU_GADU_CONFIG_SOUND_APP_FILE:
			    print_debug ("changing var setting sound_app_file to %s\n", kv->value);
			    config_var_set (handler, "sound_app_file", kv->value);
			    break;
			case GGADU_GADU_GADU_CONFIG_HISTORY:
			    print_debug ("changing var setting log to %d\n", kv->value);
			    config_var_set (handler, "log", kv->value);
			    break;
			case GGADU_GADU_GADU_CONFIG_DCC:
			    print_debug ("changing var setting dcc to %d\n", kv->value);
			    config_var_set (handler, "dcc", kv->value);
			    gadu_gadu_enable_dcc_socket ((gboolean) config_var_get (handler, "dcc"));
			    break;
			case GGADU_GADU_GADU_CONFIG_AUTOCONNECT:
			    print_debug ("changing var setting autoconnect to %d\n", kv->value);
			    config_var_set (handler, "autoconnect", kv->value);
			    break;
			case GGADU_GADU_GADU_CONFIG_AUTOCONNECT_STATUS:
			    {
				/* change string description to int value depending on current locales so it cannot be hardcoded eh.. */
				GSList *statuslist_tmp = p->statuslist;
				gint val = -1;

				while (statuslist_tmp)
				  {
				      GGaduStatusPrototype *sp = (GGaduStatusPrototype *) statuslist_tmp->data;
				      if (!ggadu_strcasecmp (sp->description, kv->value))
					{
					    val = sp->status;
					}
				      statuslist_tmp = statuslist_tmp->next;
				  }

				print_debug ("changing var setting status to %d\n", val);
				config_var_set (handler, "status", (gpointer) val);
			    }
			    break;
			case GGADU_GADU_GADU_CONFIG_REASON:
			    {
				gchar *utf = NULL;
				print_debug ("changing derault reason %s\n", kv->value);
				from_utf8 ("ISO-8859-2", kv->value, utf);
				config_var_set (handler, "reason", utf);
			    }
			    break;
			case GGADU_GADU_GADU_CONFIG_FRIENDS_MASK:
			    print_debug ("changing var setting private to %d\n", kv->value);
			    config_var_set (handler, "private", kv->value);
			    break;
			}
		      tmplist = tmplist->next;
		  }
		config_save (handler);
	    }
	  GGaduDialog_free (d);
	  return;
      }

    if (signal->name == SEND_MESSAGE_SIG)
      {
	  GGaduMsg *msg = signal->data;

	  if (connected && msg)
	    {
		gchar *message = NULL;

		from_utf8 ("CP1250", msg->message, message);
		message = insert_cr (message);

		print_debug ("%s\n", message);

		msg->time = time (NULL);

		if ((msg->class == GGADU_CLASS_CONFERENCE) && (msg->recipients != NULL))
		  {
		      gint i = 0;
		      uin_t *recipients = g_malloc (g_slist_length (msg->recipients) * sizeof (uin_t));
		      GSList *tmp = msg->recipients;

		      while (tmp)
			{
			    if (tmp->data != NULL)	/* ? */
				recipients[i++] = atoi ((gchar *) tmp->data);
			    tmp = tmp->next;
			}

		      if (gg_send_message_confer
			  (session, GG_CLASS_CHAT, g_slist_length (msg->recipients), recipients, message) == -1)
			{
			    signal_emit (GGadu_PLUGIN_NAME, "gui show warning", g_strdup (_("Unable send message")),
					 "main-gui");
			    print_debug ("zjebka podczas send message %s\n", msg->id);
			}
		      else if (config_var_get (handler, "log"))
			{
			    GSList *tmp = msg->recipients;

			    while (tmp)
			      {
				  gchar *line =
				      g_strdup_printf (_(":: Me (%s)::\n%s\n\n"), get_timestamp (0), msg->message);
				  ggadu_gg_save_history ((gchar *) tmp->data, line);
				  g_free (line);
				  tmp = tmp->next;
			      }
			}
		  }
		else
		  {
		      gint class = GG_CLASS_MSG;

		      if (msg->class == GGADU_CLASS_CHAT)
			  class = GG_CLASS_CHAT;

		      if ((msg->id == NULL) || (gg_send_message (session, class, atoi (msg->id), message) == -1))
			{
			    signal_emit (GGadu_PLUGIN_NAME, "gui show warning", g_strdup (_("Unable send message")),
					 "main-gui");
			    print_debug ("zjebka podczas send message %s\n", msg->id);
			}
		      else if (config_var_get (handler, "log"))
			{
			    gchar *line = g_strdup_printf (_(":: Me (%s) ::\n%s\n\n"), get_timestamp (0), msg->message);
			    ggadu_gg_save_history (msg->id, line);
			    g_free (line);
			}
		  }

		g_free (message);
		/* i wiele wiele innych */
	    }
      }

    if (signal->name == SEARCH_SIG)
      {
	  GGaduDialog *d = signal->data;
	  GSList *tmplist = d->optlist;
	  gg_pubdir50_t req;

	  if ((d->response == GGADU_OK) || (d->response == GGADU_NONE))
	    {
		if (!(req = gg_pubdir50_new (GG_PUBDIR50_SEARCH)))
		  {
		      GGaduDialog_free (d);
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
				  gchar *first_name;
				  to_cp ("UTF-8", kv->value, first_name);
				  gg_pubdir50_add (req, GG_PUBDIR50_FIRSTNAME, first_name);
				  g_free (first_name);
			      }
			    break;
			case GGADU_SEARCH_LASTNAME:
			    if (kv->value && *(gchar *) kv->value)
			      {
				  gchar *last_name;
				  to_cp ("UTF-8", kv->value, last_name);
				  gg_pubdir50_add (req, GG_PUBDIR50_LASTNAME, last_name);
				  g_free (last_name);
			      }
			    break;
			case GGADU_SEARCH_NICKNAME:
			    if (kv->value && *(gchar *) kv->value)
			      {
				  gchar *nick_name;
				  to_cp ("UTF-8", kv->value, nick_name);
				  gg_pubdir50_add (req, GG_PUBDIR50_NICKNAME, nick_name);
				  g_free (nick_name);
			      }
			    break;
			case GGADU_SEARCH_CITY:
			    if (kv->value && *(gchar *) kv->value)
			      {
				  gchar *city;
				  to_cp ("UTF-8", kv->value, city);
				  gg_pubdir50_add (req, GG_PUBDIR50_CITY, city);
				  g_free (city);
			      }
			    break;
			case GGADU_SEARCH_BIRTHYEAR:
			    if (kv->value && *(gchar *) kv->value)
				gg_pubdir50_add (req, GG_PUBDIR50_BIRTHYEAR, kv->value);
			    break;
			case GGADU_SEARCH_GENDER:
			    if (kv->value)
			      {
				  if (!ggadu_strcasecmp (kv->value, _("female")))
				      gg_pubdir50_add (req, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_FEMALE);
				  if (!ggadu_strcasecmp (kv->value, _("male")))
				      gg_pubdir50_add (req, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_MALE);
			      };
			    break;
			case GGADU_SEARCH_ACTIVE:
			    if (kv->value)
				gg_pubdir50_add (req, GG_PUBDIR50_ACTIVE, GG_PUBDIR50_ACTIVE_TRUE);
			    break;
			case GGADU_SEARCH_ID:
			    {
				if (kv->value)
				    gg_pubdir50_add (req, GG_PUBDIR50_UIN, kv->value);
			    }
			    break;
			}
		      tmplist = tmplist->next;
		  }

		if (gg_pubdir50 (session, req) == -1)
		  {
		      signal_emit (GGadu_PLUGIN_NAME, "gui show warning", g_strdup (_("Error! Cannot perform search!")),
				   "main-gui");
		  }

		gg_pubdir50_free (req);
	    }
	  GGaduDialog_free (d);

	  return;
      }

    if (signal->name == GET_CURRENT_STATUS_SIG)
      {
          signal->data_return = (gpointer) GG_STATUS_NOT_AVAIL;
	  
	  if (session)
	      signal->data_return =  (gpointer) ((session->status & GG_STATUS_FRIENDS_MASK) ? session->status ^ GG_STATUS_FRIENDS_MASK : session->status);
	      
      }
}

void destroy_plugin ()
{
    config_save (handler);

    print_debug ("destroy_plugin %s\n", GGadu_PLUGIN_NAME);

    if (menu_pluginmenu)
      {
	  signal_emit (GGadu_PLUGIN_NAME, "gui unregister menu", menu_pluginmenu, "main-gui");
	  ggadu_menu_free (menu_pluginmenu);
      }

    ggadu_repo_del ("gadu-gadu");
    ggadu_repo_del_value ("_protocols_", p);

    signal_emit (GGadu_PLUGIN_NAME, "gui unregister protocol", p, "main-gui");

    g_slist_free (userlist);
    g_free (this_configdir);
}


void load_contacts (gchar * encoding)
{
    FILE *fp;
    GGaduContact *k = NULL;
    gchar *line;
    gchar *path;

    path = g_build_filename (this_configdir, "userlist", NULL);

    fp = fopen (path, "r");

    if (!fp)
      {
	  g_free (path);
	  g_warning (_("I still cannot open contacts files! Exiting..."));
	  return;
      }

    g_free (path);

    line = g_malloc0 (1024);

    while (fgets (line, 1023, fp))
      {
	  gchar *buf = NULL;
	  gchar **l;
	  gchar *first_name, *last_name, *nick, *nick2 /*, *comment */ , *mobile, *group, *uin;

	  if (line[0] == '#' || !ggadu_strcasecmp (g_strstrip (line), ""))
	      continue;

	  to_utf8 (encoding, line, buf);

	  l = g_strsplit (buf, ";", 11);
	  if (!l[0])		/* ZONK */
	      continue;
	  first_name = l[0];
	  last_name = l[1];
	  nick = l[2];
	  nick2 = l[3];
	  mobile = l[4];
	  group = l[5];
	  uin = l[6];
	  /* comment = l[6]; */

	  if ((!uin || !*uin) && (!mobile || !*mobile))
	      continue;

	  k = g_new0 (GGaduContact, 1);

	  print_debug ("%s\n", uin);
	  k->id = uin ? g_strdup (uin) : g_strdup ("");
	  k->first_name = g_strdup (first_name);
	  k->last_name = g_strdup (last_name);

	  if (strlen (nick2) == 0)
	      k->nick = (strlen (nick) == 0) ? g_strconcat (first_name, " ", last_name, NULL) : g_strdup (nick);
	  else
	      k->nick = g_strdup (nick2);

	  /* k->comment = g_strdup (comment); */
	  k->mobile = g_strdup (mobile);
	  k->group = g_strdup (group);
	  k->status = GG_STATUS_NOT_AVAIL;

	  g_strfreev (l);

	  userlist = g_slist_append (userlist, k);
	  ggadu_repo_add_value ("gadu-gadu", k->id, k, REPO_VALUE_CONTACT);
      }

    g_free (line);
    fclose (fp);
}

void save_addressbook_file (gpointer userlist)
{
    GIOChannel *ch = NULL;
    gchar *path = NULL;

    path = g_build_filename (this_configdir, "userlist", NULL);
    print_debug ("path is %s\n", path);
    ch = g_io_channel_new_file (path, "w", NULL);

    if (ch)
      {
	  gchar *temp = userlist_dump (userlist);

	  if (g_io_channel_set_encoding (ch, "ISO-8859-2", NULL) != G_IO_STATUS_ERROR)
	    {
		if (temp != NULL)
		    g_io_channel_write_chars (ch, temp, -1, NULL, NULL);
	    }

	  g_free (temp);
	  g_io_channel_shutdown (ch, TRUE, NULL);
      }

    g_free (path);
}
