/* $Id: tlen_plugin.h,v 1.3 2003/06/01 13:49:20 shaster Exp $ */

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
    GGADU_TLEN_UIN,
    GGADU_TLEN_NICK,
    GGADU_TLEN_GROUP,
    GGADU_TLEN_PASSWORD,
    GGADU_TLEN_AUTOCONNECT,
    GGADU_TLEN_AUTOCONNECT_STATUS
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

gpointer ggadu_tlen_login(gpointer data);

void wyjdz_signal_handler();

void load_contacts();

void setup_watch(GIOChannel *source);

gpointer user_preferences_action(gpointer user_data);

#endif
