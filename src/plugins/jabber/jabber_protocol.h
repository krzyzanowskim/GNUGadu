#ifndef JABBER_PROTOCOL_H
#define JABBER_PROTOCOL_H 1

#include "jabber_plugin.h"

void action_subscribe (LmConnection *connection, LmMessage *message, gpointer data);
void action_subscribe_result (LmConnection *connection, LmMessage *message, gpointer data);

void jabber_change_status (enum states status);
void jabber_fetch_roster (void);

#endif
