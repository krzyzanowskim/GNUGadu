/* $Id: jabber_plugin.h,v 1.20 2004/01/17 12:37:58 krzyzak Exp $ */

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
