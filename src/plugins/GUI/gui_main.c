/* $Id: gui_main.c,v 1.39 2004/01/18 21:13:25 krzyzak Exp $ */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "ggadu_conf.h"
#include "signals.h"
#include "support.h"
#include "repo.h"

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
static GQuark GUI_REGISTER_USERLIST_MENU_SIG;
static GQuark GUI_UNREGISTER_USERLIST_MENU_SIG;
static GQuark GUI_ADD_USER_WINDOW_SIG;
static GQuark GUI_CHANGE_USER_WINDOW_SIG;
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

	if (signal->name == GUI_ADD_USER_WINDOW_SIG)
		handle_add_user_window(signal);
	else if (signal->name == GUI_SHOW_DIALOG_SIG)
		handle_show_dialog(signal);
	else if (signal->name == GUI_SHOW_WINDOW_WITH_TEXT_SIG)
		handle_show_window_with_text(signal);
	else if (signal->name == GUI_SHOW_ABOUT_SIG)
		handle_show_about(signal);
	else if (signal->name == GUI_CHANGE_USER_WINDOW_SIG)
		handle_change_user_window(signal);
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

	gtk_init(NULL, NULL);
	gtk_window_set_auto_startup_notification(FALSE);

	GGadu_PLUGIN_ACTIVATE(conf_ptr);

	print_debug("%s : initialize", "main-gui");

	gui_handler = (GGaduPlugin *) register_plugin("main-gui", "GTK+2 GUI");

	register_signal_receiver((GGaduPlugin *) gui_handler, (signal_func_ptr) gui_signal_receive);

	if (g_getenv("CONFIG_DIR") || g_getenv("HOME_ETC"))
		this_configdir =
			g_build_filename(g_get_home_dir(),
					 g_getenv("CONFIG_DIR") ? g_getenv("CONFIG_DIR") : g_getenv("HOME_ETC"), "gg2",
					 NULL);
	else
		this_configdir = g_build_filename(g_get_home_dir(), ".gg2", NULL);

	ggadu_config_set_filename((GGaduPlugin *) gui_handler, g_build_filename(this_configdir, "gui", NULL));
	g_free(this_configdir);

	ggadu_config_var_add(gui_handler, "theme", VAR_STR);
	ggadu_config_var_add(gui_handler, "emot", VAR_BOOL);
	ggadu_config_var_add(gui_handler, "icons", VAR_STR);
	ggadu_config_var_add(gui_handler, "tree", VAR_BOOL);
	ggadu_config_var_add(gui_handler, "chat_window_auto_raise", VAR_BOOL);
	ggadu_config_var_add(gui_handler, "use_xosd_for_new_msgs", VAR_BOOL);
	ggadu_config_var_add(gui_handler, "chat_type", VAR_INT);
	ggadu_config_var_add(gui_handler, "chat_window_auto_show", VAR_BOOL);
	ggadu_config_var_add(gui_handler, "chat_paned_size", VAR_INT);
	ggadu_config_var_add(gui_handler, "expand", VAR_BOOL);
	ggadu_config_var_add(gui_handler, "show_active", VAR_BOOL);
	ggadu_config_var_add(gui_handler, "width", VAR_INT);
	ggadu_config_var_add(gui_handler, "height", VAR_INT);
	ggadu_config_var_add(gui_handler, "top", VAR_INT);
	ggadu_config_var_add(gui_handler, "left", VAR_INT);
	ggadu_config_var_add(gui_handler, "send_on_enter", VAR_BOOL);
	ggadu_config_var_add_with_default(gui_handler, "msg_header_color", VAR_STR, g_strdup("blue"));
	ggadu_config_var_add_with_default(gui_handler, "msg_header_font", VAR_STR, g_strdup("bold"));
	ggadu_config_var_add_with_default(gui_handler, "msg_out_header_color", VAR_STR, g_strdup("brown"));
	ggadu_config_var_add_with_default(gui_handler, "msg_out_header_font", VAR_STR, g_strdup("bold"));
	ggadu_config_var_add(gui_handler, "msg_body_font", VAR_STR);
	ggadu_config_var_add(gui_handler, "msg_body_color", VAR_STR);
	ggadu_config_var_add(gui_handler, "msg_out_body_color", VAR_STR);
	ggadu_config_var_add(gui_handler, "msg_out_body_font", VAR_STR);
	ggadu_config_var_add(gui_handler, "hide_on_start", VAR_BOOL);
	ggadu_config_var_add(gui_handler, "hide_toolbar", VAR_BOOL);
	ggadu_config_var_add_with_default(gui_handler, "sound_msg_in", VAR_STR,
					  g_strconcat(PACKAGE_DATA_DIR, "/sounds/", NULL));
	ggadu_config_var_add_with_default(gui_handler, "sound_msg_out", VAR_STR,
					  g_strconcat(PACKAGE_DATA_DIR, "/sounds/", NULL));
	ggadu_config_var_add(gui_handler, "contact_list_contact_font", VAR_STR);
	ggadu_config_var_add(gui_handler, "contact_list_protocol_font", VAR_STR);
	ggadu_config_var_add_with_default(gui_handler, "chat_window_width", VAR_INT,
					  (gpointer) DEFAULT_CHAT_WINDOW_WIDTH);
	ggadu_config_var_add_with_default(gui_handler, "chat_window_height", VAR_INT,
					  (gpointer) DEFAULT_CHAT_WINDOW_HEIGHT);
	ggadu_config_var_add(gui_handler, "blink", VAR_BOOL);
	ggadu_config_var_add(gui_handler, "blink_interval", VAR_INT);
	ggadu_config_var_add(gui_handler, "auto_away", VAR_BOOL);
	ggadu_config_var_add(gui_handler, "auto_away_interval", VAR_INT);
	ggadu_config_var_add(gui_handler, "use_username", VAR_BOOL);

	if (!ggadu_config_read(gui_handler))
		g_warning(_("Unable to read configuration file for plugin GUI"));

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
	GUI_REGISTER_USERLIST_MENU_SIG = register_signal(gui_handler, "gui register userlist menu");	/* !!! DODAC FREE */
	GUI_UNREGISTER_USERLIST_MENU_SIG = register_signal(gui_handler, "gui unregister userlist menu");	/* !!! o to chodzi? ;) */

	GUI_ADD_USER_WINDOW_SIG = register_signal(gui_handler, "gui add user window");
	GUI_CHANGE_USER_WINDOW_SIG = register_signal(gui_handler, "gui change user window");

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
	sigdata = g_slist_append(sigdata, "GNU Gadu 2");

	signal_emit_full("main-gui", "docklet set default icon", sigdata, NULL, (gpointer) g_slist_free);

}

void destroy_plugin()
{
	print_debug("destroy_plugin %s", GGadu_PLUGIN_NAME);

	gtk_widget_destroy(GTK_WIDGET(window));
}
