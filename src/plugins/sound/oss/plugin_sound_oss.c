/* $Id: plugin_sound_oss.c,v 1.9 2004/05/04 21:39:12 krzyzak Exp $ */

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

#include "ggadu_types.h"
#include "plugins.h"
#include "signals.h"
#include "ggadu_menu.h"
#include "ggadu_support.h"

GGaduPlugin *handler;

GGadu_PLUGIN_INIT("sound-oss", GGADU_PLUGIN_TYPE_MISC);

#define DSPDEV "/dev/dsp"

void oss_play_file(const gchar * file)
{
    int s, w, d = -1;
    gchar buff[4096];
    struct wfmt
    {
	gint32 size;		/* fmt chunk size */
	gchar *data;		/* fmt data */
	gint16 format;		/* format */
	guint16 chan;		/* channels */
	guint32 rate;		/* sample rate */
	guint16 blk;		/* avg block size */
	guint16 sample;		/* bits per sample */
    } fmt;

    for (s = 0; s < 10; s++)
	if ((d = open(DSPDEV, O_WRONLY)) < 0)
	{
	    g_warning("Can't open %s", DSPDEV);
	    usleep(100000);
	}
	else
	    break;
    if (d < 0)
    {
	print_debug("Couldn't open %s", DSPDEV);
	return;
    }

    if ((w = open(file, O_RDONLY)) < 0)
    {
	print_debug("Can't open %s: %s", file, g_strerror(errno));
	return;
    }

    if (read(w, buff, 20) < 20)
    {
	print_debug("Error while reading %s", file);
	return;
    }

    if (strncmp(buff, "RIFF", 4) || strncmp(buff + 8, "WAVE", 4) || strncmp(buff + 12, "fmt ", 4))
    {
	print_debug("Not a RIFF/WAVE file?");
	return;
    }
    memcpy(&fmt.size, buff + 16, 4);
    fmt.data = g_malloc(fmt.size);
    g_assert(fmt.data != NULL);

    if (read(w, fmt.data, fmt.size) < fmt.size)
    {
	print_debug("Error while reading %s", file);
	return;
    }

    memcpy(&fmt.format, fmt.data, 2);
    if (fmt.format != 1)
    {
	print_debug("Unsupported format (not PCM)");
	return;
    }

    memcpy(&fmt.chan, fmt.data + 2, 2);
    if (ioctl(d, SNDCTL_DSP_CHANNELS, &fmt.chan) < 0)
	perror("ioctl(SNDCTL_DSP_CHANNELS)");
    memcpy(&fmt.rate, fmt.data + 4, 4);
    if (ioctl(d, SNDCTL_DSP_SPEED, &fmt.rate) < 0)
	perror("ioctl(SNDCTL_DSP_SPEED)");
    memcpy(&fmt.blk, fmt.data + 12, 2);
    if (ioctl(d, SNDCTL_DSP_GETBLKSIZE, &fmt.blk) < 0)
	perror("ioctl(SNDCTL_DSP_GETBLKSIZE)");
    memcpy(&fmt.sample, fmt.data + 14, 2);
    if (ioctl(d, SNDCTL_DSP_SAMPLESIZE, &fmt.sample) < 0)
	perror("ioctl(SNDCTL_DSP_SAMPLESIZE)");
    g_free(fmt.data);

    if (fmt.sample == 8)
    {
	s = AFMT_S8;
    }
    else if (fmt.sample == 16)
    {
	s = AFMT_S16_NE;
    }
    else
    {
	print_debug("Strange sample size");
	return;
    }
    if (ioctl(d, SNDCTL_DSP_SETFMT, &s) < 0)
	perror("ioctl(SNDCTL_DSP_SETFMT)");

    while ((s = read(w, buff, 4096)) > 0)
	if (write(d, buff, s) == -1)
	{
	    print_debug("Error while writing to %s", DSPDEV);
	    return;
	}
    close(d);
}

gpointer ggadu_play_file(gpointer user_data)
{
    oss_play_file(user_data);
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
