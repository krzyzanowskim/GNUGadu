#ifndef JABBER_PLUGIN_H
#define JABBER_PLUGIN_H 1

#include <iksemel.h>

#define id_print IKS_ID_USER | IKS_ID_SERVER

extern GGaduPlugin *jabber_handler;

extern GIOChannel *source_chan;
extern GSList *userlist;
extern struct netdata *jabber_session;
extern gint watch;
extern gboolean connected;

enum {
    GGADU_JID,
    GGADU_JID_PASSWORD,
    GGADU_JID_AUTOCONNECT
};


enum states {
    JABBER_STATUS_AVAILABLE = IKS_SHOW_AVAILABLE,
    JABBER_STATUS_UNAVAILABLE = IKS_SHOW_UNAVAILABLE,
    JABBER_STATUS_AWAY = IKS_SHOW_AWAY,
    JABBER_STATUS_XA = IKS_SHOW_XA,
    JABBER_STATUS_DND = IKS_SHOW_DND,
    JABBER_STATUS_CHAT = IKS_SHOW_CHAT
};

enum netstates
{
	NET_OFF,
	NET_CONNECT,
	NET_STREAM,
	NET_REG,
	NET_AUTH,
	NET_ON
};


struct netdata
{
	iksparser *parser;
	iksid *id;
	enum netstates state;
	enum states status;
	int regflag;
};


gboolean updatewatch(struct netdata *);

#endif
