#include <string.h>

#include "jabber_cb.h"
#include "jabber_protocol.h"

void connection_auth_cb (LmConnection *connection, gboolean success, gint *status)
{
  if (!success)
  {
    print_debug ("jabber: Authentication failed.\n");
    return;
  }

  connected = 2;
  
  print_debug ("jabber: Authentication succeeded. Changing status...\n");
  
  jabber_change_status (*(gint *)&status);

  print_debug ("jabber: Fetching roster.\n");
  
  jabber_fetch_roster ();
}

void connection_open_result_cb (LmConnection *connection, gboolean success, gint *status)
{
  gchar *jid;
  
  if (!success)
  {
    print_debug ("jabber: Connection failed.\n");
    return;
  }

  connected = 1;
  print_debug ("jabber: Connection succeeded. Authenticating...\n");

  jid = g_strdup (config_var_get (jabber_handler, "jid"));
  strchr (jid, '@')[0] = '\0';
  
  if (!lm_connection_authenticate (connection, jid,
	config_var_get (jabber_handler, "password"), "GNU Gadu 2",
	(LmResultFunction) connection_auth_cb,
	status, NULL, NULL))
  {
    print_debug ("jabber: lm_connection_authenticate() failed.\n");
  }
  g_free (jid);
}

LmHandlerResult presence_cb (LmMessageHandler *handler, LmConnection *connection, LmMessage *message, gpointer user_data)
{
  gchar *jid;
  GSList *list = rosterlist;
  GGaduContact *k = NULL;
  gchar *descr;
  gchar *show;
  LmMessageNode *node;

  jid  = (gchar *) lm_message_node_get_attribute (message->node, "from"); 

  print_debug ("\n%s", lm_message_node_to_string (message->node));
  
  if (strchr (jid, '/'))
    strchr (jid, '/')[0] = '\0';

  if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_SUBSCRIBE) {
    GGaduDialog *d;
    gchar *msg;

    d = ggadu_dialog_new ();
    msg = g_strdup_printf (_("User : %s\nwants to subscribe your presence"), jid);
    ggadu_dialog_set_title (d, _("Confirmation"));
    ggadu_dialog_set_type (d, GGADU_DIALOG_YES_NO);
    ggadu_dialog_callback_signal (d, "jabber subscribe");
    ggadu_dialog_add_entry (&(d->optlist), 0, msg, VAR_NULL, NULL, VAR_FLAG_NONE);
    d->user_data = g_strdup (jid);
    signal_emit ("jabber", "gui show dialog", d, "main-gui");
    
    return LM_HANDLER_RESULT_REMOVE_MESSAGE;
  } else if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_SUBSCRIBED) {
    gchar *msg = g_strdup_printf (_("%s accepts your subscription"), jid);
    signal_emit ("jabber", "gui show message", msg, "main-gui");
    return LM_HANDLER_RESULT_REMOVE_MESSAGE;
  } else if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_UNSUBSCRIBE) {
    gchar *msg = g_strdup_printf (_("%s unsubscribed you from presence"), jid);
    signal_emit ("jabber", "gui show message", msg, "main-gui");
    return LM_HANDLER_RESULT_REMOVE_MESSAGE;
  } else if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_UNSUBSCRIBED) {
    gchar *msg = g_strdup_printf (_("%s won't be on your subscription list."), jid);
    signal_emit ("jabber", "gui show message", msg, "main-gui");
    return LM_HANDLER_RESULT_REMOVE_MESSAGE;
  }

  descr = (gchar *) lm_message_node_get_attribute (message->node, "status");

  while (list) {
    k = (GGaduContact *) list->data;
    if (!ggadu_strcasecmp (k->id, jid)) {
      gint oldstatus = k->status;

      switch (lm_message_get_sub_type (message)) {
	default:
	case LM_MESSAGE_SUB_TYPE_AVAILABLE:
	  node = lm_message_node_get_child (message->node, "show");
	  show = NULL;
	  if (node)
	    show = (gchar *) lm_message_node_get_value (node);
	  if (show) {
	    if (!strcmp (show, "away"))
	      k->status = JABBER_STATUS_AWAY;
	    else if (!strcmp (show, "xa"))
	      k->status = JABBER_STATUS_XA;
	    else if (!strcmp (show, "dnd"))
	      k->status = JABBER_STATUS_DND;
	    else
	      k->status = JABBER_STATUS_AVAILABLE;
	  } else
	    k->status = JABBER_STATUS_AVAILABLE;
	  if (descr)
	    k->status_descr = g_strdup (descr);
	  break;
	case LM_MESSAGE_SUB_TYPE_ERROR:
	case LM_MESSAGE_SUB_TYPE_UNAVAILABLE:
	  k->status = JABBER_STATUS_UNAVAILABLE;
	  if (descr)
	    k->status_descr = g_strdup (descr);
	  break;
      }
      
      if (k->status != oldstatus)
	ggadu_repo_change_value ("jabber", k->id, k, REPO_VALUE_DC);
    }
    list = list->next;
  }
  
  return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
}

LmHandlerResult iq_cb (LmMessageHandler *handler, LmConnection *connection, LmMessage *message, gpointer user_data)
{
  GSList *list = actions;
  waiting_action *action;
  gchar *type;
  gchar *id;
  
  id   = (gchar *) lm_message_node_get_attribute (message->node, "id");
  type = (gchar *) lm_message_node_get_attribute (message->node, "type");
  
  /* first we check if we have waiting actions for the message
   * if there is one, then run the associated function instead of the code
   * below. */
  
  while (list) {
    action = (waiting_action *) list->data;
    if (!strcmp (type, action->type) && !strcmp (id, action->id)) {
      action->func (connection, message, action->data);
      actions = g_slist_remove (actions, action);
      g_free (action->id);
      g_free (action->type);
      g_free (action);
      return LM_HANDLER_RESULT_REMOVE_MESSAGE;
    }
    list = list->next;
  }

  return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

LmHandlerResult iq_roster_cb (LmMessageHandler *handler, LmConnection *connection, LmMessage *message, gpointer data)
{
  LmMessageNode *node;
  LmMessageNode *child;
  int first_time = 0;

  if (!rosterlist)
    first_time = 1;

  print_debug ("\n%s", lm_message_node_to_string (message->node));
  if (!(node = lm_message_node_get_child (message->node, "query"))) {
    print_debug ("jabber: weird roster.\n%s", lm_message_node_to_string (message->node));
    return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
  }

  if (lm_message_get_sub_type (message) != LM_MESSAGE_SUB_TYPE_SET &&
      lm_message_get_sub_type (message) != LM_MESSAGE_SUB_TYPE_RESULT) {
    print_debug ("%s\n", lm_message_node_get_attribute (message->node, "type")); 
    print_debug ("xml:\n%s", lm_message_node_to_string (message->node));
    return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
  }

  if (strcmp (lm_message_node_get_attribute (node, "xmlns"), "jabber:iq:roster"))
    return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
  
  child = lm_message_node_get_child (node, "item");

  while (child) {
    gchar *jid;
    gchar *name;
    gchar *subs;
    GGaduContact *k = NULL;

    jid  = (gchar *) lm_message_node_get_attribute (child, "jid");
    name = (gchar *) lm_message_node_get_attribute (child, "name");
    subs = (gchar *) lm_message_node_get_attribute (child, "subscription");

    if (!jid)
      continue;

    if (strchr (jid, '/'))
      strchr (jid, '/')[0] = '\0';

    print_debug ("jabber: roster: %s -> %s with %s\n", jid, name, subs);

    if (!strcmp (subs, "remove")) {
      GSList *list = rosterlist;
      while (list) {
	k = (GGaduContact *) list->data;
	print_debug ("%p\n", k, jid);
       	if (!ggadu_strcasecmp (k->id, jid)) {
	  if (k->nick)
	    g_free (k->nick);
	  rosterlist = g_slist_remove (rosterlist, k);
	  userlist   = g_slist_remove (userlist, k);
	  ggadu_repo_del_value ("jabber", k->id);
	  g_free (k->id);
	  g_free (k);
	  break;
	}
	list = list->next;
      }
      return LM_HANDLER_RESULT_REMOVE_MESSAGE;
    }

    if (rosterlist) {
      GSList *list = rosterlist;
      while (list) {
	k = (GGaduContact *) list->data;
	if (!ggadu_strcasecmp (k->id, jid)) {
	  if (k->nick)
	    g_free (k->nick);
	  break;
	}
	list = list->next;
	k = NULL;
      }
    }
    
    if (!k) {
      k = g_new0 (GGaduContact, 1);
      k->id   = g_strdup (jid);
      rosterlist = g_slist_append (rosterlist, k);
    }
    k->nick = g_strdup (name ? name:jid);
    node = lm_message_node_get_child (child, "status");
    if (k->status_descr) {
      g_free (k->status_descr);
      k->status_descr = NULL;
    }
    if (node) {
      k->status_descr = g_strdup (lm_message_node_get_value (node));
    }
    
    if (strcmp (subs, "none") && strcmp (subs, "from"))
    {
      if (!strcmp (subs, "to"))
	k->status = JABBER_STATUS_WAIT_SUBSCRIBE;
      else
	k->status = JABBER_STATUS_UNAVAILABLE;
      
      if (!g_slist_find (userlist, k))
	userlist = g_slist_append (userlist, k);

      if (!ggadu_repo_add_value ("jabber", k->id, k, REPO_VALUE_CONTACT))
	ggadu_repo_change_value ("jabber", k->id, k, REPO_VALUE_DC);
    }
    
    child = child->next;
  }

  signal_emit ("jabber", "gui send userlist", userlist, "main-gui");
  
  if (first_time) {
    GSList *list;

    lm_connection_register_message_handler (connection, presence_handler,
	LM_MESSAGE_TYPE_PRESENCE, LM_HANDLER_PRIORITY_NORMAL);
    lm_connection_register_message_handler (connection, message_handler,
	LM_MESSAGE_TYPE_MESSAGE, LM_HANDLER_PRIORITY_NORMAL);

    list = userlist;
    while (list) {
      LmMessage *m;
      GGaduContact *k = (GGaduContact *) list->data;
      
      m = lm_message_new_with_sub_type (k->id, LM_MESSAGE_TYPE_PRESENCE, LM_MESSAGE_SUB_TYPE_PROBE);
      lm_connection_send (connection, m, NULL);
      lm_message_unref (m);
	
      list = list->next;
    }
  }

  return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

LmHandlerResult message_cb (LmMessageHandler *handler, LmConnection *connection, LmMessage *message, gpointer user_data)
{
  gchar *jid;
  GGaduMsg *msg;
  LmMessageNode *body;

  jid  = (gchar *) lm_message_node_get_attribute (message->node, "from"); 
  
  if (strchr (jid, '/'))
    strchr (jid, '/')[0] = '\0';

  if (lm_message_get_sub_type (message) == LM_MESSAGE_SUB_TYPE_ERROR) {
    print_debug ("jabber: Error message.\n");
    print_debug ("The message is:\n%s", lm_message_node_to_string (message->node));
    return LM_HANDLER_RESULT_REMOVE_MESSAGE;
  }

  body = lm_message_node_find_child (message->node, "body");
  if (!body) {
    print_debug ("jabber: Message from %s without a body.\n", jid);
    return LM_HANDLER_RESULT_REMOVE_MESSAGE;
  }
  
  msg = g_new0 (GGaduMsg, 1);
  msg->id = g_strdup (jid);
  msg->message = g_strdup (lm_message_node_get_value (body));
  
  signal_emit ("jabber", "gui msg receive", msg, "main-gui");
  
  lm_message_unref (message);

  return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}
