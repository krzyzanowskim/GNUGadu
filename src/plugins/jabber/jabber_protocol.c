/* $Id: jabber_protocol.c,v 1.14 2004/01/07 23:49:17 thrulliq Exp $ */

#include <loudmouth/loudmouth.h>
#include <string.h>

#include "jabber_protocol.h"
#include "jabber_plugin.h"

extern jabber_data_type jabber_data;

waiting_action* action_queue_add (gchar *id, gchar *type, gpointer action_callback, gchar *data)
{
  waiting_action *action;
  action = g_new0 (waiting_action, 1);
  action->id   = g_strdup (id);
  action->type = g_strdup (type);
  action->func = action_callback;
  action->data = data ? g_strdup (data) : NULL;

  jabber_data.actions = g_slist_append (jabber_data.actions, action);
  return action;
}

void action_queue_del (waiting_action *action)
{
  jabber_data.actions = g_slist_remove (jabber_data.actions, action);
  g_free (action);
}

void action_roster_add_result (LmConnection *connection, LmMessage *message, gpointer id)
{
    LmMessage *m = lm_message_new_with_sub_type (id, LM_MESSAGE_TYPE_PRESENCE, LM_MESSAGE_SUB_TYPE_SUBSCRIBE);
    signal_emit ("jabber", "gui show message", g_strdup (_("Roster list updated")), "main-gui");
    lm_connection_send (connection, m, NULL);
    lm_message_unref(m);

}

void action_roster_remove_result (LmConnection *connection, LmMessage *message, gpointer data)
{
    gchar *id = (gchar *)data;

    jabber_data.userlist = ggadu_userlist_remove_id(jabber_data.userlist, id);
    ggadu_repo_del_value ("jabber", id);
    //GGaduContact_free(k);
    signal_emit ("jabber", "gui send userlist", jabber_data.userlist, "main-gui");
    signal_emit ("jabber", "gui show message", g_strdup (_("Contact removed from roster")), "main-gui");
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

  if ((status == jabber_data.status) && (!jabber_data.status_descr)) return;

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
  
  if (jabber_data.status_descr)
    lm_message_node_add_child (m->node, "status", jabber_data.status_descr);
  
  print_debug ("STATUS - %d\n", status);
  
  result = lm_connection_send (connection, m, NULL);
  lm_message_unref (m);
  if (!result)
    print_debug ("jabber: Couldn't change status!\n");
  else {
    jabber_data.status = status;
    signal_emit ("jabber", "gui status changed", (gpointer) status, "main-gui");
  }
}

void jabber_fetch_roster (void)
{
  LmMessage *m;
  LmMessageNode *node;

  m = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);
  node = lm_message_node_add_child (m->node, "query", NULL);
  lm_message_node_set_attribute (node, "xmlns", "jabber:iq:roster");

  if (!lm_connection_send (connection, m, NULL))
    print_debug ("jabber: Can't fetch roster (lm_connection_send() failed).\n");
  
  lm_message_unref (m);

  
}

void action_search_form (LmConnection *connection, LmMessage *message, gpointer data)
{
  GGaduDialog *d = ggadu_dialog_new ();
  LmMessageNode *node;
  LmMessageNode *child_first, *child_last, *child_nick, *child_email, *child_instr;

  ggadu_dialog_set_title (d, _("Jabber search: form"));
  ggadu_dialog_callback_signal (d, "search");
  d->user_data = (gpointer) g_strdup (lm_message_node_get_attribute (message->node, "from")); 
  
  node = lm_message_node_get_child (message->node, "query");
  if (!strcmp (lm_message_node_get_attribute (node, "xmlns"), "jabber:iq:search")) {
    child_first = lm_message_node_get_child (node, "first");
    child_last  = lm_message_node_get_child (node, "last");
    child_nick  = lm_message_node_get_child (node, "nick");
    child_email = lm_message_node_get_child (node, "email");
    child_instr = lm_message_node_get_child (node, "instructions");
    
    if (child_first)
      ggadu_dialog_add_entry (&(d->optlist), GGADU_SEARCH_FIRSTNAME, _("First name:"), VAR_STR, NULL, VAR_FLAG_NONE);
    if (child_last)
      ggadu_dialog_add_entry (&(d->optlist), GGADU_SEARCH_LASTNAME, _("Last name:"), VAR_STR, NULL, VAR_FLAG_NONE);
    if (child_nick)
      ggadu_dialog_add_entry (&(d->optlist), GGADU_SEARCH_NICKNAME, _("Nick:"), VAR_STR, NULL, VAR_FLAG_NONE);
    if (child_email)
      ggadu_dialog_add_entry (&(d->optlist), GGADU_SEARCH_EMAIL, _("Email:"), VAR_STR, NULL, VAR_FLAG_NONE);

    signal_emit ("jabber", "gui show dialog", d, "main-gui");
  }
}

void action_search_result (LmConnection *connection, LmMessage *message, gpointer data)
{
  LmMessageNode *node;
  LmMessageNode *child;
  GSList *list = NULL;

  node = lm_message_node_get_child (message->node, "query");
  if (!strcmp (lm_message_node_get_attribute (node, "xmlns"), "jabber:iq:search")) {
    child = lm_message_node_get_child (node, "item");
    if (!child) {
      signal_emit ("jabber", "gui show message", g_strdup (_("No users have been found!")), "main-gui");
      return;
    }

    while (child) {
      gchar *jid = (gchar *) lm_message_node_get_attribute (child, "jid");
      GGaduContact *k;
      LmMessageNode *child_first, *child_last, *child_nick, *child_email;

      k = g_new0 (GGaduContact, 1);
      k->id = g_strdup (jid ? jid : "?");

      child_first = lm_message_node_get_child (node, "first");
      child_last  = lm_message_node_get_child (node, "last");
      child_nick  = lm_message_node_get_child (node, "nick");
      child_email = lm_message_node_get_child (node, "email");
      
      if (child_first)
	k->first_name = g_strdup ((gchar *) lm_message_node_get_value (child_first));
      if (child_last)
	k->last_name  = g_strdup ((gchar *) lm_message_node_get_value (child_last));
      if (child_nick)
	k->nick       = g_strdup ((gchar *) lm_message_node_get_value (child_nick));
      if (child_email)
	k->email      = g_strdup ((gchar *) lm_message_node_get_value (child_email));
      
      k->status = JABBER_STATUS_UNAVAILABLE;
      list = g_slist_append (list, k);
      child = child->next;
    }
    
    signal_emit ("jabber", "gui show search results", list, "main-gui");
  }
}
