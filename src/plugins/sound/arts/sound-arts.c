/* $Id: sound-arts.c,v 1.1 2004/08/24 13:56:00 krzyzak Exp $ */
    /*

       Copyright (C) 2004 Marcin Krzy¿anowski krzak@hakore.com
       Based on esound project sources (http://www.tux.org/~ricdude/EsounD.html)

       This program is free software; you can redistribute it and/or modify
       it under the terms of the GNU General Public License as published by
       the Free Software Foundation; either version 2 of the License, or
       (at your option) any later version.

       This program is distributed in the hope that it will be useful,
       but WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
       GNU General Public License for more details.

       You should have received a copy of the GNU General Public License
       along with this program; if not, write to the Free Software
       Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

       Permission is also granted to link this program with the Qt
       library, treating Qt like a library that normally accompanies the
       operating system kernel, whether or not that is in fact the case.            
     */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <artsc.h>
#include <audiofile.h>

#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

#include "sound-arts.h"

#define FILE_NAME_MAX 512
#define MAX_BUF_SIZE (4 * 1024)

int arts_play_file(char *filename)
{
    /* input from libaudiofile... */
    AFfilehandle in_file;
    int in_format, in_width, in_channels, frame_count;
    double in_rate;
    int bytes_per_frame;
    arts_stream_t a_stream;
    char buf[ MAX_BUF_SIZE ];
    int buf_frames;
    int frames_read;

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
    
    /* connect to server and play stream */
    a_stream = arts_play_stream(in_rate,in_width,in_channels,g_path_get_basename(filename));

    while ( ( frames_read = afReadFrames( in_file, AF_DEFAULT_TRACK, buf, buf_frames ) ) )
    {
        if (arts_write(a_stream,buf,frames_read * bytes_per_frame) <= 0)
	    return 1;
    }
    
    arts_close_stream(a_stream);

    if ( afCloseFile ( in_file ) )
	return 0;

    return 1;
}
