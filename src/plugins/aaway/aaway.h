/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef PLUGIN_AAWAY_H
#define PLUGIN_AAWAY_H 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define GGADU_AAWAY_CONFIG_DEFAULT_ENABLE_AUTOAWAY 0
#define GGADU_AAWAY_CONFIG_DEFAULT_INTERVAL 5
#define GGADU_AAWAY_CONFIG_DEFAULT_ENABLE_AWAY_MSG 0
#define GGADU_AAWAY_CONFIG_DEFAULT_AWAY_MSG " Auto away ..."

enum
{
	GGADU_AAWAY_CONFIG_ENABLE_AUTOAWAY,
	GGADU_AAWAY_CONFIG_INTERVAL,
	GGADU_AAWAY_CONFIG_ENABLE_AWAY_MSG,
	GGADU_AAWAY_CONFIG_AWAY_MSG
};

#include "gg2_core.h"

#define TLEN_STATUS_AVAILABLE   2
#define TLEN_STATUS_EXT_AWAY    3
#define TLEN_STATUS_AWAY        4
#define TLEN_STATUS_DND         5
#define TLEN_STATUS_CHATTY      6
#define TLEN_STATUS_INVISIBLE   7
#define TLEN_STATUS_UNAVAILABLE 8
#define TLEN_STATUS_DESC		9

#define GG_STATUS_NOT_AVAIL 	  0x0001
#define GG_STATUS_NOT_AVAIL_DESCR 0x0015
#define GG_STATUS_AVAIL 	  		0x0002
#define GG_STATUS_AVAIL_DESCR 	  0x0004
#define GG_STATUS_BUSY 		  		0x0003
#define GG_STATUS_BUSY_DESCR 	  0x0005
#define GG_STATUS_INVISIBLE 	  0x0014
#define GG_STATUS_INVISIBLE_DESCR 0x0016
#define GG_STATUS_BLOCKED 	0x0006


#define JABBER_STATUS_UNAVAILABLE 0
#define JABBER_STATUS_AVAILABLE   1
#define JABBER_STATUS_CHAT	  2
#define JABBER_STATUS_AWAY	  3
#define JABBER_STATUS_XA	  4
#define JABBER_STATUS_DND	  5
#define JABBER_STATUS_ERROR   6
#define JABBER_STATUS_DESCR   7


gint set_configuration(void);

#endif /* PLUGIN_AAWAY_H */
