/* $Id: plugin_sound_arts.c,v 1.10 2004/10/13 13:34:30 krzyzak Exp $ */

/* 
 * sound-aRts plugin for GNU Gadu 2 
 * 
 * Copyright (C) 2003-2004 GNU Gadu Team 
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

#include <glib.h>
#include <artsc.h>

#include "ggadu_types.h"
#include "plugins.h"
#include "signals.h"
#include "ggadu_menu.h"
#include "ggadu_support.h"
#include "sound-arts.h"

GGaduPlugin *handler;

GGadu_PLUGIN_INIT("sound-arts", GGADU_PLUGIN_TYPE_MISC);

static gpointer ggadu_play_file(gpointer filename)
{
    static GStaticMutex play_mutex = G_STATIC_MUTEX_INIT;
    gchar *filename_native = NULL;
    gsize r,w;

    g_static_mutex_lock(&play_mutex);

    filename_native = g_filename_from_utf8(filename,-1,&r,&w,NULL);
    arts_play_file(filename_native);
    g_free(filename_native);

    g_static_mutex_unlock(&play_mutex);
    g_thread_exit(0);
    return NULL;
}

void my_signal_receive(gpointer name, gpointer signal_ptr)
{
    GGaduSignal *signal = (GGaduSignal *) signal_ptr;

    if (signal && signal->name == g_quark_from_static_string("sound play file"))
    {
	gchar *filename = signal->data;

        print_debug("%s : receive signal %d\n", GGadu_PLUGIN_NAME, signal->name);

	if ((filename != NULL) && g_file_test(filename, G_FILE_TEST_IS_REGULAR))
	    g_thread_create(ggadu_play_file, filename, FALSE, NULL);
    }
}

void start_plugin()
{
    int err = arts_init();
    
    if (err != 0)
    {
	signal_emit(GGadu_PLUGIN_NAME, "gui show message", g_strdup(arts_error_text(err)), "main-gui");
    }
	
}

GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
    print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);

    GGadu_PLUGIN_ACTIVATE(conf_ptr);

    handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("aRts sound driver"));

    register_signal(handler, "sound play file");

    register_signal_receiver((GGaduPlugin *) handler, (signal_func_ptr) my_signal_receive);

    return handler;
}

void destroy_plugin()
{
    print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
    arts_free();
}
