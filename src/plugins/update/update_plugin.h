/* $Id: update_plugin.h,v 1.4 2004/01/27 01:23:24 shaster Exp $ */

/* 
 * Update plugin for GNU Gadu 2 
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

#ifndef UPDATE_PLUGIN_H
#define UPDATE_PLUGIN_H 1

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#define GGADU_UPDATE_USERAGENT_STRING "GNU Gadu 2 " VERSION " update plugin"
#define GGADU_UPDATE_SERVER "sourceforge.net"
#define GGADU_UPDATE_URL "/export/rss2_projfiles.php?group_id=76206"
#define GGADU_UPDATE_PORT 80

#define GGADU_UPDATE_BUFLEN 8192

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
