/* $Id: dockapp_plugin.h,v 1.4 2003/12/11 10:02:20 thrulliq Exp $ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define DOCKLET_PLUGIN_NAME "docklet-dockapp"

enum
{
    GGADU_DOCKAPP_CONFIG_PROTOCOL,
    GGADU_DOCKAPP_CONFIG_VISIBLE
};

#define GGADU_DOCKAPP_STATUS_ONLINE	1
#define GGADU_DOCKAPP_STATUS_AWAY	2
#define GGADU_DOCKAPP_STATUS_OFFLINE	3
#define GGADU_DOCKAPP_STATUS_UNKNOWN	4
