/* $Id: dockapp_plugin.h,v 1.3 2003/11/25 23:40:30 thrulliq Exp $ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define DOCKLET_PLUGIN_NAME "docklet-dockapp"

enum
{
    GGADU_DOCKAPP_CONFIG_PROTOCOL
};

#define GGADU_DOCKAPP_STATUS_ONLINE	1
#define GGADU_DOCKAPP_STATUS_AWAY	2
#define GGADU_DOCKAPP_STATUS_OFFLINE	3
#define GGADU_DOCKAPP_STATUS_UNKNOWN	4
