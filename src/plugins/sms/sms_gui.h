/* $Id: sms_gui.h,v 1.3 2003/06/09 18:24:34 shaster Exp $ */

#ifndef SMS_PLUGIN_H
#define SMS_PLUGIN_H 1

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif
 
enum {
    GGADU_SMS_CONFIG_SENDER,
    GGADU_SMS_CONFIG_NUMBER,
    GGADU_SMS_CONFIG_BODY,
    GGADU_SMS_CONFIG_EXTERNAL,
    GGADU_SMS_CONFIG_SHOW_IN_STATUS
    };

enum {
    GGADU_SMS_CONTACT_ID,
    GGADU_SMS_CONTACT_NICK,
    GGADU_SMS_CONTACT_NUMBER
    };

enum {
    GGADU_SMS_METHOD_POPUP,
    GGADU_SMS_METHOD_CHAT
    };

gchar *SENDER;

gint method;
    
GGaduPlugin *initialize_plugin(gpointer conf_ptr);

void signal_receive(gpointer name, gpointer signal_ptr);

void start_plugin();

void destroy_plugin();

void load_smslist();

void save_smslist();

#endif
