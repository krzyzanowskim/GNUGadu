/* $Id: main.c,v 1.1 2003/03/20 10:37:05 krzyzak Exp $ */

/*
 *  (C) Copyright 2001-2002 Igor Popik <thrull@slackware.pl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/wait.h>

#include "support.h"
#include "gg-types.h"
#include "plugins.h"
#include "signals.h"

GGaduConfig *config;


void load_available_modules();

gboolean gnu_gadu_init(gpointer data);

/**
    Laduje dostepne pluginy z configpath/modules. Tylko pliki o rozszerzeniu ".so"
**/
void load_available_modules() {
    gchar *dirpath	= PACKAGE_LIB_DIR; /* "Makefile.am" */
    GDir  *directory	= g_dir_open(dirpath,0,NULL);
    gchar *namepl		= NULL;

//#ifdef DEBUG
    /* just for easy testing purpose part code */
    if (directory == NULL)
    {
	dirpath = g_build_filename(config->configdir,"modules",NULL);
    
	if ((directory = g_dir_open(dirpath,0,NULL)) == NULL)
	{
	show_error(_("Cannot open any modules directory\n"));
	exit(1);
	}
    }
    /* */
//#endif
    
    config->all_plugins_loaded = FALSE;
    
    print_debug("core : searching modules in directory : %s\n",dirpath);
    
    if (directory == NULL) {
	g_error(_("Unable to open directory with plugins (%s)\n"),dirpath);
	return;
    }
    
    while ((namepl = (gchar *)g_dir_read_name(directory)) != NULL) 
    {
	gchar *fullfilename = g_build_filename(dirpath, namepl, NULL);

	if ( (!g_file_test(fullfilename, G_FILE_TEST_IS_DIR)) && (str_has_suffix(namepl, ".so")) ) 
	{
		print_debug("core : load_avilable_modules : %s\n",namepl);

		if (!load_plugin(fullfilename))  /* zaladowanie plugina */
		{
		    g_warning(_("Cannot load plugin %s\n"),fullfilename);
		}

	}
	
	g_free(fullfilename);
    }
    
    g_dir_close(directory);
};



void start_plugins()
{
    GSList 	*tmp	= NULL;
    GGaduPlugin *plugin = NULL;
    void (*start_plugin)();
    
    tmp = config->plugins;
    
    while (tmp) 
    {
	plugin = (GGaduPlugin *) tmp->data;
	
	start_plugin = plugin->start_plugin;
	
	if (start_plugin != NULL)
	    start_plugin();
	
	tmp = tmp->next;
    }
}

gboolean gnu_gadu_init(gpointer data)
{
    config = g_new0(GGaduConfig,1);

    /* katalog konfiguracyjny */    
    if (g_getenv("CONFIG_DIR")) 
        config->configdir = g_build_filename(g_getenv("HOME"), g_getenv("CONFIG_DIR"), "gg2", NULL);
     else 
        config->configdir = g_build_filename(g_getenv("HOME"), ".gg2", NULL);
    
    mkdir(config->configdir, 0700);

    load_available_modules();

    start_plugins();
    
    flush_queued_signals();

    return TRUE;
}

void sigchld(int sig)
{
    gint pid, status;
    
    while ((pid = wait(&status)) != -1) {
	waitpid(pid, NULL, 0);
    }
}

int main(int argc, char **argv)
{
#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
    textdomain(PACKAGE);
    bind_textdomain_codeset (PACKAGE, "UTF-8");
#endif
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, sigchld);

//    if (!g_thread_supported()) 
	g_thread_init(NULL);	// a nuz widelec ktos bedzie potrzebowal watkow ?

    gnu_gadu_init(NULL);

    config->main_loop = g_main_loop_new(NULL,FALSE);

    g_main_loop_run( config->main_loop );

    return 0;
}
