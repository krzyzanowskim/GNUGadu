/* $Id: gui_handlers.h,v 1.18 2004/12/20 09:15:14 krzyzak Exp $ */

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

#include "signals.h"
#include "gui_main.h"

void handle_add_user_window(GGaduSignal * signal);
void handle_show_dialog(GGaduSignal * signal);
void handle_show_window_with_text(GGaduSignal * signal);
void handle_show_about(GGaduSignal * signal);
void handle_change_user_window(GGaduSignal * signal);
#ifdef PERL_EMBED
void perl_gui_msg_receive(GGaduSignal * signal, gchar * perl_func, void *pperl);
#endif
void handle_msg_receive(GGaduSignal * signal);
void handle_show_invisible_chats(GGaduSignal * signal);
void handle_register_protocol(GGaduSignal * signal);
void handle_unregister_protocol(GGaduSignal * signal);
void handle_register_menu(GGaduSignal * signal);
void handle_unregister_menu(GGaduSignal * signal);
void handle_send_userlist(GGaduSignal * signal);
void handle_auth_request(GGaduSignal * signal);
void handle_auth_request_accepted(GGaduSignal * signal);
void handle_unauth_request(GGaduSignal * signal);
void handle_unauth_request_accepted(GGaduSignal * signal);
void handle_show_warning(GGaduSignal * signal);
void handle_show_message(GGaduSignal * signal);
void handle_notify(GGaduSignal * signal);
void handle_disconnected(GGaduSignal * signal);
void handle_change_icon(GGaduSignal * signal);
void handle_show_search_results(GGaduSignal * signal);
void handle_status_changed(GGaduSignal * signal);
void handle_null(GGaduSignal * signal);

void notify_callback(gchar * repo_name, gpointer key, gint actions);


