/* $Id: jabber_login.c,v 1.42 2004/10/13 10:51:29 krzyzak Exp $ */

/* 
 * Jabber plugin for GNU Gadu 2 
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

#include <string.h>
#include <stdlib.h>

#include "ggadu_conf.h"
#include "jabber_login.h"
#include "jabber_cb.h"
#include "jabber_protocol.h"

extern jabber_data_type jabber_data;

static LmSSLResponse jabber_connection_ssl_func (LmSSL *ssl, LmSSLStatus status, gpointer data)
 {
	 return LM_SSL_RESPONSE_CONTINUE;
 }

gpointer jabber_login_connect(gpointer status)
{
	static GStaticMutex connect_mutex = G_STATIC_MUTEX_INIT;
	gchar *jid = NULL;
	gchar *server = NULL;

	g_static_mutex_lock(&connect_mutex);
	
	if (!(jid = g_strdup(ggadu_config_var_get(jabber_handler, "jid"))))
	{
		g_warning("I want jid!");
		signal_emit_from_thread("jabber", "gui disconnected", NULL, "main-gui");
		signal_emit_from_thread("jabber", "gui show warning", g_strdup(_("Check you Jabber ID in properties!")), "main-gui");
		g_static_mutex_unlock(&connect_mutex);
		g_thread_exit(0);
		return NULL;
	}

	/* no 'server' variable, get server from JID */
	if (!(server = ggadu_config_var_get(jabber_handler, "server")))
	{
		/* look for '@' in JID */
		if (!(server = g_strstr_len(jid, strlen(jid), "@")))
		{
			signal_emit_from_thread("jabber", "gui disconnected", NULL, "main-gui");
			signal_emit_from_thread("jabber", "gui show warning", g_strdup(_("Invalid jid!")), "main-gui");
			g_free(jid);
			g_static_mutex_unlock(&connect_mutex);
			g_thread_exit(0);
			return NULL;
		}

		/* if '@' in JID was found, increase server ptr to point to hostname */
		server++;
	}

	if (!server || !*server)
	{
		signal_emit_from_thread("jabber", "gui disconnected", NULL, "main-gui");
		signal_emit_from_thread("jabber", "gui show warning", g_strdup(_("Invalid jid!")), "main-gui");
		g_free(jid);
		g_static_mutex_unlock(&connect_mutex);
		g_thread_exit(0);
		return NULL;
	}

	if (!jabber_data.connection || !lm_connection_is_open(jabber_data.connection))
	{
		print_debug("jabber: Connecting to %s with %s", server,jid);
		jabber_data.connection = lm_connection_new(server);
		lm_connection_set_keep_alive_rate(jabber_data.connection,30);

		/* proxy setting taken from EKG project */
		gint jabber_proxy_enabled = 0;
		gchar *jabber_proxy_host = NULL;
		gint jabber_proxy_port;

		if (ggadu_config_var_check(jabber_handler, "proxy"))
		{
			gchar **auth = array_make((gchar *) ggadu_config_var_get(jabber_handler, "proxy"), "@", 0, 0, 0);
			gchar **proxy_userpass = NULL;
			gchar **proxy_hostport = NULL;

			jabber_proxy_enabled = 1;

			if (auth[0] && auth[1])
			{
				proxy_userpass = array_make(auth[0], ":", 0, 0, 0);
				proxy_hostport = array_make(auth[1], ":", 0, 0, 0);
			}
			else
			{
				proxy_hostport = array_make(auth[0], ":", 0, 0, 0);
			}

			jabber_proxy_host = g_strdup(proxy_hostport[0]);
			jabber_proxy_port = proxy_hostport[1] ? atoi(proxy_hostport[1]) : 8080;

			array_free(proxy_hostport);
			array_free(proxy_userpass);
			array_free(auth);

			print_debug("proxy : %s %d", jabber_proxy_host, jabber_proxy_port);
		}
		
		if (jabber_proxy_enabled == 1) {
			jabber_data.proxy = lm_proxy_new_with_server (LM_PROXY_TYPE_HTTP,
				jabber_proxy_host, jabber_proxy_port);

			lm_connection_set_proxy(jabber_data.connection, jabber_data.proxy);
		}

	}
	else if (ggadu_strcasecmp(lm_connection_get_server(jabber_data.connection), server))
	{
		print_debug("jabber: Changing server to %s", server);
		lm_connection_close(jabber_data.connection, NULL);
		lm_connection_set_server(jabber_data.connection, server);
		lm_connection_set_port(jabber_data.connection,LM_CONNECTION_DEFAULT_PORT);
		/* lm_connection_unref(connection); */
		lm_proxy_unref(jabber_data.proxy);
	}

#ifdef BUILD_WITH_NEW_LOUDMOUTH	
	lm_connection_set_jid(jabber_data.connection, jid);
#endif

	if (ggadu_config_var_get(jabber_handler, "use_ssl"))
	{
		if (lm_ssl_is_supported())
		{
			LmSSL *ssl = lm_ssl_new (NULL,jabber_connection_ssl_func,NULL,NULL);
			lm_connection_set_port(jabber_data.connection,LM_CONNECTION_DEFAULT_PORT_SSL);
			lm_connection_set_ssl(jabber_data.connection,ssl);
			lm_ssl_unref(ssl);
		}
		else
		{
			signal_emit_from_thread("jabber", "gui disconnected", NULL, "main-gui");
			signal_emit_from_thread("jabber", "gui show warning",
						g_strdup(_("SSL not supported by loudmouth")), "main-gui");
		}
	}

	if (!iq_handler)
	{
		iq_handler = lm_message_handler_new(iq_cb, NULL, NULL);
		lm_connection_register_message_handler(jabber_data.connection, iq_handler, LM_MESSAGE_TYPE_IQ,
						       LM_HANDLER_PRIORITY_FIRST);
	}

	if (!iq_roster_handler)
	{
		iq_roster_handler = lm_message_handler_new(iq_roster_cb, NULL, NULL);
		lm_connection_register_message_handler(jabber_data.connection, iq_roster_handler, LM_MESSAGE_TYPE_IQ,
						       LM_HANDLER_PRIORITY_NORMAL);
	}

	if (!iq_version_handler)
	{
		iq_version_handler = lm_message_handler_new(iq_version_cb, NULL, NULL);
		lm_connection_register_message_handler(jabber_data.connection, iq_version_handler, LM_MESSAGE_TYPE_IQ,
						       LM_HANDLER_PRIORITY_NORMAL);
	}

	if(!iq_vcard_handler)
	{
                iq_vcard_handler = lm_message_handler_new(iq_vcard_cb, NULL, NULL);
		lm_connection_register_message_handler(jabber_data.connection, iq_vcard_handler, LM_MESSAGE_TYPE_IQ,
						       LM_HANDLER_PRIORITY_NORMAL);
	}


	if(!iq_account_data_handler)
	{
                iq_account_data_handler = lm_message_handler_new(iq_account_data_cb, NULL, NULL);
		lm_connection_register_message_handler(jabber_data.connection, iq_account_data_handler, LM_MESSAGE_TYPE_IQ,
						       LM_HANDLER_PRIORITY_NORMAL);
	}

	if (!presence_handler)
	{
		presence_handler = lm_message_handler_new(presence_cb, NULL, NULL);
		lm_connection_register_message_handler(jabber_data.connection, presence_handler, LM_MESSAGE_TYPE_PRESENCE,
						       LM_HANDLER_PRIORITY_NORMAL);
	}


	if (!message_handler)
	{
		message_handler = lm_message_handler_new(message_cb, NULL, NULL);
		lm_connection_register_message_handler(jabber_data.connection, message_handler, LM_MESSAGE_TYPE_MESSAGE,
						       LM_HANDLER_PRIORITY_NORMAL);
	}

	lm_connection_set_disconnect_function(jabber_data.connection, jabber_disconnect_cb, NULL, NULL);
	
	if (!lm_connection_open(jabber_data.connection, (LmResultFunction) connection_open_result_cb, (gint *) status, NULL, NULL))
	{
		signal_emit_from_thread("jabber", "gui disconnected", NULL, "main-gui");
		signal_emit_from_thread("jabber", "gui show warning", g_strdup(_("Unable to open connection")), "main-gui");
//		jabber_disconnect_cb(jabber_data.connection,LM_DISCONNECT_REASON_OK, NULL);
	}


	g_free(jid);
	g_static_mutex_unlock(&connect_mutex);
	g_thread_exit(0);

	return NULL;
}
