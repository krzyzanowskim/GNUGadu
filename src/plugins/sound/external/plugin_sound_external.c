/* $Id: plugin_sound_external.c,v 1.4 2003/05/18 10:51:19 zapal Exp $ */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "menu.h"
#include "support.h"
#include "dialog.h"
#include "plugin_sound_external.h"

GGaduPlugin *handler;
GGaduMenu *menu_pluginmenu = NULL;

GGadu_PLUGIN_INIT("sound-external", GGADU_PLUGIN_TYPE_MISC);

gpointer ggadu_play_file(gpointer user_data)
{
    gchar *cmd;
    if (!config_var_get(handler, "player"))
      return NULL;
    cmd = g_strdup_printf("%s %s", (char *)config_var_get(handler, "player"), (gchar *)user_data);
    system(cmd);
    g_free(cmd);
    return NULL;
}

void my_signal_receive(gpointer name, gpointer signal_ptr) 
{
	GGaduSignal *signal = (GGaduSignal *)signal_ptr;
	
        print_debug("%s : receive signal %s\n",GGadu_PLUGIN_NAME,(gchar *)signal->name);

	if (!ggadu_strcasecmp(signal->name,"sound play file")) 
	{
	    gchar *filename = signal->data;
	    
	    if (filename != NULL) 
		g_thread_create(ggadu_play_file, filename, FALSE, NULL);
	}

	if (!ggadu_strcasecmp(signal->name,"update config"))
	{
            GGaduDialog *d = signal->data;
            GSList *tmplist = d->optlist;
	    
	    if (d->response == GGADU_OK) {
	        while (tmplist) 
        	{
    	    	    GGaduKeyValue *kv = (GGaduKeyValue *)tmplist->data;
            	    switch (kv->key)
            	    {
                	case GGADU_SE_CONFIG_PLAYER:
                    	    print_debug("changing var setting player to %s\n", kv->value);
                    	    config_var_set(handler, "player", kv->value);
                    	    break;
		    }
		    tmplist = tmplist->next;
		}
	    }
	    config_save(handler);
	    GGaduDialog_free(d);
	}
}


gpointer se_preferences (gpointer user_data)
{
    GGaduDialog *d = NULL;
    print_debug ("%s: Preferences\n", "Sound external");
    
    d = ggadu_dialog_new();
    ggadu_dialog_set_title(d,_("Sound external preferences"));
    ggadu_dialog_callback_signal(d,"update config");
    ggadu_dialog_set_type(d, GGADU_DIALOG_CONFIG);
    
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SE_CONFIG_PLAYER, _("Player program name"), VAR_STR, config_var_get(handler, "player"), VAR_FLAG_NONE);
    
    signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");
    
    return NULL;
}

GGaduMenu *build_plugin_menu ()
{
    GGaduMenu *root     = ggadu_menu_create();
    GGaduMenu *item_gg  = ggadu_menu_add_item(root,"Sound",NULL,NULL);
    ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Preferences"), se_preferences, NULL) );
    
    return root;   
}


void start_plugin() {
    menu_pluginmenu = build_plugin_menu();
    signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_pluginmenu, "main-gui");
}

GGaduPlugin *initialize_plugin(gpointer conf_ptr) 
{
    gchar *this_configdir;
    
    print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);
    
    GGadu_PLUGIN_ACTIVATE(conf_ptr);
    handler = (GGaduPlugin *)register_plugin(GGadu_PLUGIN_NAME,_("External player sound driver"));
    
    register_signal(handler,"sound play file");
    register_signal(handler,"update config");
    
    if (g_getenv("CONFIG_DIR"))
        this_configdir = g_build_filename(g_get_home_dir(),g_getenv("CONFIG_DIR"),"gg2",NULL);
    else
        this_configdir = g_build_filename(g_get_home_dir(),".gg2",NULL);
    
    set_config_file_name((GGaduPlugin *)handler, g_build_filename(this_configdir,"sound-external",NULL));
    
    g_free(this_configdir);

    config_var_add(handler, "player", VAR_STR);
                     
    if (!config_read(handler))
        g_warning(_("Unable to read configuration file for plugin %s"),"");
	
    register_signal_receiver((GGaduPlugin *)handler, (signal_func_ptr)my_signal_receive);
    
    return handler;
}

void destroy_plugin() {
    print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
    if (menu_pluginmenu)
    {
      signal_emit (GGadu_PLUGIN_NAME, "gui unregister menu", menu_pluginmenu, "main-gui");
    }
}
