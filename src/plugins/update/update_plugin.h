/* $Id: update_plugin.h,v 1.2 2003/05/31 20:45:13 shaster Exp $ */

#ifndef UPDATE_PLUGIN_H
#define UPDATE_PLUGIN_H 1

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#define GGADU_UPDATE_USERAGENT_STRING "GNU Gadu 2 " VERSION " update plugin"
#define GGADU_UPDATE_SERVER "sourceforge.net"
#define GGADU_UPDATE_URL "/export/rss2_projfiles.php?group_id=76206"
#define GGADU_UPDATE_PORT 80

enum
{
    GGADU_UPDATE_CONFIG_CHECK_ON_STARTUP,
    GGADU_UPDATE_CONFIG_CHECK_AUTOMATICALLY,
    GGADU_UPDATE_CONFIG_CHECK_INTERVAL,
    GGADU_UPDATE_CONFIG_USE_XOSD,
    GGADU_UPDATE_CONFIG_SERVER
};

GGaduPlugin *initialize_plugin(gpointer conf_ptr);
void signal_receive(gpointer name, gpointer signal_ptr);
void start_plugin();
void destroy_plugin();

#endif
