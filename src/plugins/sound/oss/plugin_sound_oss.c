/* $Id: plugin_sound_oss.c,v 1.15 2004/11/12 11:26:27 krzyzak Exp $ */

/* 
 * XOSD plugin for GNU Gadu 2 
 * 
 * Originally written by Igor Truszkowski <pol128@polsl.gliwice.pl>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/soundcard.h>
#include <audiofile.h>

#include "ggadu_types.h"
#include "plugins.h"
#include "signals.h"
#include "ggadu_menu.h"
#include "ggadu_support.h"

GGaduPlugin *handler;

GGadu_PLUGIN_INIT("sound-oss", GGADU_PLUGIN_TYPE_MISC);

#define DSPDEV "/dev/dsp"
#define MAX_BUF_SIZE (4 * 1024)

static gint oss_play_file(gchar *filename)
{
    /* input from libaudiofile... */
    AFfilehandle in_file;
    gint in_format, in_width, in_channels, frame_count;
    double in_rate;
    gint bytes_per_frame;
    gchar buf[ MAX_BUF_SIZE ];
    gint buf_frames;
    gint frames_read;
    gint retry = 0;
    gint dspfile;


    for (retry = 0; retry < 10; retry++)
    {
	if ((dspfile = open(DSPDEV, O_WRONLY)) < 0)
	{
	    g_warning("Can't open %s", DSPDEV);
	    usleep(100000);
	} else
	break;
    }

    if (dspfile < 0)
    {
	print_debug("Couldn't open %s", DSPDEV);
	return 0;
    }
    

    /* open the audio file */
    in_file = afOpenFile( filename, "rb", NULL );
    if ( !in_file )
	return 0;

    /* get audio file parameters */
    frame_count = afGetFrameCount( in_file, AF_DEFAULT_TRACK );
    in_channels = afGetChannels( in_file, AF_DEFAULT_TRACK );
    in_rate = afGetRate( in_file, AF_DEFAULT_TRACK );
    afGetSampleFormat( in_file, AF_DEFAULT_TRACK, &in_format, &in_width );

    bytes_per_frame = ( in_width  * in_channels ) / 8;
    buf_frames = MAX_BUF_SIZE / bytes_per_frame;
    
    /* play a stream */    
    while ( ( frames_read = afReadFrames( in_file, AF_DEFAULT_TRACK, buf, buf_frames ) ) )
    {
	if (write(dspfile,buf,frames_read * bytes_per_frame) <= 0)
	{
	    print_debug("Error while writing to %s", DSPDEV);
	    afCloseFile (in_file);
	    close(dspfile);
	    return 0;
	}
    }
    

    if ( afCloseFile(in_file) || (close(dspfile) == -1))
	return 0;

    return 1;
}


gpointer ggadu_play_file(gpointer filename)
{
    static GStaticMutex play_mutex = G_STATIC_MUTEX_INIT;
    gsize r,w;
    gchar *filename_native = NULL;

    g_static_mutex_lock(&play_mutex);

    filename_native = g_filename_from_utf8(filename,-1,&r,&w,NULL);
    print_debug("oss play file %s\n",filename_native);
    oss_play_file(filename_native);
    g_free(filename_native);

    g_static_mutex_unlock(&play_mutex);
    return NULL;
}

void my_signal_receive(gpointer name, gpointer signal_ptr)
{
    GGaduSignal *signal = (GGaduSignal *) signal_ptr;

    print_debug("%s : receive signal %d\n", GGadu_PLUGIN_NAME, signal->name);

    if (signal->name == g_quark_from_static_string("sound play file"))
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
    print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);

    GGadu_PLUGIN_ACTIVATE(conf_ptr);

    handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("OSS sound driver"));

    register_signal(handler, "sound play file");

    register_signal_receiver((GGaduPlugin *) handler, (signal_func_ptr) my_signal_receive);

    return handler;
}

void destroy_plugin()
{
    print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
}
