#ifndef JABBER_CB_H
#define JABBER_CB_H 1

#include "jabber_plugin.h"

void connection_auth_cb (LmConnection *connection, gboolean success, gint *status);
void connection_open_result_cb (LmConnection *connection, gboolean success, gint *status);

LmHandlerResult presence_cb (LmMessageHandler *handler, LmConnection *connection, LmMessage *message, gpointer user_data);
LmHandlerResult iq_cb (LmMessageHandler *handler, LmConnection *connection, LmMessage *message, gpointer user_data);
LmHandlerResult iq_roster_cb (LmMessageHandler *handler, LmConnection *connection, LmMessage *message, gpointer user_data);
LmHandlerResult message_cb (LmMessageHandler *handler, LmConnection *connection, LmMessage *message, gpointer user_data);

#endif
