#ifndef JABBER_PLUGIN_H
#define JABBER_PLUGIN_H 1

#include <loudmouth/loudmouth.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "menu.h"
#include "support.h"
#include "dialog.h"
#include "repo.h"

enum {
  GGADU_JABBER_JID,
  GGADU_JABBER_PASSWORD,
  GGADU_JABBER_LOG,
  GGADU_JABBER_AUTOCONNECT,
  GGADU_JABBER_USESSL,
  GGADU_JABBER_RESOURCE
};

enum states {
  JABBER_STATUS_UNAVAILABLE,
  JABBER_STATUS_AVAILABLE,
  JABBER_STATUS_CHAT,
  JABBER_STATUS_AWAY,
  JABBER_STATUS_XA,
  JABBER_STATUS_DND,
  JABBER_STATUS_WAIT_SUBSCRIBE,
  JABBER_STATUS_DESCR
};

enum subscription {
  JABBER_S_NONE,
  JABBER_S_TO,
  JABBER_S_FROM,
  JABBER_S_BOTH
};

typedef struct {
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

typedef struct {
    gint status;
    gchar *status_descr;
    GSList *userlist;
    GSList *actions;
    gint connected;
} jabber_data_type;


void ggadu_jabber_save_history (gchar *to, gchar *txt);

GGaduContact *user_in_list (gchar *jid, GSList *list);

/*#define user_in_userlist(x) user_in_list (x, userlist)
   #define user_in_rosterlist(x) user_in_list (x, rosterlist)*/

#endif
