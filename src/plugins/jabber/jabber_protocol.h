/* $Id: jabber_protocol.h,v 1.9 2004/01/17 00:45:02 shaster Exp $ */

#ifndef GGADU_JABBER_PROTOCOL_H
#define GGADU_JABBER_PROTOCOL_H 1

#include "jabber_plugin.h"

waiting_action *action_queue_add(gchar * id, gchar * type, gpointer action_callback, gchar * data, gboolean str_dup);

void action_queue_del(waiting_action * action);

void jabber_change_status(enum states status);
void jabber_fetch_roster(gpointer user_data);

void action_roster_add_result(LmConnection * connection, LmMessage * message, gpointer id);
void action_roster_remove_result(LmConnection * connection, LmMessage * message, gpointer data);
void action_roster_fetch_result(LmConnection * connection, LmMessage * message, gpointer user_data);

void action_search_form(LmConnection * connection, LmMessage * message, gpointer data);
void action_search_result(LmConnection * connection, LmMessage * message, gpointer data);

#endif
