/* $Id: dockapp_plugin.h,v 1.5 2004/01/28 23:40:42 shaster Exp $ */

/* 
 * Dockapp plugin for GNU Gadu 2 
 * 
 * Copyright (C) 2003-2004 GNU Gadu Team 
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
