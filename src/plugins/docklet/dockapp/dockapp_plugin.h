/* $Id: dockapp_plugin.h,v 1.7 2005/02/17 16:52:43 andyx_x Exp $ */

/* 
 * Dockapp plugin for GNU Gadu 2 
 * 
 * Copyright (C) 2003-2005 GNU Gadu Team 
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

enum 
{
	GGADU_DOCKAPP_PROTOCOL,
	GGADU_DOCKAPP_USERFONT,	
	GGADU_DOCKAPP_COLOR_ONLINE,
	GGADU_DOCKAPP_COLOR_AWAY,	
	GGADU_DOCKAPP_COLOR_OFFLINE,	
	GGADU_DOCKAPP_COLOR_BACK,	
};

#define GGADU_DOCKAPP_STATUS_ONLINE	1
#define GGADU_DOCKAPP_STATUS_AWAY	2
#define GGADU_DOCKAPP_STATUS_OFFLINE	3
#define GGADU_DOCKAPP_STATUS_UNKNOWN	4
