/* $Id: gui_handlers.h,v 1.6 2003/05/01 20:18:09 shaster Exp $ */

#include "signals.h"

void handle_add_user_window(GGaduSignal *signal);
void handle_show_dialog(GGaduSignal *signal);
void handle_show_window_with_text(GGaduSignal *signal);
void handle_show_about(GGaduSignal *signal);
void handle_change_user_window(GGaduSignal *signal);
void handle_msg_receive(GGaduSignal *signal);
void handle_show_invisible_chats(GGaduSignal *signal);
void handle_register_protocol(GGaduSignal *signal);
void handle_unregister_protocol(GGaduSignal *signal);
void handle_register_menu(GGaduSignal *signal);
void handle_unregister_menu(GGaduSignal *signal);
void handle_register_userlist_menu(GGaduSignal *signal);
void handle_unregister_userlist_menu(GGaduSignal *signal);
void handle_add_user_to_list(GGaduSignal *signal);
void handle_send_userlist(GGaduSignal *signal);
void handle_auth_request(GGaduSignal *signal);
void handle_auth_request_accepted(GGaduSignal *signal);
void handle_unauth_request(GGaduSignal *signal);
void handle_unauth_request_accepted(GGaduSignal *signal);
void handle_show_warning(GGaduSignal *signal);
void handle_show_message(GGaduSignal *signal);
void handle_notify(GGaduSignal *signal);
void handle_disconnected(GGaduSignal *signal);
void handle_change_icon(GGaduSignal *signal);
void handle_show_search_results(GGaduSignal *signal);
void handle_status_changed(GGaduSignal *signal);

void notify_callback (gchar *repo_name, gpointer key, gint actions);
