/* $Id: gui_userview.h,v 1.5 2004/12/20 09:15:15 krzyzak Exp $ */

/* 
 * GUI (gtk+) plugin for GNU Gadu 2 
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

#ifndef GGadu_GUI_USERVIEW_H
#define GGadu_GUI_USERVIEW_H 1

#include <gtk/gtk.h>
#include "gui_main.h"

void gui_user_view_clear(gui_protocol * gp);

void gui_user_view_add_userlist(gui_protocol * gp);

void gui_user_view_register(gui_protocol * gp);

void gui_user_view_notify(gui_protocol * gp, GGaduNotify * n);

void gui_create_tree();

void gui_user_view_switch();

void gui_user_view_refresh();

void gui_user_view_clear();

void gui_user_view_unregister(gui_protocol * gp);

#endif
