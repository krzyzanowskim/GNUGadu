/* $Id: plugin_sound_esd.c,v 1.6 2004/01/17 00:45:04 shaster Exp $ */

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

static GQuark SOUND_PLAY_FILE_SIG;

gpointer ggadu_play_file(gpointer user_data)
{
    esd_play_file(g_get_prgname(), user_data, 1);
    return NULL;
}

void my_signal_receive(gpointer name, gpointer signal_ptr)
{
    GGaduSignal *signal = (GGaduSignal *) signal_ptr;

    print_debug("%s : receive signal %d", GGadu_PLUGIN_NAME, signal->name);

    if (signal->name == SOUND_PLAY_FILE_SIG)
    {
	gchar *filename = signal->data;

	if ((filename != NULL) && g_file_test(filename, G_FILE_TEST_IS_REGULAR))
	    g_thread_create(ggadu_play_file, filename, FALSE, NULL);

    }
    return;
}

void start_plugin()
{
}

GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
    print_debug("%s : initialize", GGadu_PLUGIN_NAME);

    GGadu_PLUGIN_ACTIVATE(conf_ptr);

    handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("ESD sound driver"));

    SOUND_PLAY_FILE_SIG = register_signal(handler, "sound play file");

    register_signal_receiver((GGaduPlugin *) handler, (signal_func_ptr) my_signal_receive);

    return handler;
}

void destroy_plugin()
{
    print_debug("destroy_plugin %s", GGadu_PLUGIN_NAME);
}
