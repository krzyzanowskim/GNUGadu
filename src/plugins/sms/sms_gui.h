/* $Id: sms_gui.h,v 1.7 2003/09/21 14:08:31 shaster Exp $ */

#ifndef SMS_PLUGIN_H
#define SMS_PLUGIN_H 1

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#define GGADU_SMS_MAXLEN_NUMBER	20
#define GGADU_SMS_MAXLEN_NICK	100

enum
{
    GGADU_SMS_CONFIG_SENDER,
    GGADU_SMS_CONFIG_NUMBER,
    GGADU_SMS_CONFIG_BODY,
    GGADU_SMS_CONFIG_EXTERNAL,
    GGADU_SMS_CONFIG_ERA_LOGIN,
    GGADU_SMS_CONFIG_ERA_PASSWORD,
    GGADU_SMS_CONFIG_SHOW_IN_STATUS
};

enum
{
    GGADU_SMS_CONTACT_ID,
    GGADU_SMS_CONTACT_NICK,
    GGADU_SMS_CONTACT_NUMBER
};

enum
{
    GGADU_SMS_METHOD_POPUP,
    GGADU_SMS_METHOD_CHAT
};

gint method;

GGaduPlugin *initialize_plugin(gpointer conf_ptr);
void signal_receive(gpointer name, gpointer signal_ptr);
void start_plugin();
void destroy_plugin();
void load_smslist();
void save_smslist();

#endif
