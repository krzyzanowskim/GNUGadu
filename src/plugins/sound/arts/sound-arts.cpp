/* $Id: sound-arts.cpp,v 1.2 2004/01/17 00:45:03 shaster Exp $ */
    /*

       Copyright (C) 1999 Stefan Westerfeld
       stefan@space.twc.de

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

#include <arts/soundserver.h>
#include <arts/artsversion.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <iostream>

#include "sound-arts.h"

using namespace std;
using namespace Arts;

/* returns the absolute path to a filename */
static string absolutePath(const string & path)
{
    if (path[0] == '/')
	return path;

    char buffer[PATH_MAX];
    getcwd(buffer, PATH_MAX);

    if (buffer[strlen(buffer) - 1] == '/')
	return buffer + path;
    else
	return string(buffer) + '/' + path;
}

int arts_play_file(char *filename)
{
    Dispatcher dispatcher;
    SimpleSoundServer server;
    server = Arts::Reference("global:Arts_SimpleSoundServer");

    if (server.isNull())
    {
	cerr << "Can't connect to sound server" << endl;
	return 1;
    }
    return server.play(absolutePath(filename)) != 0;
}
