/* $Id: gui_main.c,v 1.5 2003/04/01 15:38:40 zapal Exp $ */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "support.h"

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

GGadu_PLUGIN_INIT("main-gui",GGADU_PLUGIN_TYPE_UI);

static
gui_signal_handler handlers[] = {
	{"gui add user window", handle_add_user_window},
	{"gui show dialog", handle_show_dialog},
	{"gui show window with text", handle_show_window_with_text},
	{"gui change user window", handle_change_user_window},
	{"gui msg receive", handle_msg_receive},
	{"gui show invisible chats", handle_show_invisible_chats},
	{"gui register protocol", handle_register_protocol},
	{"gui unregister protocol", handle_unregister_protocol},
	{"gui register menu", handle_register_menu},
	{"gui unregister menu", handle_unregister_menu},
	{"gui register userlist menu", handle_register_userlist_menu},
	{"gui unregister userlist menu", handle_unregister_userlist_menu},
	{"gui send userlist", handle_send_userlist},
	{"auth request", handle_auth_request},
	{"auth request accepted", NULL},
	{"unauth request", NULL},
	{"unauth request accepted", NULL},
	{"gui show warning", handle_show_warning},
	{"gui show message", handle_show_message},
	{"gui notify", handle_notify},
	{"gui disconnected", handle_disconnected},
	{"gui show search results", handle_show_search_results},
	{"gui status changed", handle_status_changed},
	{NULL, NULL}
};

void gui_signal_receive(gpointer name, gpointer signal_ptr) 
{
	
	GGaduSignal *signal = (GGaduSignal *)signal_ptr;
	gui_signal_handler *h;
//	gdk_threads_enter();

	print_debug("%s : receive signal %s\n","main-gui",(gchar *)signal->name);
	
	for (h = handlers;h->signal_name; h++) {
		if (!ggadu_strcasecmp(h->signal_name, signal->name))
			(h->handler_func)(signal);
	}
//	gdk_threads_leave();
}


GGaduPlugin *initialize_plugin(gpointer conf_ptr) 
{
//    gdk_threads_init();

    gchar *this_configdir = NULL;

    gtk_init(NULL, NULL);
    
    GGadu_PLUGIN_ACTIVATE(conf_ptr);

    print_debug("%s : initialize\n", "main-gui");

    gui_handler = (GGaduPlugin *)register_plugin("main-gui","GTK+2 GUI");
    
    register_signal_receiver((GGaduPlugin *)gui_handler, (signal_func_ptr)gui_signal_receive);
    
    if (g_getenv("CONFIG_DIR"))
	this_configdir = g_build_filename(g_get_home_dir(),g_getenv("CONFIG_DIR"),"gg2",NULL);
    else
	this_configdir = g_build_filename(g_get_home_dir(),".gg2",NULL);

    set_config_file_name((GGaduPlugin *)gui_handler, g_build_filename(this_configdir,"gui",NULL));

    config_var_add(gui_handler, "theme", VAR_STR);
    config_var_add(gui_handler, "emot", VAR_BOOL);
    config_var_add(gui_handler, "icons", VAR_STR);
    config_var_add(gui_handler, "tree", VAR_BOOL);
    config_var_add(gui_handler, "chat_window_auto_raise", VAR_BOOL);
    config_var_add(gui_handler, "use_xosd_for_new_msgs", VAR_BOOL);
    config_var_add(gui_handler, "chat_type",  VAR_INT);
    config_var_add(gui_handler, "expand", VAR_BOOL);
    config_var_add(gui_handler, "show_active", VAR_BOOL);
    config_var_add(gui_handler, "width", VAR_INT);
    config_var_add(gui_handler, "height", VAR_INT);
    config_var_add(gui_handler, "top", VAR_INT);
    config_var_add(gui_handler, "left", VAR_INT);
    config_var_add(gui_handler, "send_on_enter", VAR_BOOL);
    config_var_add(gui_handler, "msg_header_color", VAR_STR);
    config_var_add(gui_handler, "msg_body_color", VAR_STR);
    config_var_add(gui_handler, "msg_header_font", VAR_STR);
    config_var_add(gui_handler, "msg_body_font", VAR_STR);
    config_var_add(gui_handler, "msg_out_header_color", VAR_STR);
    config_var_add(gui_handler, "msg_out_body_color", VAR_STR);
    config_var_add(gui_handler, "msg_out_header_font", VAR_STR);
    config_var_add(gui_handler, "msg_out_body_font", VAR_STR);
    config_var_add(gui_handler, "hide_on_start", VAR_BOOL);

    if (!config_read(gui_handler))
	g_warning(_("Unable to read configuration file for plugin GUI"));

    /* 
     * SIGNAL : gui register menu
     * Rejestruje menu w oknie glownym wieceij info na ten temat w dokumentacji do gtk2.
     * przeslany parametr powinien byc wskaznikiem do GtkItemFactoryEntry menu_for_gui[] = {}
     * 
     */
    register_signal(gui_handler,"gui register protocol");
    register_signal(gui_handler,"gui unregister protocol");
    
    /*
     * SIGNAL : gui register menu
     * rejestruje menu ktore jest dodawane do glownego menu
     */ 
    register_signal(gui_handler,"gui register menu");
    register_signal(gui_handler,"gui unregister menu");
    
    /* przesyla cala liste kontaktow */
    register_signal(gui_handler,"gui send userlist");
    
    /* 
     * SIGNAL : gui notify
     * powiadamia o zmianie statusu
     */
    register_signal(gui_handler,"gui notify");
    
    /* 
     * SIGNAL : gui msg receive
     * przesyla wiadomosc
     *
     */
    register_signal(gui_handler,"gui msg receive"); /* !!! DODAC FREE */

    register_signal(gui_handler,"gui register userlist menu"); /* !!! DODAC FREE */
    register_signal(gui_handler,"gui unregister userlist menu"); /* !!! o to chodzi? ;) */

    /* 
     * SIGNAL : auth request
     * prosi o autoryzacje (tlen)
     *
     */
    register_signal(gui_handler,"auth request");

    /* 
     * SIGNAL : unauth request
     * prosi o deautoryzacje (tlen)
     *
     */
    register_signal(gui_handler,"unauth request");

    /* 
     * SIGNAL : auth request accepted
     * Ktos zgodzil sie na nasza prosbe o autoryzacje (tlen)
     *
     */
    register_signal(gui_handler,"auth request accepted");

    /* 
     * SIGNAL : unauth request accepted
     * ktos zgodzil sie na nasza prosbe o deautoryzacje (tlen)
     *
     */
    register_signal(gui_handler,"unauth request accepted");

    register_signal(gui_handler,"gui add user window");
    register_signal(gui_handler,"gui change user window");

    register_signal(gui_handler,"gui show invisible chats");
    
    /*
     * SIGNAL : conn failed 
     * 
     * Wyswietlanie okienek dialogowych z ostrzezeniami lub wiadomosciami
     */
    register_signal(gui_handler,"gui show warning"); 
    register_signal(gui_handler,"gui show message"); 
    
    /*
     * SIGNAL : gui disconnected
     * z jakich¶ przyczyn nast±pi³o roz³±czenie z serwerem
     */
    register_signal(gui_handler,"gui disconnected"); 
    register_signal(gui_handler,"gui show dialog"); 
    
    /* pokazuje okno z tekstem przeslanym, nic wiêcej */
    register_signal(gui_handler,"gui show window with text"); 

    /* pokazuje okno z wynikami wyszukiwania uzytkownikow */
    register_signal(gui_handler,"gui show search results"); 

    /* przede wszystkim zmienia ikonki ;) */
    register_signal(gui_handler,"gui status changed"); 


    return gui_handler;
}

void start_plugin()
{
    GSList *sigdata = NULL;
    
    gui_build_default_menu();

    if (config_var_get(gui_handler, "tree"))
	tree = TRUE;

    if (config_var_get(gui_handler, "hide_on_start") && find_plugin_by_name("docklet"))
	gui_main_window_create(FALSE);
    else
	gui_main_window_create(TRUE);

    print_debug("%s : start_plugin\n","main-gui");

    config->send_on_enter = TRUE;

    if ((config_var_get(gui_handler, "theme")))
	gui_load_theme();
    else
	print_debug("%s : No theme variable set, using defaults\n", "main-gui");
    
    gui_config_emoticons();
    
    sigdata = g_slist_append(sigdata, (gchar *) config_var_get(gui_handler, "icons"));
    sigdata = g_slist_append(sigdata, "online.png");
    sigdata = g_slist_append(sigdata, "GNU Gadu 2");

    signal_emit_full("main-gui", "docklet set default icon", sigdata, NULL, (gpointer)g_slist_free);

//    if (config_var_get(gui_handler, "hide_on_start") && find_plugin_by_name("docklet"))
//	handle_show_invisible_chats (NULL);
	
}

void destroy_plugin() 
{
}
