/* $Id: plugin_sound_esd.c,v 1.17 2004/12/20 09:15:40 krzyzak Exp $ */

/* 
 * sound-ESD plugin for GNU Gadu 2 
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

#include <glib.h>
#include <esd.h>

#include "ggadu_types.h"
#include "plugins.h"
#include "signals.h"
#include "ggadu_menu.h"
#include "ggadu_support.h"

GGaduPlugin *handler;

GGadu_PLUGIN_INIT("sound-esd", GGADU_PLUGIN_TYPE_MISC);

static GQuark SOUND_PLAY_FILE_SIG;

gpointer ggadu_play_file(gpointer filename)
{
    static GStaticMutex play_mutex = G_STATIC_MUTEX_INIT;
    gchar *filename_native = NULL;
    gsize r,w;
    
    g_static_mutex_lock(&play_mutex);

    filename_native = g_filename_from_utf8(filename,-1,&r,&w,NULL);
    if (!esd_play_file(g_get_prgname(), filename_native, TRUE))
    {
	signal_emit_from_thread(GGadu_PLUGIN_NAME, "gui show message", g_strdup(_("ESD plugin: Problems while playing file")), "main-gui");
    }

    g_free(filename_native);
    g_static_mutex_unlock(&play_mutex);
    return NULL;
}

void my_signal_receive(gpointer name, gpointer signal_ptr)
{
    GGaduSignal *signal = (GGaduSignal *) signal_ptr;

    if (signal && signal->name == SOUND_PLAY_FILE_SIG)
    {
	gchar *filename = signal->data;

        print_debug("%s : receive signal %d", GGadu_PLUGIN_NAME, signal->name);

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
