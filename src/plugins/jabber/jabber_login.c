#include <string.h>

#include "ggadu_conf.h"
#include "jabber_login.h"
#include "jabber_cb.h"
#include "jabber_protocol.h"

extern jabber_data_type jabber_data;

void jabber_login (enum states status)
{
	if (!jabber_data.connected && status == JABBER_STATUS_UNAVAILABLE)
		return;

	if (jabber_data.connected == 1)
	{
		print_debug ("jabber: Slow down, please");
	} else if (jabber_data.connected && status == JABBER_STATUS_UNAVAILABLE) {
    
        GSList *list = jabber_data.userlist;

		jabber_data.status = status;
    
		if (lm_connection_close (connection, NULL)) {
            signal_emit ("jabber", "gui status changed", (gpointer) status, "main-gui");
            signal_emit ("jabber", "gui send userlist", NULL, "main-gui");
        }

		while (list)
		{
			GGaduContact *k = (GGaduContact *) list->data;
			ggadu_repo_del_value ("jabber", k->id);
			g_free (k->id);
			g_free (k->nick);
			g_free (k);
			list = list->next;
		}
    
		g_slist_free (jabber_data.userlist);
    
		jabber_data.userlist = NULL;
		jabber_data.connected = 0;
	}
	else if (jabber_data.connected == 2)
	{
		jabber_change_status (status);
	}
	else
	{
		/* we haven't connected yet */
		g_thread_create (jabber_login_connect, (gpointer) status, FALSE, NULL);
	}
}

gpointer jabber_login_connect (gpointer status)
{
	static GStaticMutex connect_mutex = G_STATIC_MUTEX_INIT;
	gchar *jid;
	gchar *server;

	g_static_mutex_lock (&connect_mutex);
	if (!(jid = g_strdup (ggadu_config_var_get (jabber_handler, "jid"))))
	{
		g_warning ("I want jid!");
		g_static_mutex_unlock (&connect_mutex);
		g_thread_exit (0);
		return NULL;
	}

	server = strchr (jid, '@');
	if (!server++)
	{
		signal_emit_from_thread ("jabber", "gui disconnected", NULL, "main-gui");
		signal_emit_from_thread ("jabber", "gui show warning", g_strdup (_("Invalid jid!")), "main-gui");
		g_static_mutex_unlock (&connect_mutex);
        g_free(jid);
		g_thread_exit (0);
		return NULL;
	}

	print_debug ("jabber: Connecting to %s", server);
    
    if (!connection)
        connection = lm_connection_new (server);

	if (ggadu_config_var_get (jabber_handler, "use_ssl"))
	{
		if (lm_connection_supports_ssl ())
		{
			lm_connection_set_use_ssl (connection, 1);
			lm_connection_set_port (connection, LM_CONNECTION_DEFAULT_PORT_SSL);
		}
		else
		{
			print_debug ("SSL not supported by loudmouth.\n");
			signal_emit_from_thread ("jabber", "gui disconnected", NULL, "main-gui");
            signal_emit_from_thread ("jabber", "gui show warning", g_strdup (_("SSL not supported by loudmouth")), "main-gui");
		}
	}

	if (!iq_handler) {
		iq_handler = lm_message_handler_new (iq_cb, NULL, NULL);
        lm_connection_register_message_handler (connection, iq_handler, LM_MESSAGE_TYPE_IQ, LM_HANDLER_PRIORITY_FIRST);
    }
	
    if (!iq_roster_handler) {
		iq_roster_handler = lm_message_handler_new (iq_roster_cb, NULL, NULL);
        lm_connection_register_message_handler (connection, iq_roster_handler, LM_MESSAGE_TYPE_IQ,
						LM_HANDLER_PRIORITY_NORMAL);
    }
    
	if (!iq_version_handler) {
		iq_version_handler = lm_message_handler_new (iq_version_cb, NULL, NULL);
        lm_connection_register_message_handler (connection, iq_version_handler, LM_MESSAGE_TYPE_IQ,
						LM_HANDLER_PRIORITY_NORMAL);
    }


	if (!presence_handler) {
		presence_handler = lm_message_handler_new (presence_cb, NULL, NULL);
        lm_connection_register_message_handler (connection, presence_handler, LM_MESSAGE_TYPE_PRESENCE,
							LM_HANDLER_PRIORITY_NORMAL);
    }
    
    
	if (!message_handler) {
		message_handler = lm_message_handler_new (message_cb, NULL, NULL);
		lm_connection_register_message_handler (connection, message_handler, LM_MESSAGE_TYPE_MESSAGE,
							LM_HANDLER_PRIORITY_NORMAL);
     }

    

	if (!lm_connection_open (connection, (LmResultFunction) connection_open_result_cb, (gint *) status, NULL, NULL))
	{
		print_debug ("jabber: lm_connection_open() failed.\n");
		signal_emit_from_thread ("jabber", "gui disconnected", NULL, "main-gui");
        signal_emit_from_thread ("jabber", "gui show warning", g_strdup (_("Connection failed")), "main-gui");
	}

	g_free (jid);
	g_static_mutex_unlock (&connect_mutex);
	g_thread_exit (0);

	return NULL;
}
