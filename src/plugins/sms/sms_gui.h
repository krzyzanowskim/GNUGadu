/* $Id: sms_gui.h,v 1.10 2004/01/27 01:19:54 shaster Exp $ */

/* 
 * SMS plugin for GNU Gadu 2 
 * 
 * Copyright (C) 2003 Bartlomiej Pawlak 
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


#ifndef SMS_PLUGIN_H
#define SMS_PLUGIN_H 1

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#define GGADU_SMS_MAXLEN_NUMBER	512
#define GGADU_SMS_MAXLEN_NICK	512

#define GGADU_SMS_USERLIST_FILENAME	"smslist"
#define GGADU_SMS_USERLIST_TMPFILE	"smslist.tmp_"

enum
{
    GGADU_SMS_CONFIG_SENDER,
    GGADU_SMS_CONFIG_NUMBER,
    GGADU_SMS_CONFIG_BODY,
    GGADU_SMS_CONFIG_EXTERNAL,
    GGADU_SMS_CONFIG_ERA_LOGIN,
    GGADU_SMS_CONFIG_ERA_PASSWORD,
    GGADU_SMS_CONFIG_SHOW_IN_STATUS
};

enum
{
    GGADU_SMS_CONTACT_ID,
    GGADU_SMS_CONTACT_NICK,
    GGADU_SMS_CONTACT_NUMBER
};

enum
{
    GGADU_SMS_METHOD_POPUP,
    GGADU_SMS_METHOD_CHAT
};

GGaduPlugin *initialize_plugin(gpointer conf_ptr);
void signal_receive(gpointer name, gpointer signal_ptr);
void start_plugin();
void destroy_plugin();
void load_smslist();
void save_smslist();

#endif
