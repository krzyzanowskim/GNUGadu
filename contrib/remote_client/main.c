/* $Id: main.c,v 1.3 2004/01/28 23:39:12 shaster Exp $ */

/* 
 * Example: client for GNU Gadu 2 remote plugin 
 * 
 * Copyright (C) 2001-2004 GNU Gadu Team 
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

#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "remote_client.h"

int main(int argc, char *argv[])
{
    char *s;

    if (remote_init() == -1)
	return -1;

    if (argc > 1)
    {
	remote_send(argv[1]);
	return 0;
    }

    remote_sig_remote_get_icon(&s);
    printf("get_icon -> %s\n", s);
    free(s);
    remote_sig_remote_get_icon_path(&s);
    printf("get_icon_path -> %s\n", s);
    free(s);

    remote_close();

    return 0;
}
