/* $Id: plugins.c,v 1.13 2004/01/28 23:39:26 shaster Exp $ */

/* 
 * GNU Gadu 2 
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

#include <stdio.h>
#include <glib.h>
#include <gmodule.h>
#include <dlfcn.h>

#include "plugins.h"
#include "support.h"
#include "gg-types.h"
#include "unified-types.h"

/* 
    sprawdza czy plugin jest na liscie modulow do zaladowania 
    jesli nie ma pliku .gg2/modules.load laduje wszystkie dostepne pluginy
    UWAGA: jesli plik istnieje lecz jest pusty, zaden plugin sie nie zaladuje
*/
gboolean plugin_at_list(gchar * name)
{
	GIOChannel *ch = NULL;
	GString *buffer = g_string_new(NULL);
	gchar *filename;
	gint lines = 0;

	filename = g_build_filename(config->configdir, "modules.load", NULL);

	ch = g_io_channel_new_file(filename, "r", NULL);

	g_free(filename);

	if (!ch)
	{
		g_string_free(buffer, TRUE);
		return TRUE;
	}
	while (g_io_channel_read_line_string(ch, buffer, NULL, NULL) != G_IO_STATUS_EOF)
	{
		if (buffer->str && *buffer->str == '\n')
		{
			continue;
		}
		if (!g_strncasecmp(buffer->str, name, buffer->len - 1))
		{
			g_string_free(buffer, TRUE);
			return TRUE;
		}
		lines++;
	}
	g_io_channel_shutdown(ch, TRUE, NULL);
	g_string_free(buffer, TRUE);

	if (!lines)
		return TRUE;

	return FALSE;
}

/*
 * Laduje modul o podanej sciezce (path) oraz nazwie (name)
 */
gboolean load_plugin(gchar * path)
{
	GGaduPlugin *plugin_handler = NULL;
	GGaduPlugin *(*initialize_plugin) (gpointer);
	void (*start_plugin) ();
	void (*destroy_plugin) ();
	gchar *(*ggadu_plugin_name) ();
	gint(*ggadu_plugin_type) ();
	void *handler = NULL;
	gchar *error = NULL;

	int i;
	const struct
	{
		char *name;
		void (**ptr) ();
	} syms[] =
	{
		{
		"ggadu_plugin_name", (void *) &ggadu_plugin_name},
		{
		"ggadu_plugin_type", (void *) &ggadu_plugin_type},
		{
		"initialize_plugin", (void *) &initialize_plugin},
		{
		"start_plugin", &start_plugin},
		{
		"destroy_plugin", &destroy_plugin},
		{
		NULL, NULL}
	};

	print_debug("core: loading plugin: %s\n", path);

	handler = dlopen(path, RTLD_NOW);
	if (!handler)
	{
		g_warning("%s is not a valid plugin file, load failed! %s\n", path, dlerror());
		return FALSE;
	}

	for (i = 0; syms[i].name; i++)
	{
		*syms[i].ptr = dlsym(handler, syms[i].name);

		if ((error = dlerror()) != NULL)
		{
			g_warning(_("core: %s have no %s: %s\n"), path, syms[i].name, error);
			dlclose(handler);
			return FALSE;
		}
	}

	if (g_slist_find(config->plugins, ggadu_plugin_name()))
	{
		print_debug("core: ekhm... plugin %s is already loaded\n", path);
		dlclose(handler);
		return FALSE;
	}

	if (plugin_at_list(ggadu_plugin_name()) || config->all_plugins_loaded)
	{
		plugin_handler = initialize_plugin(config);
		plugin_handler->plugin_so_handler = handler;
		plugin_handler->start_plugin = start_plugin;
		plugin_handler->destroy_plugin = destroy_plugin;
		plugin_handler->name = ggadu_plugin_name();
		plugin_handler->type = ggadu_plugin_type();
	}

	if (config->all_plugins_loaded)
	{
		GSList *tmp = NULL;
		GGaduPluginFile *pf = NULL;
		char siedzi = 0;

		tmp = config->all_available_plugins;
		while (tmp)
		{
			pf = (GGaduPluginFile *) tmp->data;
			if (!ggadu_strcasecmp(pf->name, ggadu_plugin_name()))
			{
				siedzi = 1;
				break;
			}
			tmp = tmp->next;
		}
		if (!siedzi)
		{
			pf = g_new0(GGaduPluginFile, 1);

			pf->name = g_strdup(ggadu_plugin_name());
			pf->path = g_strdup(path);

			config->all_available_plugins = g_slist_append(config->all_available_plugins, pf);
		}
		start_plugin();
	}
	else
	{
		GGaduPluginFile *pf = g_new0(GGaduPluginFile, 1);

		pf->name = g_strdup(ggadu_plugin_name());
		pf->path = g_strdup(path);

		config->all_available_plugins = g_slist_append(config->all_available_plugins, pf);
	}

	return TRUE;
}

void unload_plugin(gchar * name)
{
	GGaduPlugin *plugin_handler = find_plugin_by_name(name);
	GSList *list_tmp = NULL;
	GGaduVar *var_tmp = NULL;
	GGaduSignalinfo *sig_tmp = NULL;

	if (plugin_handler == NULL)
	{
		g_warning(_("core : trying to unload not loaded plugin %s\n"), name);
		return;
	}

	print_debug("core: unloading plugin %s\n", name);

	/* najpierw dajemy czas pluginowi na zrobienie porz±dku ze sob± */
	plugin_handler->destroy_plugin();
	dlclose(plugin_handler->plugin_so_handler);	/* tego ju¿ nie potrzebujemy */

	/* jako, ¿e ju¿ nam praktycznie wszystko jedno w jakiej kolejno¶ci,
	 * to dla porz±dku czy¶cimy w takiej kolejno¶ci, jak w gg-types.h,
	 * ¿eby o niczym nie zapomnieæ
	 */

	/* waÅ¼ne - nie tykaæ listy dostêpnych modu³ów */
	/* wypierdzielamy plugin z listy w³±czonych pluginów */
	config->plugins = g_slist_remove(config->plugins, plugin_handler);

	/* nie tykaæ name */
/*
    g_free (plugin_handler->name);
*/
	g_free(plugin_handler->description);	/* description pod ¶cianê */

	/* plugin_so_handler za³atwiony wcze¶niej, wiêc skipujemy */
	g_free(plugin_handler->config_file);	/* po co komu plik konfiguracyjny? */

	/* variables na celowniku */
	list_tmp = (GSList *) plugin_handler->variables;
	while (list_tmp)
	{
		var_tmp = (GGaduVar *) list_tmp->data;
		g_free(var_tmp->name);
		g_free(var_tmp);
		list_tmp = list_tmp->next;
	}

	g_slist_free(plugin_handler->variables);

	/* bye bye signals */
	list_tmp = (GSList *) plugin_handler->signals;

	while (list_tmp)
	{
		sig_tmp = (GGaduSignalinfo *) list_tmp->data;
/*
	g_free (sig_tmp->name);
*/
		g_free(sig_tmp);
		list_tmp = list_tmp->next;
	}
	g_slist_free(plugin_handler->signals);
	plugin_handler->signals = NULL;

	/* dobry protokó³ to zwolniony protokó³ ;> */
/*
    g_free (plugin_handler->protocol->display_name);
    g_free (plugin_handler->protocol->img_filename);
*/

	/* protocol->statuslist */
/*
    list_tmp = (GSList *)plugin_handler->protocol->statuslist;
    while (list_tmp) 
    {
	sta_tmp = (GGaduStatusPrototype *) list_tmp->data;
	GGaduStatusPrototype_free (sta_tmp);
	list_tmp = list_tmp->next;
    }
    g_slist_free(plugin_handler->protocol->statuslist);
*/

	/* u¶mierciæ */
/*
    g_free (plugin_handler->protocol);
*/
	g_free(plugin_handler);
}

/* caution : can return NULL and it not mean that there is no such variable */
gpointer config_var_get(GGaduPlugin * handler, gchar * name)
{
	GGaduVar *var = NULL;
	GSList *tmp = NULL;

	if ((handler == NULL) || (name == NULL) || (handler->variables == NULL))
		return NULL;

	tmp = handler->variables;

	while (tmp)
	{
		var = (GGaduVar *) tmp->data;

		if ((var != NULL) && (!g_strcasecmp(var->name, name)))
		{
			return var->ptr;
		}

		tmp = tmp->next;
	}
	return NULL;
}


void register_signal_receiver(GGaduPlugin * plugin_handler, void (*sgr) (gpointer, gpointer))
{
	if ((plugin_handler == NULL) || (sgr == NULL))
		return;

	print_debug("core : register_signal_receiver for %s\n", plugin_handler->name);

	plugin_handler->signal_receive_func = sgr;
}

/*
 *    Rejestruje nowy protokol o nazwie (name) oraz o strukturze (struct_ptr) wrzucajac go do listy protokolow
 *    Zwraca : handler zarejestrowanego protokolu
 */

GGaduPlugin *register_plugin(gchar * name, gchar * desc)
{
	GGaduPlugin *plugin_handler = NULL;

	if (name == NULL)
		return NULL;

	print_debug("core : register_plugin %s\n", name);

	plugin_handler = g_new0(GGaduPlugin, 1);	/* tu jest tworzony plugin tak naprawde */

	plugin_handler->name = g_strdup(name);
	plugin_handler->description = g_strdup(desc);

	config->plugins = g_slist_append(config->plugins, plugin_handler);

	return (GGaduPlugin *) plugin_handler;
}


/*
 *    Zwraca handler protokolu o podanej nazwie, z zaladowanych pluginow
 */

GGaduPlugin *find_plugin_by_name(gchar * name)
{
	GSList *tmp = (config) ? config->plugins : NULL;
	GGaduPlugin *plugin_handler = NULL;

	if (name == NULL)
		return NULL;

	while (tmp)
	{
		plugin_handler = (GGaduPlugin *) tmp->data;

		if ((plugin_handler != NULL) && (plugin_handler->name != NULL) &&
		    (!ggadu_strcasecmp(plugin_handler->name, name)))
			return plugin_handler;

		tmp = tmp->next;
	}

	return NULL;
}

/* 
 *  zwraca liste handlerow pasujacych do wzorca 
 */
GSList *find_plugin_by_pattern(gchar * pattern)
{
	GSList *tmp = config->plugins;
	GGaduPlugin *plugin_handler = NULL;
	GSList *found_list = NULL;

	if (pattern == NULL)
		return NULL;

	while (tmp)
	{
		plugin_handler = (GGaduPlugin *) tmp->data;

		/* print_debug("pattern %s, name %s\n",pattern, plugin_handler->name); */

		if (g_pattern_match_simple(pattern, plugin_handler->name))
			found_list = g_slist_append(found_list, plugin_handler);

		tmp = tmp->next;
	}

	return found_list;
}


void register_extension_for_plugins(GGaduPluginExtension * ext)
{
	GSList *plugins;

	if ((!ext) || (!config))
		return;

	plugins = config->plugins;

	while (plugins)
	{
		GGaduPlugin *handler = (GGaduPlugin *) plugins->data;

		if (handler)
			handler->extensions = g_slist_append(handler->extensions, ext);

		plugins = plugins->next;
	}

}

void unregister_extension_for_plugins(GGaduPluginExtension * ext)
{
	GSList *plugins;
	/* GSList *extensions; */

	if ((!ext) || (!config))
		return;

	plugins = config->plugins;

	while (plugins)
	{
		GGaduPlugin *handler = (GGaduPlugin *) plugins->data;

		g_slist_remove((GSList *) handler->extensions, ext);

/*		extensions = handler->extensions;
	
		while (extensions) {
			GGaduPluginExtension *ext_search = extensions->data;
		
			if (ext_search == ext)
				return ext;
		
			extensions = extensions->next;
		}
		
*/
		plugins = plugins->next;
	}

}


GSList *get_list_modules_load()
{
	GIOChannel *ch = NULL;
	GString *buffer = g_string_new(NULL);
	GSList *tmp = NULL, *ret = NULL;
	GGaduPlugin *plugin = NULL;

	ch = g_io_channel_new_file(g_build_filename(config->configdir, "modules.load", NULL), "r", NULL);

	if (ch)
	{
		while (g_io_channel_read_line_string(ch, buffer, NULL, NULL) != G_IO_STATUS_EOF)
		{
			tmp = config->plugins;
			while (tmp)
			{
				plugin = (GGaduPlugin *) tmp->data;
				if (!g_strncasecmp(buffer->str, plugin->name, buffer->len - 1))
					ret = g_slist_append(ret, plugin);
				tmp = tmp->next;
			}
		}
		g_io_channel_shutdown(ch, TRUE, NULL);
	}

	/* ugly hack: no modules.load file, load all plugins */
	if (ret == NULL)
	{
		tmp = config->plugins;
		while (tmp)
		{
			plugin = (GGaduPlugin *) tmp->data;
			ret = g_slist_append(ret, plugin);
			tmp = tmp->next;
		}
	}
	return ret;
}
