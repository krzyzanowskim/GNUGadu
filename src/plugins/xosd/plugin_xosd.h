/* $Id: plugin_xosd.h,v 1.7 2004/12/20 09:15:43 krzyzak Exp $ */

/* 
 * XOSD plugin for GNU Gadu 2 
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

#ifndef PLUGIN_XOSD_H
#define PLGUIN_XOSD_H 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define GGADU_XOSD_WELCOME_STRING "GNU Gadu 2"

#define GGADU_XOSD_DEFAULT_FONT "-*-*-*-r-*-*-20-*-*-*-*-*-iso8859-2"
#define GGADU_XOSD_DEFAULT_COLOUR "#67FF40"
#define GGADU_XOSD_DEFAULT_NUMLINES 5
#define GGADU_XOSD_DEFAULT_TIMEOUT 5
#define GGADU_XOSD_DEFAULT_SHADOW_OFFSET 1
#define GGADU_XOSD_DEFAULT_HORIZONTAL_OFFSET 0
#define GGADU_XOSD_DEFAULT_VERTICAL_OFFSET 0
#define GGADU_XOSD_DEFAULT_ALIGN XOSD_center
#define GGADU_XOSD_DEFAULT_POS XOSD_top

enum
{
    GGADU_XOSD_CONFIG_COLOUR,
    GGADU_XOSD_CONFIG_NUMLINES,
    GGADU_XOSD_CONFIG_FONT,
    GGADU_XOSD_CONFIG_TIMEOUT,
    GGADU_XOSD_CONFIG_SHADOW_OFFSET,
    GGADU_XOSD_CONFIG_HORIZONTAL_OFFSET,
    GGADU_XOSD_CONFIG_VERTICAL_OFFSET,
    GGADU_XOSD_CONFIG_ALIGN,
    GGADU_XOSD_CONFIG_POS,
    GGADU_XOSD_CONFIG_TIMESTAMP
};

gint set_configuration(void);

#endif /* PLUGIN_XOSD_H */
