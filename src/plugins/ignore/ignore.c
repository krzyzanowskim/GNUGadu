/* $Id: ignore.c,v 1.2 2004/12/22 16:24:18 krzyzak Exp $ */

/* 
 * Ignore: plugin code for GNU Gadu 2 
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gg2_core.h"
#include "ignore.h"

static GGaduPlugin *ignore_handler;

GGadu_PLUGIN_INIT("ignore-main", GGADU_PLUGIN_TYPE_MISC);

/* SIGNALS HANDLER */
static void my_signal_receive(gpointer name, gpointer signal_ptr)
{
	GGaduSignal *signal = (GGaduSignal *) signal_ptr;

	if (signal->name == IGNORE_ADD_CONTACT_SIG)
	{
	}

	if (signal->name == IGNORE_REMOVE_CONTACT_SIG)
	{
	}

	if (signal->name == IGNORE_CHECK_CONTACT_SIG)
	{
	    GGaduContact *k = (GGaduContact *)signal->data;
	    
	    signal->data_return = FALSE;
	    if (g_strrstr(ggadu_config_var_get(ignore_handler, "list"), k->id))
	    {
	        signal->data_return = (gpointer)TRUE;
	    }
	    
	}
}

static gchar *ggadu_add_ignored_contact(gchar *ignored_list, GGaduContact *k)
{
	gchar *ret = NULL;

	if(!k || !k->id) return NULL;
	if (!ignored_list) ignored_list = "";

	if(!g_strrstr(ignored_list, k->id))
		ret = g_strconcat(ignored_list, k->id, ":", NULL);
	
	return ret;
}

static gchar *ggadu_remove_ignored_contact(gchar *ignored_list, GGaduContact *k)
{
	gchar *ret = NULL;
	gchar *searchk = g_strconcat(k->id,":",NULL);

	if(!k || !ignored_list || !k->id) return NULL;
	
	if(g_strrstr(ignored_list, searchk))
	{
	        gchar **tab;

	        tab = g_strsplit(ignored_list, searchk, 2);
		ret = g_strconcat(tab[0], tab[1], NULL);
		g_strfreev(tab);
	}
	
	g_free(searchk);
        return ret;
}


static gpointer ignore_ignore_contact(gpointer selected_contacts)
{
    GSList *users = (GSList *) selected_contacts;
    GGaduContact *k = (users) ? (GGaduContact *) users->data : NULL;
    gchar *ignored_list_after = NULL;
    gchar *ignored_list_prev = ggadu_config_var_get(ignore_handler, "list");
    
    print_debug("ignore action %s",k->id);
    ignored_list_after = ggadu_add_ignored_contact(ignored_list_prev,k);
    
    if (ignored_list_after)
    {
	ggadu_config_var_set(ignore_handler, "list", ignored_list_after);
	ggadu_config_save(ignore_handler);
        g_free(ignored_list_prev);
    }
    return NULL;
}

static gpointer ignore_unignore_contact(gpointer selected_contacts)
{
    GSList *users = (GSList *) selected_contacts;
    GGaduContact *k = (users) ? (GGaduContact *) users->data : NULL;
    gchar *ignored_list_after = NULL;
    gchar *ignored_list_prev = ggadu_config_var_get(ignore_handler, "list");

    print_debug("unignore action %s",k->id);
    ignored_list_after = ggadu_remove_ignored_contact(ignored_list_prev,k);
    
    if (ignored_list_after)
    {
	ggadu_config_var_set(ignore_handler, "list", ignored_list_after);
	ggadu_config_save(ignore_handler);
        g_free(ignored_list_prev);
    }
    
    
    return NULL;
}


/* MAIN */
void start_plugin()
{
    GGaduPluginExtension *ext_in, *ext_un;
    ext_in = g_new0(GGaduPluginExtension, 1);
    ext_in->type = GGADU_PLUGIN_EXTENSION_USER_MENU_TYPE;
    ext_in->callback = ignore_ignore_contact;
    ext_in->txt = _("_Ignore");
    register_extension_for_plugin(ext_in,GGADU_PLUGIN_TYPE_PROTOCOL);

    ext_un = g_new0(GGaduPluginExtension, 1);
    ext_un->type = GGADU_PLUGIN_EXTENSION_USER_MENU_TYPE;
    ext_un->callback = ignore_unignore_contact;
    ext_un->txt = _("_UnIgnore");
    register_extension_for_plugin(ext_un,GGADU_PLUGIN_TYPE_PROTOCOL);

}

/* PLUGIN INITIALISATION */
GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
	gchar *this_configdir = NULL;

	GGadu_PLUGIN_ACTIVATE(conf_ptr);
	ignore_handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("Ignore option support"));

	IGNORE_ADD_CONTACT_SIG = register_signal(ignore_handler, "ignore add contact");
	IGNORE_REMOVE_CONTACT_SIG = register_signal(ignore_handler, "ignore remove contact");
	IGNORE_CHECK_CONTACT_SIG = register_signal(ignore_handler, "ignore check contact");

	if (g_getenv("HOME_ETC"))
		this_configdir = g_build_filename(g_getenv("HOME_ETC"), "gg2", NULL);
	else
		this_configdir = g_build_filename(g_get_home_dir(), ".gg2", NULL);

	ggadu_config_set_filename(ignore_handler, g_build_filename(this_configdir, "ignore-main", NULL));
	ggadu_config_var_add(ignore_handler, "list", VAR_STR);

	if (!ggadu_config_read(ignore_handler))
		g_warning(_("Unable to read configuration file for plugin %s"), "");

	register_signal_receiver(ignore_handler, (signal_func_ptr) my_signal_receive);

	g_free(this_configdir);
	return ignore_handler;
}

void destroy_plugin()
{
	print_debug("destroy_plugin %s", GGadu_PLUGIN_NAME);
}
