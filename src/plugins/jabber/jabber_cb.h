/* $Id: jabber_cb.h,v 1.4 2004/01/07 23:49:16 thrulliq Exp $ */

#ifndef JABBER_CB_H
#define JABBER_CB_H 1

#include <loudmouth/loudmouth.h>

void connection_auth_cb (LmConnection * connection, gboolean success, gint * status);
void connection_open_result_cb (LmConnection * connection, gboolean success, gint * status);

LmHandlerResult presence_cb (LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			     gpointer user_data);
LmHandlerResult iq_cb (LmMessageHandler * handler, LmConnection * connection, LmMessage * message, gpointer user_data);
LmHandlerResult iq_roster_cb (LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			      gpointer user_data);
LmHandlerResult iq_version_cb (LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			      gpointer user_data);
LmHandlerResult message_cb (LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			    gpointer user_data);

#endif
