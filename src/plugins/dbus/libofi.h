/*
    ORG.FREEDESKTOP.IM interface common header file.
    licence: BSD
    Authors : 
              Marcin Krzy¿anowski <krzak@linux.net.pl>
              Igor Popik <igipop@wsfiz.edu.pl>  

    You can freely distribute it and use in any application
*/

#ifndef LIBOFI_H
#define LIBOFI_H 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* define this one by yourself  */
/* #define DBUS_ORG_FREEDESKTOP_IM_SERVICE "org.freedesktop.im.SampleApplicationService" */

/* const */

#define DBUS_ORG_FREEDESKTOP_IM_INTERFACE "org.freedesktop.im"
#define DBUS_ORG_FREEDESKTOP_IM_OBJECT "/org/freedesktop/im"

/* methods */
#define DBUS_ORG_FREEDESKTOP_IM_GET_PRESENCE "getPresence"
typedef enum
{
    OFI_IM_PRESENCE_NOT_FOUND = 1,
    OFI_IM_PRESENCE_AVAILABLE,
    OFI_IM_PRESENCE_AWAY,
    OFI_IM_PRESENCE_OFFLINE
} ofiPresenceEnum;

#define DBUS_ORG_FREEDESKTOP_IM_GET_PROTOCOLS "getProtocols"
#define DBUS_ORG_FREEDESKTOP_IM_OPEN_CHAT "openChat"



char **
org_freedesktop_im_getServices(DBusConnection *bus);

#endif

