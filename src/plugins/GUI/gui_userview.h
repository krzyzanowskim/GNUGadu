/* $Id: gui_userview.h,v 1.2 2003/03/24 20:21:19 krzyzak Exp $ */

#ifndef GGadu_GUI_USERVIEW_H
#define GGadu_GUI_USERVIEW_H 1

#include <gtk/gtk.h>
#include "gui_main.h"

void gui_user_view_clear(gui_protocol *gp);

void gui_user_view_add_userlist(gui_protocol *gp);

void gui_user_view_register(gui_protocol *gp);

void gui_user_view_notify(gui_protocol *gp, GGaduNotify *n);

void gui_create_tree();

void gui_user_view_switch();

void gui_user_view_refresh();

void gui_user_view_clear();

void gui_user_view_unregister(gui_protocol *gp);

#endif
