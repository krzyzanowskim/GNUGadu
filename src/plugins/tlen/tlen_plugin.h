/* $Id: tlen_plugin.h,v 1.1 2003/03/20 10:37:08 krzyzak Exp $ */

#ifndef GGadu_PROTOCOL_GADU_H
#define GGadu_PROTOCOL_GADU_H 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* meta status for "Set description" function */
#define TLEN_STATUS_DESC 666

typedef struct {
    guint    uin;
    gpointer password;
    guint    dummy;
} dummy_config;

enum {
    TLEN_TLEN_UIN,
    TLEN_TLEN_NICK,
    TLEN_TLEN_GROUP,
    TLEN_TLEN_PASSWORD,
    TLEN_TLEN_AUTOCONNECT
};

enum {
    GGADU_TLEN_SEARCH_FIRSTNAME,
    GGADU_TLEN_SEARCH_LASTNAME,
    GGADU_TLEN_SEARCH_NICKNAME,
    GGADU_TLEN_SEARCH_CITY,
    GGADU_TLEN_SEARCH_ACTIVE,
    GGADU_TLEN_SEARCH_ID
};

GGaduPlugin *initialize_plugin(gpointer conf_ptr);

gpointer config_init_plugin(GGaduPlugin *plugin_handler);

gboolean test_chan(GIOChannel *source, GIOCondition condition, gpointer data);

void my_signal_receive(gpointer name, gpointer signal_ptr);

void start_plugin();

void destroy_plugin();

void available(gpointer widget, gpointer data);

gpointer login(gpointer data);

void wyjdz_signal_handler();

void load_contacts();

void setup_watch(GIOChannel *source);

gpointer user_preferences_action(gpointer user_data);

#endif
