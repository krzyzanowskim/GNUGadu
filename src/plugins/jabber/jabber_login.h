#ifndef JABBER_LOGIN_H
#define JABBER_LOGIN_H 1

#include "jabber_plugin.h"

void jabber_login (enum states status);

gpointer jabber_login_connect(gpointer status);

#endif
