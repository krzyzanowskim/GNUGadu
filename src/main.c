/* $Id: main.c,v 1.15 2004/01/17 00:44:59 shaster Exp $ */

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
#include "repo.h"
#include "perl_embed.h"

GGaduConfig *config;

gboolean gnu_gadu_init(gpointer data);

/* Load availabe plugin from configpath/modules. Only files with extension ".so" */
void load_available_modules()
{
    gchar *dirpath = PACKAGE_LIB_DIR;	/* "Makefile.am" */
    GDir *directory = g_dir_open(dirpath, 0, NULL);
    gchar *namepl = NULL;

    /* just for easy testing purpose part code */
    if (directory == NULL)
    {
	dirpath = g_build_filename(config->configdir, "modules", NULL);

	if ((directory = g_dir_open(dirpath, 0, NULL)) == NULL)
	{
	    show_error(_("Cannot open any modules directory\n"));
	    exit(1);
	}
    }

    config->all_plugins_loaded = FALSE;

    print_debug("core : searching modules in directory : %s\n", dirpath);

    if (directory == NULL)
    {
	g_error(_("Unable to open directory with plugins (%s)\n"), dirpath);
	return;
    }

    while ((namepl = (gchar *) g_dir_read_name(directory)) != NULL)
    {
	gchar *fullfilename = g_build_filename(dirpath, namepl, NULL);

	if ((!g_file_test(fullfilename, G_FILE_TEST_IS_DIR)) && (str_has_suffix(namepl, ".so")))
	{
	    print_debug("core : load_avilable_modules : %s\n", namepl);

	    if (!load_plugin(fullfilename))	/* zaladowanie plugina */
		g_warning(_("Cannot load plugin %s\n"), fullfilename);
	}

	g_free(fullfilename);
    }

    g_dir_close(directory);
};

void start_plugins()
{
    GSList *tmp = NULL;
    GGaduPlugin *plugin = NULL;
    void (*start_plugin) ();

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

void start_plugins_ordered()
{
    GSList *tmp = NULL;
    GGaduPlugin *plugin = NULL;
    void (*start_plugin) ();
    gboolean main_gui_loaded = FALSE;

    tmp = get_list_modules_load();

    while (tmp)
    {
	plugin = (GGaduPlugin *) tmp->data;
	start_plugin = plugin->start_plugin;
	if (start_plugin)
	    start_plugin();

	if (!g_strcasecmp(plugin->name, "main-gui"))
	    main_gui_loaded = TRUE;

	tmp = tmp->next;
    }

    if (!main_gui_loaded)
    {
	g_print(_("\n\t***************************\n \
	GNU Gadu kindly inform you that because you have no GUI plugin installed/compiled/loaded.\n  \
	you shouldn't expect common interaction with application.\n \
	Check README for more information about plugins\n \
	***************************\n"));
    }

}

gboolean gnu_gadu_init(gpointer data)
{
    config = g_new0(GGaduConfig, 1);

    /* configure directory */
    if (g_getenv("CONFIG_DIR"))
	config->configdir = g_build_filename(g_getenv("HOME"), g_getenv("CONFIG_DIR"), "gg2", NULL);
    else
	config->configdir = g_build_filename(g_getenv("HOME"), ".gg2", NULL);

    mkdir(config->configdir, 0700);
    config->repos = g_new0(GGaduRepo, 1);
    ggadu_repo_add("_protocols_");
#ifdef PERL_EMBED
    perl_load_scripts();
#endif
    signal_from_thread_enabled();
    load_available_modules();
    start_plugins_ordered();
    flush_queued_signals();

    return TRUE;
}

void sigchld(int sig)
{
    gint pid, status;

    while ((pid = wait(&status)) != -1)
    {
	waitpid(pid, NULL, 0);
    }
}

int main(int argc, char **argv)
{
#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
    textdomain(PACKAGE);
    bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, sigchld);

    g_thread_init(NULL);
    gnu_gadu_init(NULL);
    config->main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(config->main_loop);

    return 0;
}
