/* $Id: jabber_login.h,v 1.5 2004/01/13 22:22:44 krzyzak Exp $ */

#ifndef GGADU_JABBER_LOGIN_H
#define GGADU_JABBER_LOGIN_H 1

#include "jabber_plugin.h"

void jabber_login (enum states status);

gpointer jabber_login_connect(gpointer status);

#endif
