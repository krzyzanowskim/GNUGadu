/* $Id: jabber_protocol.h,v 1.13 2004/12/29 13:32:42 krzyzak Exp $ */

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

#ifndef GGADU_JABBER_PROTOCOL_H
#define GGADU_JABBER_PROTOCOL_H 1

#include "jabber_plugin.h"

waiting_action *action_queue_add(gchar * id, gchar * type, gpointer action_callback, gchar * data, gboolean str_dup);

void action_queue_del(waiting_action * action);

void jabber_change_status(GGaduStatusPrototype *sp);
void jabber_fetch_roster(gpointer user_data);

void action_roster_add_result(LmConnection * connection, LmMessage * message, gpointer id);
void action_roster_remove_result(LmConnection * connection, LmMessage * message, gpointer data);
void action_roster_fetch_result(LmConnection * connection, LmMessage * message, gpointer user_data);

void action_search_form(LmConnection * connection, LmMessage * message, gpointer data);
void action_search_result(LmConnection * connection, LmMessage * message, gpointer data);

#endif
