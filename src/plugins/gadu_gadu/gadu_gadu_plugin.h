/* $Id: gadu_gadu_plugin.h,v 1.4 2003/04/10 10:11:34 krzyzak Exp $ */

#ifndef GGadu_PROTOCOL_GADU_H
#define GGadu_PROTOCOL_GADU_H 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

enum {
    GGADU_GADU_GADU_CONFIG_ID,
    GGADU_GADU_GADU_CONFIG_PASSWORD,
    GGADU_GADU_GADU_CONFIG_SERVER,
    GGADU_GADU_GADU_CONFIG_SOUND_CHAT_FILE,
    GGADU_GADU_GADU_CONFIG_SOUND_MSG_FILE,
    GGADU_GADU_GADU_CONFIG_SOUND_APP_FILE,
    GGADU_GADU_GADU_CONFIG_HISTORY,
    GGADU_GADU_GADU_CONFIG_AUTOCONNECT,
    GGADU_GADU_GADU_CONFIG_AUTOCONNECT_STATUS,
    GGADU_GADU_GADU_CONFIG_REASON,
    GGADU_GADU_GADU_CONFIG_FRIENDS_MASK
};

GGaduPlugin *initialize_plugin(gpointer conf_ptr);

gpointer config_init_plugin(GGaduPlugin *plugin_handler);

gboolean test_chan(GIOChannel *source, GIOCondition condition, gpointer data);

void my_signal_receive(gpointer name, gpointer signal_ptr);

void start_plugin();

void destroy_plugin();

gpointer gadu_gadu_login(gpointer data, gint status);

void wyjdz_signal_handler();

void load_contacts(gchar *encoding);

void test();

void save_addressbook_file(gpointer userlist);

gpointer user_preferences_action(gpointer user_data);

#endif
