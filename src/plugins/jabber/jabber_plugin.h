/* $Id: jabber_plugin.h,v 1.21 2004/01/28 23:41:29 shaster Exp $ */

/* 
 * Jabber plugin for GNU Gadu 2 
 * 
 * Copyright (C) 2003-2004 GNU Gadu Team 
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

#ifndef GGADU_JABBER_PLUGIN_H
#define GGADU_JABBER_PLUGIN_H 1

#include <loudmouth/loudmouth.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "menu.h"
#include "support.h"
#include "dialog.h"
#include "repo.h"

enum
{
    GGADU_JABBER_JID,
    GGADU_JABBER_PASSWORD,
    GGADU_JABBER_LOG,
    GGADU_JABBER_AUTOCONNECT,
    GGADU_JABBER_USESSL,
    GGADU_JABBER_RESOURCE,
    GGADU_JABBER_SERVER
};

enum states
{
    JABBER_STATUS_UNAVAILABLE,
    JABBER_STATUS_AVAILABLE,
    JABBER_STATUS_CHAT,
    JABBER_STATUS_AWAY,
    JABBER_STATUS_XA,
    JABBER_STATUS_DND,
    JABBER_STATUS_ERROR,
    JABBER_STATUS_DESCR
};

enum subscription
{
    JABBER_S_NONE,
    JABBER_S_TO,
    JABBER_S_FROM,
    JABBER_S_BOTH
};

typedef struct
{
    gchar *id;
    gchar *type;
    gpointer data;
    void (*func) (LmConnection *, LmMessage *, gpointer);
} waiting_action;

extern GGaduPlugin *jabber_handler;
extern LmConnection *connection;

extern LmMessageHandler *iq_handler;
extern LmMessageHandler *iq_roster_handler;
extern LmMessageHandler *iq_version_handler;
extern LmMessageHandler *presence_handler;
extern LmMessageHandler *message_handler;

typedef struct
{
    gint status;
    gchar *status_descr;
    GSList *actions;
    gint connected;
} jabber_data_type;


void ggadu_jabber_save_history(gchar * to, gchar * txt);

#endif
