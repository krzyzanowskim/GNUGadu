/*
 * Jabber protocol plugin for GNU Gadu 2 based on loudmouth library
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "jabber_plugin.h"
#include "jabber_login.h"
#include "jabber_protocol.h"

GGaduPlugin *jabber_handler;
LmConnection *connection;

LmMessageHandler *iq_handler;
LmMessageHandler *iq_roster_handler;
LmMessageHandler *presence_handler;
LmMessageHandler *message_handler;

GSList *userlist;
GSList *rosterlist;

GSList *actions;

gint connected = 0;

enum states jabber_status = JABBER_STATUS_UNAVAILABLE;

gchar *status_descr = NULL;

extern GGaduConfig *config;
GGaduProtocol *p;
GGaduMenu *jabbermenu;

GGadu_PLUGIN_INIT ("jabber", GGADU_PLUGIN_TYPE_PROTOCOL);

GGaduContact *user_in_list (gchar *jid, GSList *list)
{
  GSList *tmp = list;
  GGaduContact *k;
  
  while (list)
  {
    k = (GGaduContact *) tmp->data;
    if (!ggadu_strcasecmp (k->id, jid))
      return k;
    list = list->next;
  }

  return NULL;
}

void jabber_signal_recv (gpointer name, gpointer signal_ptr)
{
  GGaduSignal *signal = (GGaduSignal *) signal_ptr;

  print_debug ("%s: receive signal %d %s\n", GGadu_PLUGIN_NAME,
      signal->name, g_quark_to_string (signal->name));

  if (signal->name == g_quark_from_static_string ("update config")) {
    GGaduDialog *d = signal->data;
    GSList *tmplist = d->optlist;

    if (d->response != GGADU_OK)
      return;
    while (tmplist)
    {
      GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
      switch (kv->key) {
	case GGADU_JABBER_JID:
	  print_debug ("changing jid to %s\n", kv->value);
	  config_var_set (jabber_handler, "jid", kv->value);
	  break;
	case GGADU_JABBER_PASSWORD:
	  print_debug ("changing password to %s\n", kv->value);
	  config_var_set (jabber_handler, "password", kv->value);
	  break;
	case GGADU_JABBER_AUTOCONNECT:
	  print_debug ("changing autoconnect to %d\n", kv->value);
	  config_var_set (jabber_handler, "autoconnect", kv->value);
	  break;
	case GGADU_JABBER_USESSL:
	  print_debug ("changing use_ssl to %d\n", kv->value);
	  config_var_set (jabber_handler, "use_ssl", kv->value);
	  break;
      }
      tmplist = tmplist->next;
    }
    config_save (jabber_handler);

    GGaduDialog_free (d);
  } else if (signal->name == g_quark_from_static_string ("change status descr")) {
    GGaduDialog *d = signal->data;

    if (d->response == GGADU_OK) {
      if (status_descr)
	g_free (status_descr);
      status_descr = g_strdup (((GGaduKeyValue *)(d->optlist->data))->value);
      jabber_login (jabber_status);
    }
  } else if (signal->name == g_quark_from_static_string ("change status")) {
    GGaduStatusPrototype *sp = signal->data;

    if (!sp)
      return;

    if (sp->status == JABBER_STATUS_DESCR) {
      GGaduDialog *d = ggadu_dialog_new ();

      ggadu_dialog_set_title (d, _("Enter status description"));
      ggadu_dialog_callback_signal (d, "change status descr");
      ggadu_dialog_add_entry (&d->optlist, 0, _("Description:"), VAR_STR, status_descr, VAR_FLAG_FOCUS);
      d->user_data = sp;
      signal_emit ("jabber", "gui show dialog", d, "main-gui");
    }

    jabber_login (sp->status);
  } else if (signal->name == g_quark_from_static_string ("get current status")) {
    signal->data_return = (gpointer) jabber_status;
  } else if (signal->name == g_quark_from_static_string ("send message")) {
    GGaduMsg *msg = signal->data;

    if (msg && connected == 2) {
      LmMessage *m;
      gboolean result;
      GError *error = NULL;

      m = lm_message_new_with_sub_type (msg->id, LM_MESSAGE_TYPE_MESSAGE, LM_MESSAGE_SUB_TYPE_CHAT);
      lm_message_node_add_child (m->node, "body", msg->message);
      result = lm_connection_send (connection, m, &error);
      if (!result)
	print_debug ("jabber: Can't send!\n");
      lm_message_unref (m);
    }
  } else if (signal->name == g_quark_from_static_string ("add user"))
  {
    GGaduContact *k = g_new0 (GGaduContact, 1);
    GSList *kvlist = (GSList *) signal->data;
    LmMessage *m;
    LmMessageNode *node;
    gboolean result;
    waiting_action *action;

    while (kvlist)
    {
      GGaduKeyValue *kv = (GGaduKeyValue *) kvlist->data;

      if ((gint) kv->key == GGADU_ID) {
	k->id = g_strdup ((gchar *) kv->value);
	g_free (kv->value);
      } else if ((gint) kv->key == GGADU_NICK) {
	if (kv->value && ((gchar *)kv->value)[0] != '\0')
	  k->nick = g_strdup ((gchar *) kv->value);
      }
      kvlist = kvlist->next;
    }

    g_slist_free (kvlist);

    m = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ,
	LM_MESSAGE_SUB_TYPE_SET);
    lm_message_node_set_attribute (m->node, "id", "roster_2");
    
    node = lm_message_node_add_child (m->node, "query", NULL);
    lm_message_node_set_attribute (node, "xmlns", "jabber:iq:roster");

    node = lm_message_node_add_child (node, "item", NULL);
    print_debug ("%s\n", k->id);
    lm_message_node_set_attribute (node, "jid", k->id);
   
    if (k->nick) {
      lm_message_node_set_attribute (node, "name", k->nick);
    } else {
      action = g_new0 (waiting_action, 1);
      action->id = g_strdup ("roster_2");
      action->type = g_strdup ("result");
      action->data = g_strdup (k->id);
      action->func = action_subscribe;
      actions = g_slist_append (actions, action);
    }

    result = lm_connection_send (connection, m, NULL);
    lm_message_unref (m);
    if (!result)
    {
      actions = g_slist_remove (actions, action);
      g_free (action);
      print_debug ("jabber: Couldn't send.\n");
    }

    g_free (k->id);
    g_free (k);
  } else if (signal->name == g_quark_from_static_string ("jabber subscribe")) {
    GGaduDialog *d = signal->data;
    gchar *jid = d->user_data;

    if (jid && d->response == GGADU_OK) {
      LmMessage *msg;
      gboolean result;

      msg = lm_message_new_with_sub_type (jid, LM_MESSAGE_TYPE_PRESENCE, LM_MESSAGE_SUB_TYPE_SUBSCRIBED);
      result = lm_connection_send (connection, msg, NULL);
      lm_message_unref (msg);
      if (!result)
	print_debug ("Ojej\n");
    }

    if (jid)
      g_free (jid);

    GGaduDialog_free (d);
  }
}

GSList *status_init ()
{
  GSList *list = NULL;
  GGaduStatusPrototype *sp;

  if (!(sp = g_new0 (GGaduStatusPrototype, 7)))
    return NULL;

  sp->status      = JABBER_STATUS_AVAILABLE;
  sp->description = g_strdup (_("Available"));
  sp->image       = g_strdup ("jabber-online.png");
  list = g_slist_append (list, sp++);

  sp->status      = JABBER_STATUS_AWAY;
  sp->description = g_strdup (_("Away"));
  sp->image       = g_strdup ("jabber-away.png");
  list = g_slist_append (list, sp++);

  sp->status      = JABBER_STATUS_XA;
  sp->description = g_strdup (_("XA"));
  sp->image       = g_strdup ("jabber-xa.png");
  list = g_slist_append (list, sp++);

  sp->status      = JABBER_STATUS_DND;
  sp->description = g_strdup (_("DND"));
  sp->image       = g_strdup ("jabber-dnd.png");
  list = g_slist_append (list, sp++);

  sp->status      = JABBER_STATUS_UNAVAILABLE;
  sp->description = g_strdup (_("Unavailable"));
  sp->image       = g_strdup ("jabber-offline.png");
  list = g_slist_append (list, sp++);

  sp->status      = JABBER_STATUS_DESCR;
  sp->description = g_strdup (_("Set description ..."));
  sp->image       = g_strdup ("tlen-desc.png");
  list = g_slist_append (list, sp++);

  sp->status      = JABBER_STATUS_WAIT_SUBSCRIBE;
  sp->description = g_strdup (_("Subscribe"));
  sp->image       = g_strdup ("jabber-subscribe.png");
  sp->receive_only= TRUE;
  list = g_slist_append (list, sp++);

  return list;
}

gpointer user_preferences_action (gpointer user_data)
{
  GGaduDialog *d;

  d = g_new0 (GGaduDialog, 1);
  d->title           = g_strdup (_("Jabber plugin configuration"));
  d->callback_signal = g_strdup ("update config");

  ggadu_dialog_set_type (d, GGADU_DIALOG_CONFIG);
  ggadu_dialog_add_entry (&(d->optlist), GGADU_JABBER_JID,
      _("Jabber ID"), VAR_STR,
      config_var_get (jabber_handler, "jid"), VAR_FLAG_NONE);
  ggadu_dialog_add_entry (&(d->optlist), GGADU_JABBER_PASSWORD,
      _("Password"), VAR_STR,
      config_var_get (jabber_handler, "password"), VAR_FLAG_PASSWORD);
  ggadu_dialog_add_entry (&(d->optlist), GGADU_JABBER_AUTOCONNECT,
      _("Autoconnect on startup"), VAR_BOOL,
      config_var_get (jabber_handler, "autoconnect"), VAR_FLAG_NONE);
  ggadu_dialog_add_entry (&(d->optlist), GGADU_JABBER_USESSL,
      _("Use SSL"), VAR_BOOL,
      config_var_get (jabber_handler, "use_ssl"), VAR_FLAG_NONE);

  signal_emit (GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");
  return NULL;
}

gpointer user_chat_action (gpointer user_data)
{
  GSList *users = (GSList *)user_data;
  GGaduMsg *msg = g_new0 (GGaduMsg, 1);

  if (!users)
    return NULL;

  /* TODO: do this, when conference is being handled */

  return NULL;
}

gpointer user_add_action (gpointer user_data)
{
  GSList *optlist = NULL;

  ggadu_dialog_add_entry (&optlist, GGADU_ID, _("Jabber ID (jid)"), VAR_STR,
      NULL, VAR_FLAG_NONE);
  signal_emit (GGadu_PLUGIN_NAME, "gui add user window", optlist, "main-gui");

  return NULL;
}

gpointer user_edit_action (gpointer user_data)
{
  GSList *optlist = NULL;
  GSList *user = (GSList *) user_data;
  GGaduContact *k;

  if (!user)
    return NULL;

  k = (GGaduContact *) user->data;
  ggadu_dialog_add_entry (&optlist, GGADU_ID, _("Jabber ID (jid)"), VAR_STR, k->id, VAR_FLAG_NONE);
  ggadu_dialog_add_entry (&optlist, GGADU_NICK, _("Nickname"), VAR_STR, k->nick, VAR_FLAG_NONE);

  signal_emit (GGadu_PLUGIN_NAME, "gui add user window", optlist, "main-gui");
  return NULL;
}

gpointer user_remove_action (gpointer user_data)
{
  GSList *users = (GSList *) user_data;

  while (users)
  {
    GGaduContact *k = (GGaduContact *) users->data;

    if (k)
    {
      if (connected == 2)
      {
        LmMessage *m;
        gboolean result;
        LmMessageNode *node;

        m = lm_message_new_with_sub_type (NULL, LM_MESSAGE_TYPE_IQ,
            LM_MESSAGE_SUB_TYPE_SET);
        node = lm_message_node_add_child (m->node, "query", NULL);
        lm_message_node_set_attributes (node, "xmlns", "jabber:iq:roster",
            NULL);
        node = lm_message_node_add_child (node, "item", NULL);
        lm_message_node_set_attributes (node, "jid", g_strdup (k->id),
            "subscription", "remove", NULL);

	rosterlist = g_slist_remove (rosterlist, k);
	userlist = g_slist_remove (userlist, k);
	ggadu_repo_del_value ("jabber", k->id);
	signal_emit ("jabber", "gui send userlist", userlist, "main-gui");
	GGaduContact_free (k);
        result = lm_connection_send (connection, m, NULL);
        if (!result)
          print_debug ("jabber: Can't send!\n");
        lm_message_unref (m);
      }
    }

    users = users->next;
  }

  return NULL;
}

GGaduMenu *build_jabber_menu ()
{
  GGaduMenu *root;
  GGaduMenu *item;

  root = ggadu_menu_create ();
  item = ggadu_menu_add_item (root, "_Jabber", NULL, NULL);

  ggadu_menu_add_submenu (item, ggadu_menu_new_item (_("Add Contact"),
      user_add_action, NULL));
  ggadu_menu_add_submenu (item, ggadu_menu_new_item (_("Preferences"),
      user_preferences_action, NULL));

  return root;
}

void register_userlist_menu (void)
{
  GGaduMenu *menu = ggadu_menu_create ();

  ggadu_menu_add_submenu (menu, ggadu_menu_new_item (_("Chat"),   user_chat_action, NULL));
  ggadu_menu_add_submenu (menu, ggadu_menu_new_item (_("Add"),    user_add_action, NULL));
  ggadu_menu_add_submenu (menu, ggadu_menu_new_item (_("Edit"),   user_edit_action, NULL));
  ggadu_menu_add_submenu (menu, ggadu_menu_new_item (_("Remove"), user_remove_action, NULL));

  signal_emit (GGadu_PLUGIN_NAME, "gui register userlist menu", menu, "main-gui");
}

void start_plugin ()
{
  p = g_new0 (GGaduProtocol, 1);
  p->display_name   = g_strdup ("Jabber");
  p->img_filename   = g_strdup ("jabber.png");
  p->statuslist     = status_init ();
  p->offline_status = g_slist_append (p->offline_status, (gint *)JABBER_STATUS_UNAVAILABLE);

  ggadu_repo_add_value ("_protocols_", p->display_name, p, REPO_VALUE_PROTOCOL);

  signal_emit (GGadu_PLUGIN_NAME, "gui register protocol", p, "main-gui");

  register_signal (jabber_handler, "change status");
  register_signal (jabber_handler, "get current status");
  register_signal (jabber_handler, "update config");
  register_signal (jabber_handler, "send message");
  register_signal (jabber_handler, "add user");
  register_signal (jabber_handler, "jabber subscribe");

  jabbermenu = build_jabber_menu ();
  signal_emit (GGadu_PLUGIN_NAME, "gui register menu", jabbermenu, "main-gui");

  register_userlist_menu ();

  if (config_var_get (jabber_handler, "autoconnect") && !connected)
  {
    print_debug ("jabber: autoconneting\n");
    jabber_login (JABBER_STATUS_AVAILABLE);
  }

}

GGaduPlugin *initialize_plugin (gpointer conf_ptr)
{
  GGadu_PLUGIN_ACTIVATE (conf_ptr);

  print_debug ("%s: initialize\n", GGadu_PLUGIN_NAME);

  jabber_handler = (GGaduPlugin *) register_plugin (GGadu_PLUGIN_NAME,
      _("Jabber protocol"));

  register_signal_receiver (jabber_handler,(signal_func_ptr)jabber_signal_recv);

  mkdir (config->configdir, 0700);
  set_config_file_name (jabber_handler,
      g_build_filename (config->configdir, "jabber", NULL));

  config_var_add (jabber_handler, "jid", VAR_STR);
  config_var_add (jabber_handler, "password", VAR_STR);
  config_var_add (jabber_handler, "autoconnect", VAR_BOOL);
  config_var_add (jabber_handler, "use_ssl", VAR_BOOL);

  if (!config_read (jabber_handler))
    g_warning (_("Unable to read configuration file for plugin jabber"));

  ggadu_repo_add ("jabber");

  return jabber_handler;
}

void destroy_plugin ()
{
  print_debug ("destroy_plugin %s\n", GGadu_PLUGIN_NAME);

  if (jabbermenu)
  {
    signal_emit (GGadu_PLUGIN_NAME,"gui unregister menu",jabbermenu,"main-gui");
    ggadu_menu_free (jabbermenu);
  }

  ggadu_repo_del_value ("_protocols", p->display_name);

  signal_emit(GGadu_PLUGIN_NAME,"gui unregister userlist menu",NULL,"main-gui");
  signal_emit(GGadu_PLUGIN_NAME,"gui unregister protocol", p, "main-gui");
}

