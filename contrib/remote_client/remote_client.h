/* $Id: remote_client.h,v 1.3 2004/01/28 23:39:21 shaster Exp $ */

/* 
 * Example: client for GNU Gadu 2 remote plugin
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

#ifndef REMOTE_CLIENT_H
#define REMOTE_CLIENT_H

int remote_init(void);
int remote_send(char *text);
int remote_send_data(char *data, int len);
int remote_close(void);
int remote_fd(void);

int remote_sig_quit(void);
int remote_sig_gui_show(void);
int remote_sig_gui_warn(char *text);
int remote_sig_gui_msg(char *text);
int remote_sig_gui_notify(char *text);
int remote_sig_xosd_show(char *text);
int remote_sig_remote_get_icon(char **filepath);
int remote_sig_remote_get_icon_path(char **filepath);

#endif
