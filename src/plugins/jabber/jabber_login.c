#include <string.h>

#include "jabber_login.h"
#include "jabber_cb.h"
#include "jabber_protocol.h"

void jabber_login (enum states status)
{
  if (!connected && status == JABBER_STATUS_UNAVAILABLE)
    return;

  if (connected == 1)
    print_debug ("jabber: Slow down, please.\n");
  else if (connected && status == JABBER_STATUS_UNAVAILABLE)
  {
    GSList *list = rosterlist;

    jabber_status = JABBER_STATUS_UNAVAILABLE;
    lm_connection_close (connection, NULL);

    signal_emit ("jabber", "gui status changed", (gpointer) status, "main-gui");
    signal_emit ("jabber", "gui send userlist", NULL, "main-gui");

    while (list)
    {
      GGaduContact *k = (GGaduContact *) list->data;
      ggadu_repo_del_value ("jabber", k->id);
      g_free (k->id);
      g_free (k->nick);
      g_free (k);
      list = list->next;
    }
    g_slist_free (userlist);
    g_slist_free (rosterlist);
    userlist = rosterlist = NULL;
    connected = 0;
  } else if (connected == 2)
    jabber_change_status (status);
  else
  {
    /* we haven't connected yet */
    gchar *jid;
    gchar *server;
    
    if (!(jid = g_strdup (config_var_get (jabber_handler, "jid"))))
    {
      g_warning ("I want jid!");
      return;
    }
    
    server = strchr (jid, '@') + 1;
    
    print_debug ("jabber: Connection to %s.\n", server);

    connection = lm_connection_new (server);

    if (!iq_handler) iq_handler = lm_message_handler_new (iq_cb, NULL, NULL);
    if (!iq_roster_handler) iq_roster_handler = lm_message_handler_new (iq_roster_cb, NULL, NULL);
    if (!presence_handler) presence_handler = lm_message_handler_new (presence_cb, NULL, NULL);
    if (!message_handler) message_handler = lm_message_handler_new (message_cb, NULL, NULL);

    lm_connection_register_message_handler (connection, iq_roster_handler, LM_MESSAGE_TYPE_IQ,
	LM_HANDLER_PRIORITY_FIRST);
    lm_connection_register_message_handler (connection, iq_handler, LM_MESSAGE_TYPE_IQ,
	LM_HANDLER_PRIORITY_NORMAL);

    if (!lm_connection_open (connection,
	  (LmResultFunction) connection_open_result_cb, (gint *)status, g_free, NULL))
    {
      print_debug ("jabber: lm_connection_open() failed.\n");
    }
    g_free (jid);
  }
}
