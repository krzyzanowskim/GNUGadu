/* $Id: plugin_sound_esd.c,v 1.2 2003/04/03 09:14:50 thrulliq Exp $ */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <esd.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "menu.h"
#include "support.h"

GGaduPlugin *handler;

GGadu_PLUGIN_INIT("sound-esd", GGADU_PLUGIN_TYPE_MISC);

gpointer ggadu_play_file(gpointer user_data)
{
    
    esd_play_file(g_get_prgname(), user_data, 1);
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
	    {
		g_thread_create(ggadu_play_file, filename, FALSE, NULL);
	    }
	    return;
	}
    return;
}


void start_plugin() {
}


GGaduPlugin *initialize_plugin(gpointer conf_ptr) {
    print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);
    
    GGadu_PLUGIN_ACTIVATE(conf_ptr);

    handler = (GGaduPlugin *)register_plugin(GGadu_PLUGIN_NAME,_("ESD sound driver"));

    register_signal(handler,"sound play file");

    register_signal_receiver((GGaduPlugin *)handler, (signal_func_ptr)my_signal_receive);
    
    return handler;
}

void destroy_plugin() {
    print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
}
