/* $Id: jabber_cb.h,v 1.7 2004/01/17 00:45:01 shaster Exp $ */

#ifndef JABBER_CB_H
#define JABBER_CB_H 1

#include <loudmouth/loudmouth.h>

void connection_auth_cb(LmConnection * connection, gboolean success, gpointer status);
void connection_open_result_cb(LmConnection * connection, gboolean success, gint * status);

LmHandlerResult presence_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message, gpointer user_data);
LmHandlerResult iq_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message, gpointer user_data);
LmHandlerResult iq_roster_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message, gpointer user_data);
LmHandlerResult iq_version_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message, gpointer user_data);
LmHandlerResult message_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message, gpointer user_data);

void jabber_disconnect_cb(LmConnection * connection, LmDisconnectReason reason, gpointer user_data);

#endif
