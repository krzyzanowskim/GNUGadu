/* $Id: jabber_plugin.h,v 1.48 2005/02/23 15:28:57 mkobierzycki Exp $ */

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

#ifndef GGADU_JABBER_PLUGIN_H
#define GGADU_JABBER_PLUGIN_H 1

#include <loudmouth/loudmouth.h>

#include "ggadu_types.h"
#include "plugins.h"
#include "signals.h"
#include "ggadu_menu.h"
#include "ggadu_support.h"
#include "ggadu_dialog.h"
#include "ggadu_repo.h"


#define ggadu_jabber_save_history(_type,_msg,_nick)	\
	if (ggadu_config_var_get(jabber_handler, "log")) \
	{ \
	    gchar *path = g_build_filename(config->configdir, "history", _msg->id, NULL); \
	    ggadu_save_history(_type, path, _nick, _msg); \
	    g_free(path); \
	}

#define JABBER_DEFAULT_RESOURCE "GNU Gadu"

enum
{
    GGADU_JABBER_JID,
    GGADU_JABBER_PASSWORD,
    GGADU_JABBER_LOG,
    GGADU_JABBER_ONLY_FRIENDS,
    GGADU_JABBER_AUTOCONNECT,
    GGADU_JABBER_AUTOSTATUS,
    GGADU_JABBER_USESSL,
    GGADU_JABBER_RESOURCE,
    GGADU_JABBER_SERVER,
    GGADU_JABBER_USERNAME,
    GGADU_JABBER_UPDATE_CONFIG,
    GGADU_JABBER_REQUEST_AUTH_FROM,
    GGADU_JABBER_PROXY,
    GGADU_JABBER_IGNORED,
    GGADU_JABBER_PASSWORD_NEW,
    GGADU_JABBER_PASSWORD_RETYPE
};

enum states
{
    JABBER_STATUS_UNAVAILABLE,
    JABBER_STATUS_AVAILABLE,
    JABBER_STATUS_CHAT,
    JABBER_STATUS_AWAY,
    JABBER_STATUS_XA,
    JABBER_STATUS_DND,
    JABBER_STATUS_DESCR,
    JABBER_STATUS_ERROR,
    JABBER_STATUS_NOAUTH,
    JABBER_STATUS_AUTH_FROM
};

enum subscription
{
    JABBER_S_NONE,
    JABBER_S_TO,
    JABBER_S_FROM,
    JABBER_S_BOTH
};

enum vcard
{
    GGADU_JABBER_GIVEN,
    GGADU_JABBER_FAMILY,
    GGADU_JABBER_FN,
    GGADU_JABBER_NICKNAME,
    GGADU_JABBER_URL,
    GGADU_JABBER_BDAY,
    GGADU_JABBER_BMONTH,
    GGADU_JABBER_BYEAR,
    GGADU_JABBER_ORGNAME,
    GGADU_JABBER_NUMBER,
    GGADU_JABBER_LOCALITY,
    GGADU_JABBER_CTRY,
    GGADU_JABBER_USERID
};

enum software
{
    GGADU_JABBER_CLIENT,
    GGADU_JABBER_VERSION,
    GGADU_JABBER_OS
};
typedef struct
{
    gchar *id;
    gchar *type;
    gpointer data;
    void (*func) (LmConnection *, LmMessage *, gpointer);
} waiting_action;

typedef struct
{
	gchar *server;
	gchar *username;
	gchar *password;
	gboolean update_config;
} GGaduJabberRegister;

typedef struct
{
	gchar *jid;
	gchar *client;
	gchar *version;
	gchar *os;
} GGaduJabberSoftware;

extern GGaduPlugin *jabber_handler;
extern GGaduProtocol *p;
extern LmMessageHandler *iq_handler;
extern LmMessageHandler *iq_roster_handler;
extern LmMessageHandler *iq_version_handler;
extern LmMessageHandler *iq_vcard_handler;
extern LmMessageHandler *iq_account_data_handler;
extern LmMessageHandler *presence_handler;
extern LmMessageHandler *message_handler;

typedef struct
{
    gint status;
    gchar *description;
    GSList *actions;
    GSList *software;
    LmConnection *connection;
    LmProxy *proxy;
} jabber_data_type;

gpointer jabber_register_account_dialog(gpointer user_data);
gpointer jabber_services_discovery_action(gpointer user_data);
gpointer user_preferences_action(gpointer user_data);

void software_slist_free(void);
void GGaduJabberSoftware_free(GGaduJabberSoftware *data);

#endif
