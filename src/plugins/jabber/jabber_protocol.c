#include <loudmouth/loudmouth.h>

#include "jabber_protocol.h"

void action_subscribe (LmConnection *connection, LmMessage *message, gpointer data)
{
  LmMessage *m;
  LmMessageNode *node;
  waiting_action *action;
  gboolean result;

  m = lm_message_new_with_sub_type (g_strdup (data), LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
  lm_message_node_set_attribute (m->node, "id", "subscribe");

  node = lm_message_node_add_child (m->node, "query", NULL);
  lm_message_node_set_attribute (node, "xmlns", "jabber:iq:roster");
  
  node = lm_message_node_add_child (node, "item", NULL);
  lm_message_node_set_attribute (node, "jid", data);

  action = g_new0 (waiting_action, 1);
  action->id   = g_strdup ("subscribe");
  action->type = g_strdup ("result");
  action->data = g_strdup (data);
  action->func = action_subscribe_result;
  actions = g_slist_append (actions, action);

  result = lm_connection_send (connection, m, NULL);
  lm_message_unref (m);
  if (!result)
  {
    actions = g_slist_remove (actions, action);
    g_free (action);
    print_debug ("jabber: Can't send.\n");
  }
}

void action_subscribe_result (LmConnection *connection, LmMessage *message, gpointer data)
{
  LmMessage *m;
  gboolean result;

  m = lm_message_new_with_sub_type (g_strdup (data), LM_MESSAGE_TYPE_PRESENCE, LM_MESSAGE_SUB_TYPE_SUBSCRIBE);
  lm_message_node_set_attribute (m->node, "id", "subscribe");

  lm_message_node_add_child (m->node, "status", "I would like to subscribe you to my roster.");
  
  result = lm_connection_send (connection, m, NULL);
  lm_message_unref (m);
  if (!result)
    print_debug ("jabber: Can't send.\n");
}

void jabber_change_status (enum states status)
{
  LmMessage *m;
  gboolean result;
  gchar *show = NULL;
  gchar *show_away = "away";
  gchar *show_xa   = "xa";
  gchar *show_dnd  = "dnd";
  gchar *show_chat = "chat";

  m = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_PRESENCE,
      status == JABBER_STATUS_UNAVAILABLE ?
      LM_MESSAGE_SUB_TYPE_UNAVAILABLE : LM_MESSAGE_SUB_TYPE_AVAILABLE);

  switch (status) {
    case JABBER_STATUS_AWAY:      show = show_away; break;
    case JABBER_STATUS_XA:        show = show_xa;   break;
    case JABBER_STATUS_DND:       show = show_dnd;  break;
    case JABBER_STATUS_CHAT:      show = show_chat; break;
    case JABBER_STATUS_AVAILABLE: show = NULL;      break;
    default:
      break;
  }

  if (show)
    lm_message_node_add_child (m->node, "show", show);
  if (status_descr)
    lm_message_node_add_child (m->node, "status", status_descr);
  
  print_debug ("STATUS - %d\n", status);
  
  result = lm_connection_send (connection, m, NULL);
  lm_message_unref (m);
  if (!result)
    print_debug ("jabber: Couldn't change status!\n");
  else {
    jabber_status = status;
    signal_emit ("jabber", "gui status changed", (gpointer) status, "main-gui");
  }
}

void jabber_fetch_roster (void)
{
  LmMessage *m;
  gboolean result;
  LmMessageNode *node;

  m = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);
  lm_message_node_set_attribute (m->node, "id", "roster_1");
  
  node = lm_message_node_add_child (m->node, "query", NULL);
  lm_message_node_set_attribute (node, "xmlns", "jabber:iq:roster");
  
  print_debug ("Sending:\n%s", lm_message_node_to_string (m->node));
  
  result = lm_connection_send (connection, m, NULL);
  lm_message_unref (m);

  if (!result)
    print_debug ("jabber: Can't fetch roster (lm_connection_send() failed).\n");
}
