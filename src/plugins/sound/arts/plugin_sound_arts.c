/* $Id: plugin_sound_arts.c,v 1.3 2003/06/09 00:20:42 krzyzak Exp $ */

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
	
        print_debug("%s : receive signal %d\n",GGadu_PLUGIN_NAME,signal->name);

	if (signal->name == g_quark_from_static_string("sound play file")) {
	    gchar *filename = signal->data;
	    
	    if (filename != NULL) {
		g_thread_create(ggadu_play_file, filename, FALSE, NULL);
	    }
	}
}


void start_plugin() {
}


GGaduPlugin *initialize_plugin(gpointer conf_ptr) {
    print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);
    
    GGadu_PLUGIN_ACTIVATE(conf_ptr);

    handler = (GGaduPlugin *)register_plugin(GGadu_PLUGIN_NAME,_("aRts sound driver"));

    register_signal(handler,"sound play file");

    register_signal_receiver((GGaduPlugin *)handler, (signal_func_ptr)my_signal_receive);
    
    return handler;
}

void destroy_plugin() {
    print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
}
