/* $Id: gui_main.c,v 1.69 2004/12/27 12:12:22 krzyzak Exp $ */

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "ggadu_types.h"
#include "plugins.h"
#include "ggadu_conf.h"
#include "signals.h"
#include "ggadu_support.h"
#include "ggadu_repo.h"

#include "GUI_plugin.h"
#include "gui_support.h"
#include "gui_main.h"
#include "gui_userview.h"
#include "gui_handlers.h"

GGaduPlugin *gui_handler = NULL;

GSList *protocols = NULL;
GSList *emoticons = NULL;
GSList *invisible_chats;

GtkWidget *window = NULL;
gboolean tree = FALSE;

static GQuark GUI_REGISTER_PROTOCOL_SIG;
static GQuark GUI_UNREGISTER_PROTOCOL_SIG;
static GQuark GUI_REGISTER_MENU_SIG;
static GQuark GUI_UNREGISTER_MENU_SIG;
static GQuark GUI_SEND_USERLIST_SIG;
static GQuark GUI_MSG_RECEIVE_SIG;
static GQuark GUI_SHOW_WARNING_SIG;
static GQuark GUI_SHOW_MESSAGE_SIG;
static GQuark GUI_DISCONNECTED_SIG;
static GQuark GUI_SHOW_DIALOG_SIG;
static GQuark GUI_SHOW_WINDOW_WITH_TEXT_SIG;
static GQuark GUI_SHOW_ABOUT_SIG;
static GQuark GUI_SHOW_SEARCH_RESULTS_SIG;
static GQuark GUI_STATUS_CHANGED_SIG;
static GQuark GUI_SHOW_INVISIBLE_CHATS_SIG;

GGadu_PLUGIN_INIT("main-gui", GGADU_PLUGIN_TYPE_UI);

void gui_signal_receive(gpointer name, gpointer signal_ptr)
{
	GGaduSignal *signal = (GGaduSignal *) signal_ptr;

	print_debug("%s : receive signal %d", "main-gui", signal->name);

	if (signal->name == GUI_SHOW_DIALOG_SIG)
		handle_show_dialog(signal);
	else if (signal->name == GUI_SHOW_WINDOW_WITH_TEXT_SIG)
		handle_show_window_with_text(signal);
	else if (signal->name == GUI_SHOW_ABOUT_SIG)
		handle_show_about(signal);
	else if (signal->name == GUI_MSG_RECEIVE_SIG)
		handle_msg_receive(signal);
	else if (signal->name == GUI_SHOW_INVISIBLE_CHATS_SIG)
		handle_show_invisible_chats(signal);
	else if (signal->name == GUI_REGISTER_PROTOCOL_SIG)
		handle_register_protocol(signal);
	else if (signal->name == GUI_UNREGISTER_PROTOCOL_SIG)
		handle_unregister_protocol(signal);
	else if (signal->name == GUI_REGISTER_MENU_SIG)
		handle_register_menu(signal);
	else if (signal->name == GUI_UNREGISTER_MENU_SIG)
		handle_unregister_menu(signal);
	else if (signal->name == GUI_SEND_USERLIST_SIG)
		handle_send_userlist(signal);
	else if (signal->name == GUI_SHOW_WARNING_SIG)
		handle_show_warning(signal);
	else if (signal->name == GUI_SHOW_MESSAGE_SIG)
		handle_show_message(signal);
	else if (signal->name == GUI_DISCONNECTED_SIG)
		handle_disconnected(signal);
	else if (signal->name == GUI_SHOW_SEARCH_RESULTS_SIG)
		handle_show_search_results(signal);
	else if (signal->name == GUI_STATUS_CHANGED_SIG)
		handle_status_changed(signal);
}

GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
	gchar *this_configdir = NULL;
	gchar *path = NULL;

	/*gdk_threads_init();*/
	gtk_init(&config->argc,&config->argv);
	gtk_window_set_auto_startup_notification(FALSE);

	GGadu_PLUGIN_ACTIVATE(conf_ptr);

	print_debug("%s : initialize", "main-gui");

	gui_handler = (GGaduPlugin *) register_plugin("main-gui", "GTK+2 GUI");

	register_signal_receiver((GGaduPlugin *) gui_handler, (signal_func_ptr) gui_signal_receive);

	if (g_getenv("HOME_ETC"))
		this_configdir = g_build_filename(g_getenv("HOME_ETC"), "gg2", NULL);
	else
		this_configdir = g_build_filename(g_get_home_dir(), ".gg2", NULL);

	path = g_build_filename(this_configdir, "gui", NULL);
	ggadu_config_set_filename((GGaduPlugin *) gui_handler, path);
	g_free(this_configdir);
	g_free(path);

	ggadu_config_var_add_with_default(gui_handler, "theme", VAR_STR, g_strdup("default"));
	ggadu_config_var_add_with_default(gui_handler, "emot", VAR_BOOL, (gpointer) TRUE);
	ggadu_config_var_add(gui_handler, "icons", VAR_STR);
	ggadu_config_var_add(gui_handler, "tree", VAR_BOOL);
	ggadu_config_var_add(gui_handler, "chat_window_auto_raise", VAR_BOOL);
	ggadu_config_var_add_with_default(gui_handler, "use_xosd_for_new_msgs", VAR_BOOL, (gpointer)TRUE);
	ggadu_config_var_add(gui_handler, "chat_type", VAR_INT);
#ifdef USE_GTKSPELL
	ggadu_config_var_add(gui_handler, "use_spell", VAR_BOOL);
	ggadu_config_var_add(gui_handler, "dictionary", VAR_STR);
#endif
	ggadu_config_var_add(gui_handler, "chat_window_auto_show", VAR_BOOL);
	ggadu_config_var_add_with_default(gui_handler, "chat_paned_size", VAR_INT,(gpointer) 80);
	ggadu_config_var_add_with_default(gui_handler, "expand", VAR_BOOL, (gpointer) TRUE);
	ggadu_config_var_add(gui_handler, "show_active", VAR_BOOL);
	ggadu_config_var_add(gui_handler, "width", VAR_INT);
	ggadu_config_var_add(gui_handler, "height", VAR_INT);
	ggadu_config_var_add(gui_handler, "top", VAR_INT);
	ggadu_config_var_add(gui_handler, "left", VAR_INT);
	ggadu_config_var_add_with_default(gui_handler, "send_on_enter", VAR_BOOL, (gpointer) TRUE);
	/* colors and fonts*/
	ggadu_config_var_add_with_default(gui_handler, "msg_header_color", VAR_STR, g_strdup("blue"));
	ggadu_config_var_add_with_default(gui_handler, "msg_header_font", VAR_STR, g_strdup("bold"));
	ggadu_config_var_add_with_default(gui_handler, "msg_out_header_color", VAR_STR, g_strdup("brown"));
	ggadu_config_var_add_with_default(gui_handler, "msg_out_header_font", VAR_STR, g_strdup("bold"));
	ggadu_config_var_add_with_default(gui_handler, "msg_body_color", VAR_STR,g_strdup("black"));
	ggadu_config_var_add(gui_handler, "msg_body_font", VAR_STR);
	ggadu_config_var_add_with_default(gui_handler, "msg_out_body_color", VAR_STR,g_strdup("black"));
	ggadu_config_var_add(gui_handler, "msg_out_body_font", VAR_STR);
	
	ggadu_config_var_add(gui_handler, "hide_on_start", VAR_BOOL);
	ggadu_config_var_add_with_default(gui_handler, "close_on_esc", VAR_BOOL, (gpointer) FALSE);
	ggadu_config_var_add_with_default(gui_handler, "notify_status_changes", VAR_BOOL, (gpointer) TRUE);
 	ggadu_config_var_add_with_default(gui_handler, "use_xosd_for_status_change", VAR_BOOL, (gpointer)TRUE);
	ggadu_config_var_add_with_default(gui_handler, "show_toolbar", VAR_BOOL, (gpointer) TRUE);
	ggadu_config_var_add_with_default(gui_handler, "sound_msg_in", VAR_STR,
					  g_strconcat(PACKAGE_DATA_DIR, "/sounds/msg.wav", NULL));
	ggadu_config_var_add_with_default(gui_handler, "sound_msg_in_first", VAR_STR,
					  g_strconcat(PACKAGE_DATA_DIR, "/sounds/", NULL));
	ggadu_config_var_add_with_default(gui_handler, "sound_msg_out", VAR_STR,
					  g_strconcat(PACKAGE_DATA_DIR, "/sounds/", NULL));
	ggadu_config_var_add(gui_handler, "contact_list_contact_font", VAR_STR);
	ggadu_config_var_add(gui_handler, "contact_list_protocol_font", VAR_STR);
	ggadu_config_var_add_with_default(gui_handler, "chat_window_width", VAR_INT,
					  (gpointer) DEFAULT_CHAT_WINDOW_WIDTH);
	ggadu_config_var_add_with_default(gui_handler, "chat_window_height", VAR_INT,
					  (gpointer) DEFAULT_CHAT_WINDOW_HEIGHT);
	ggadu_config_var_add_with_default(gui_handler, "blink", VAR_BOOL, (gpointer) TRUE);
	ggadu_config_var_add_with_default(gui_handler, "blink_interval", VAR_INT, (gpointer) 200);
	ggadu_config_var_add_with_default(gui_handler, "use_username", VAR_BOOL, (gpointer) TRUE);
	ggadu_config_var_add_with_default(gui_handler, "descr_on_list", VAR_BOOL, (gpointer) TRUE);

	if (!ggadu_config_read(gui_handler))
		g_warning(_("Unable to read configuration file for plugin GUI, don't worry"));

	/* 
	 * SIGNAL : gui register menu
	 * Rejestruje menu w oknie glownym wieceij info na ten temat w dokumentacji do gtk2.
	 * przeslany parametr powinien byc wskaznikiem do GtkItemFactoryEntry menu_for_gui[] = {}
	 * 
	 */
	GUI_REGISTER_PROTOCOL_SIG = register_signal(gui_handler, "gui register protocol");
	GUI_UNREGISTER_PROTOCOL_SIG = register_signal(gui_handler, "gui unregister protocol");

	/*
	 * SIGNAL : gui register menu
	 * rejestruje menu ktore jest dodawane do glownego menu
	 */
	GUI_REGISTER_MENU_SIG = register_signal(gui_handler, "gui register menu");
	GUI_UNREGISTER_MENU_SIG = register_signal(gui_handler, "gui unregister menu");

	/* przesyla cala liste kontaktow */
	GUI_SEND_USERLIST_SIG = register_signal(gui_handler, "gui send userlist");

	/* 
	 * SIGNAL : gui msg receive
	 * przesyla wiadomosc
	 *
	 */
	GUI_MSG_RECEIVE_SIG = register_signal(gui_handler, "gui msg receive");	/* !!! DODAC FREE */
#ifdef PERL_EMBED
	register_signal_perl("gui msg receive", perl_gui_msg_receive);
#endif

	GUI_SHOW_INVISIBLE_CHATS_SIG = register_signal(gui_handler, "gui show invisible chats");

	/*
	 * SIGNAL : conn failed 
	 * 
	 * Wyswietlanie okienek dialogowych z ostrzezeniami lub wiadomosciami
	 */
	GUI_SHOW_WARNING_SIG = register_signal(gui_handler, "gui show warning");
	GUI_SHOW_MESSAGE_SIG = register_signal(gui_handler, "gui show message");

	/*
	 * SIGNAL : gui disconnected
	 * z jakich¶ przyczyn nast±pi³o roz³±czenie z serwerem
	 */
	GUI_DISCONNECTED_SIG = register_signal(gui_handler, "gui disconnected");
	GUI_SHOW_DIALOG_SIG = register_signal(gui_handler, "gui show dialog");

	/* pokazuje okno z tekstem przeslanym, nic wiêcej */
	GUI_SHOW_WINDOW_WITH_TEXT_SIG = register_signal(gui_handler, "gui show window with text");

	/* pokazuje okno z informacjami o programie */
	GUI_SHOW_ABOUT_SIG = register_signal(gui_handler, "gui show about");

	/* pokazuje okno z wynikami wyszukiwania uzytkownikow */
	GUI_SHOW_SEARCH_RESULTS_SIG = register_signal(gui_handler, "gui show search results");

	/* przede wszystkim zmienia ikonki ;) */
	GUI_STATUS_CHANGED_SIG = register_signal(gui_handler, "gui status changed");
	
	ggadu_repo_watch_add(NULL, REPO_ACTION_VALUE_CHANGE, REPO_VALUE_CONTACT, notify_callback);

	return gui_handler;
}

void start_plugin()
{
	GSList *sigdata = NULL;

	gui_build_default_menu();
	gui_build_default_toolbar();

	if (ggadu_config_var_get(gui_handler, "tree"))
		tree = TRUE;

	if (ggadu_config_var_get(gui_handler, "hide_on_start") && find_plugin_by_pattern("docklet*"))
		gui_main_window_create(FALSE);
	else
		gui_main_window_create(TRUE);

	print_debug("%s : start_plugin", "main-gui");

	config->send_on_enter = TRUE;
	
	if ((ggadu_config_var_get(gui_handler, "theme")))
		gui_load_theme();
	else
		print_debug("%s : No theme variable set, using defaults", "main-gui");

	gui_config_emoticons();

	sigdata = g_slist_append(sigdata, (gchar *) ggadu_config_var_get(gui_handler, "icons"));
	sigdata = g_slist_append(sigdata, GGADU_DEFAULT_ICON_FILENAME);
	sigdata = g_slist_append(sigdata, "GNU Gadu");

	signal_emit_full("main-gui", "docklet set default icon", sigdata, NULL, (gpointer) g_slist_free);

}

void destroy_plugin()
{
	print_debug("destroy_plugin %s", GGadu_PLUGIN_NAME);

	gtk_widget_destroy(GTK_WIDGET(window));
}
