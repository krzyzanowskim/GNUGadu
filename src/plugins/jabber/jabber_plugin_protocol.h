/* $Id: jabber_plugin_protocol.h,v 1.3 2003/06/09 18:24:35 shaster Exp $ */

#ifndef JABBER_PLUGIN_PROTOCOL_H
#define JABBER_PLUGIN_PROTOCOL_H 1

#include <iksemel.h>

gpointer jabber_login(gpointer data);
void message_parse(ikspak *pak);
void j_handle_iq(ikspak *pak);
void j_on_packet(void *udata, ikspak *pak);
void jabber_presence(struct netdata *session, gint status, gchar *desc);

#endif
