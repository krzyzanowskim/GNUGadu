/* $Id: plugin_sound_arts.c,v 1.1 2003/03/20 10:37:08 krzyzak Exp $ */

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
#include "sound-arts.h"

GGaduPlugin *handler;

GGadu_PLUGIN_INIT("sound-arts", GGADU_PLUGIN_TYPE_MISC);

gpointer ggadu_play_file(gpointer filename) {
    arts_play_file(filename);
    return NULL;
}

void my_signal_receive(gpointer name, gpointer signal_ptr) {
	GGaduSignal *signal = (GGaduSignal *)signal_ptr;
	
        print_debug("%s : receive signal %s\n",GGadu_PLUGIN_NAME,(gchar *)signal->name);

	if (!ggadu_strcasecmp(signal->name,"sound play file")) {
	    gchar *filename = signal->data;
	    
	    if (filename != NULL) {
		g_thread_create(ggadu_play_file, filename, FALSE, NULL);
	    }
	}
}


void start_plugin() {
    register_signal(handler,"sound play file");
}


GGaduPlugin *initialize_plugin(gpointer conf_ptr) {
    print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);
    
    GGadu_PLUGIN_ACTIVATE(conf_ptr);

    handler = (GGaduPlugin *)register_plugin(GGadu_PLUGIN_NAME,_("aRts sound driver"));

    register_signal_receiver((GGaduPlugin *)handler, (signal_func_ptr)my_signal_receive);
    
    return handler;
}

void destroy_plugin() {
    print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
}
