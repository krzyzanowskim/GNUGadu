#ifndef JABBER_PROTOCOL_H
#define JABBER_PROTOCOL_H 1

#include "jabber_plugin.h"

void action_subscribe (LmConnection *connection, LmMessage *message, gpointer data);
void action_subscribe_result (LmConnection *connection, LmMessage *message, gpointer data);

waiting_action* action_queue_add (gchar *id, gchar *type, gpointer action_callback, gchar *data);
void action_queue_del (waiting_action *action);

void jabber_change_status (enum states status);
void jabber_fetch_roster (void);

void action_roster_add_result (LmConnection *connection, LmMessage *message, gpointer data);

void action_search_form (LmConnection *connection, LmMessage *message, gpointer data);
void action_search_result (LmConnection *connection, LmMessage *message, gpointer data);

#endif
