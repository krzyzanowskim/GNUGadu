/* $Id: dbus_plugin.h,v 1.6 2004/10/28 09:00:38 krzyzak Exp $ */

/* 
 * DBUS plugin code for GNU Gadu 2 
 * 
 * Copyright (C) 2001-2004 GNU Gadu Team 
 * 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version. 
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#define DBUS_ORG_FREEDESKTOP_IM_SERVICE "org.freedesktop.im.GG"
#define DBUS_ORG_FREEDESKTOP_IM_INTERFACE "org.freedesktop.im"
#define DBUS_ORG_FREEDESKTOP_IM_OBJECT "/org/freedesktop/im/GNUGadu"
#define DBUS_ORG_FREEDESKTOP_IM_GET_PRESENCE "getPresence"
#define DBUS_ORG_FREEDESKTOP_IM_GET_PROTOCOLS "getProtocols"

enum
{
    IM_PRESENCE_ONLINE,
    IM_PRESENCE_AWAY,
    IM_PRESENCE_OFFLINE
} DBusIMPresence;
