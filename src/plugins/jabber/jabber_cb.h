/* $Id: jabber_cb.h,v 1.13 2004/12/20 09:15:20 krzyzak Exp $ */

/* 
 * Jabber plugin for GNU Gadu 2 
 * 
 * Copyright (C) 2003-2005 GNU Gadu Team 
 * 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version. 
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef JABBER_CB_H
#define JABBER_CB_H 1

#include <loudmouth/loudmouth.h>
#include "jabber_plugin.h"

void connection_auth_cb(LmConnection * connection, gboolean success, gpointer status);
void connection_open_result_cb(LmConnection * connection, gboolean success, gint * status);

LmHandlerResult presence_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			    gpointer user_data);
LmHandlerResult iq_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message, gpointer user_data);
LmHandlerResult iq_roster_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			     gpointer user_data);
LmHandlerResult iq_version_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			      gpointer user_data);
LmHandlerResult iq_vcard_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			      gpointer user_data);
LmHandlerResult iq_account_data_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			      gpointer user_data);
LmHandlerResult message_cb(LmMessageHandler * handler, LmConnection * connection, LmMessage * message,
			   gpointer user_data);
LmHandlerResult register_register_handler(LmMessageHandler * handler, LmConnection * connection, LmMessage * msg,
					  GGaduJabberRegister *data);

void jabber_disconnect_cb(LmConnection * connection, LmDisconnectReason reason, gpointer user_data);
void jabber_register_account_cb(LmConnection * connection, gboolean result, gpointer data);
#endif
