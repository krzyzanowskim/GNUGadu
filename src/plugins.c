/* $Id: plugins.c,v 1.2 2003/03/22 21:46:12 zapal Exp $ */
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <gmodule.h>
#include <dlfcn.h>

#include "support.h"
#include "gg-types.h"
#include "plugins.h"
#include "unified-types.h"

extern GGaduConfig *config;

/* 
    sprawdza czy plugin jest na liscie modulow do zaladowania 
    jesli nie ma pliku .gg2/modules.load laduje wszystkie dostepne pluginy
    UWAGA: jesli plik istnieje lecz jest pusty, zaden plugin sie nie zaladuje
*/
gboolean plugin_at_list(gchar *name)
{
    GIOChannel *ch = NULL;
    GString *buffer = g_string_new(NULL);
    
    ch = g_io_channel_new_file(g_build_filename(config->configdir,"modules.load",NULL),"r",NULL);
    
    if (!ch) return TRUE;
    
    while (g_io_channel_read_line_string(ch, buffer, NULL, NULL) != G_IO_STATUS_EOF)
    {
	if (!g_strncasecmp(buffer->str,name,buffer->len-1))
	    return TRUE;
    }
    
    g_io_channel_shutdown(ch,TRUE,NULL);
        
    return FALSE;
};

/*
 * Laduje modul o podanej sciezce (path) oraz nazwie (name)
 */
gboolean load_plugin (gchar *path)
{
  GGaduPlugin *plugin_handler = NULL;
  GGaduPlugin *(*initialize_plugin) (gpointer);
  void        (*start_plugin)      ();
  void        (*destroy_plugin)    ();
  gchar       *(*ggadu_plugin_name)();
  gint        (*ggadu_plugin_type)();
  void        *handler = NULL;
  gchar       *error = NULL;
  
  int i;
  const struct
  {
    char *name;
    void (**ptr)();
  } syms[] =
  {
    {"ggadu_plugin_name", (void *)&ggadu_plugin_name},
    {"ggadu_plugin_type", (void *)&ggadu_plugin_type},
    {"initialize_plugin", (void *)&initialize_plugin},
    {"start_plugin",      &start_plugin},
    {"destroy_plugin",    &destroy_plugin},
    {NULL, NULL}
  };

  print_debug ("core: loading plugin: %s\n", path);

  handler = dlopen (path, RTLD_NOW);
  if (!handler)
  {
    print_debug ("core: %s: %s\n", path, dlerror ());
    g_warning ("%s is not a valid plugin file, load failed!\n", path);
    return FALSE;
  }

  for (i = 0; syms[i].name; i++) {
    *syms[i].ptr = dlsym (handler, syms[i].name);
	
    if ((error = dlerror ()) != NULL) {
	print_debug ("core: %s nie posiada %s: %s\n", path, syms[i].name, error);
        dlclose (handler);
        return FALSE;
    }
  }

  if (g_slist_find (config->plugins, ggadu_plugin_name ()))
  {
    print_debug ("core: ekhm... plugin %s is already loaded\n", path);
    dlclose (handler);
    return FALSE;
  }
  
  if (plugin_at_list (ggadu_plugin_name ()) || config->all_plugins_loaded)
  {
    plugin_handler                    = initialize_plugin (config);
    plugin_handler->plugin_so_handler = handler;
    plugin_handler->start_plugin      = start_plugin;
    plugin_handler->destroy_plugin    = destroy_plugin;
    plugin_handler->name              = ggadu_plugin_name ();
    plugin_handler->type              = ggadu_plugin_type ();
  }

  if (config->all_plugins_loaded)
  {
    config->plugins = g_slist_append (config->plugins, plugin_handler);
    start_plugin ();
  } else
  {
    config->all_available_plugins = g_slist_append
      (config->all_available_plugins, ggadu_plugin_name ());
  }

  return TRUE;
}

void unload_plugin(gchar *name) 
{
    GGaduPlugin *plugin_handler = find_plugin_by_name(name);
    GSList 	*list_tmp = NULL;
    GGaduVar	*var_tmp  = NULL;
    GGaduSignalinfo 	 *sig_tmp = NULL;
    
    if (plugin_handler == NULL) 
    { 
	print_debug("core : trying to unload not loaded plugin %s\n",name); 
	return; 
    }

    print_debug ("core: unloading plugin %s\n", name);
    
    /* najpierw dajemy czas pluginowi na zrobienie porz±dku ze sob± */
    plugin_handler->destroy_plugin();
    dlclose (plugin_handler->plugin_so_handler); /* tego ju¿ nie potrzebujemy*/
    
    /* jako, ¿e ju¿ nam praktycznie wszystko jedno w jakiej kolejno¶ci,
     * to dla porz±dku czy¶cimy w takiej kolejno¶ci, jak w gg-types.h,
     * ¿eby o niczym nie zapomnieæ
     */
    
    /* wa¿ne - nie tykaæ listy dostêpnych modu³ów */
    /* wypierdzielamy plugin z listy w³±czonych pluginów */
    config->plugins = g_slist_remove (config->plugins, plugin_handler);
   
    /* nie tykaæ name */
//    g_free (plugin_handler->name);         /* na pierwszy ogieñ idzie name */
    g_free (plugin_handler->description);  /* description pod ¶cianê */
    
    /* plugin_so_handler za³atwiony wcze¶niej, wiêc skipujemy */
    g_free (plugin_handler->config_file);  /* po co komu plik konfiguracyjny?*/
        
    /* variables na celowniku */
    list_tmp = (GSList *)plugin_handler->variables;
    while (list_tmp) 
    {
	var_tmp = (GGaduVar *)list_tmp->data;
	g_free(var_tmp->name);
	g_free(var_tmp);
	list_tmp = list_tmp->next;
    }
    g_slist_free(plugin_handler->variables);

    /* bye bye signals */
    list_tmp = (GSList *)plugin_handler->signals;
    while (list_tmp) 
    {
	sig_tmp = (GGaduSignalinfo *) list_tmp->data;
	g_free (sig_tmp->name);
	g_free (sig_tmp);
	list_tmp = list_tmp->next;
    }
    g_slist_free(plugin_handler->signals);
    plugin_handler->signals = NULL;

    /* dobry protokó³, to zwolniony protokó³ ;> */
//    g_free (plugin_handler->protocol->display_name);
//    g_free (plugin_handler->protocol->img_filename);
    
    /* protocol->statuslist */
//    list_tmp = (GSList *)plugin_handler->protocol->statuslist;
//    while (list_tmp) 
//    {
//  	sta_tmp = (GGaduStatusPrototype *) list_tmp->data;
//	GGaduStatusPrototype_free (sta_tmp);
//	list_tmp = list_tmp->next;
//    }
//    g_slist_free(plugin_handler->protocol->statuslist);

    /* u¶mierciæ */
//    g_free (plugin_handler->protocol);
    
    g_free(plugin_handler);
}

/* caution : can return NULL and it not mean that there is no such variable */
gpointer config_var_get(GGaduPlugin *handler,gchar *name) 
{
    GGaduVar *var = NULL;
    GSList *tmp = handler->variables;
    
    if ((handler == NULL) || (name == NULL) || (handler->variables == NULL)) 
	return NULL;
    
    while (tmp) 
    {
	var = (GGaduVar *)tmp->data;
	
	if (!g_strcasecmp(var->name,name)) {
		return var->ptr;
	}
	
	tmp = tmp->next;
    }
    return NULL;
}

gint config_var_check(GGaduPlugin *handler,gchar *name) 
{
    GGaduVar *var = NULL;
    GSList *tmp = handler->variables;
    
    if ((handler == NULL) || (name == NULL) || (handler->variables == NULL)) 
	return 0;
    
    while (tmp) 
    {
	var = (GGaduVar *)tmp->data;
	
	if (!g_strcasecmp(var->name,name)) {
		if (var->ptr != NULL)
		    return 1;
		else
		    return 0;
	}
	
	tmp = tmp->next;
    }
    return -1;
}

gint config_var_get_type(GGaduPlugin *handler, char *name)
{
    GGaduVar *var = NULL;
    GSList *tmp = handler->variables;
    
    if ((handler == NULL) || (name == NULL) || (handler->variables == NULL)) 
	return -1;
    
    while (tmp) 
    {
	var = (GGaduVar *)tmp->data;
	
	if (!ggadu_strcasecmp(var->name,name)) 
		return(var->type);
	
	tmp = tmp->next;
    }
    return -1;
}



void config_var_add(GGaduPlugin *handler, gchar *name, gint type)
{
    GGaduVar *var = NULL;
    
    if ((name == NULL) || (handler == NULL)) return;
    
    var = g_new0(GGaduVar, 1);
    var->name 	  = g_strdup (name);
    var->type 	  = type;
    var->ptr 	  = NULL;
    
    handler->variables = g_slist_append(handler->variables, var);
}

void config_var_set(GGaduPlugin *handler, gchar *name, gpointer val)
{
    GSList *tmp = handler->variables;
    GGaduVar *var = NULL;
    
//    if ((name == NULL) || (val == NULL) || (handler == NULL)) return;
    if ((name == NULL) || (handler == NULL)) return;
    
    while (tmp) 
    {
	var = (GGaduVar *)tmp->data;

	if (!ggadu_strcasecmp(var->name, name)) 
	{
	
	    if (val == NULL) {
		var->ptr = NULL;
		break;
	    }
	    print_debug("VAR \"%s\"\n",var->name);
	    switch (var->type) 
	    {
		case VAR_STR:
		case VAR_IMG: // VAR_IMG is a path to image file
		    print_debug("VAR_STR %s %d\n",val,strlen(val));
		    if (*(gchar *)val == 1)
			var->ptr = (gpointer) base64_decode(g_strstrip((gchar *)val+1));
		        else 
			var->ptr = (gpointer) g_strdup(g_strstrip((gchar *)val));
		    break;
		    
		case VAR_INT:
		case VAR_BOOL:
		    print_debug("VAR_INT||BOOL %d\n",val);
		    var->ptr = (gpointer)val;
		    break;
	    }

	    break;
	    
	}
	tmp = tmp->next;
	
    }
}

/*
 * Uruchamia czytanie plikow konfiguracyjnych dla wszystkich zarejestrowanych protokolow
 * Czyta pliki konfiguracyjne i ustawia wartosci zmiennych
 * Jesli plik nie zostal zdefiniowany to proboje czytac plik o nazwie pluginu, jesli i takiego pliku nie ma to nie wczytuje nic
 */
GGaduVar *find_variable(GGaduPlugin *plugin_handler,gchar *name)
{
    GSList *tmp = plugin_handler->variables;
    
    while (tmp)
    {
	GGaduVar *v = (GGaduVar *)tmp->data;
	if (!g_strcasecmp(name,v->name))
	    return v;
	tmp = tmp->next;
    }
    return NULL;
}
 
gboolean config_read(GGaduPlugin *plugin_handler)
{
    FILE *f;
    gchar line[1024], *val, *path;
    GGaduVar *v = NULL;

    print_debug("core : config_init : plugin : %s\n",plugin_handler->name);
	
    /* plik trzeba ustalic */
    path = g_strdup(plugin_handler->config_file);

    print_debug("core : trying to read file %s\n",path);

    f = fopen(path, "r");

    
    if (!f) 
    { 
      print_debug("core : there is no such file\n");
      return FALSE;
    }
    
    while (fgets(line, 1023, f)) 
    {

	if (line[0] == '#' || line[0] == ';') continue;

	if ( ((val = strchr(line, ' ')) == NULL) &&
	   ((val = strchr(line, '=')) == NULL) ) continue;

	*val = 0;
	
	val++;
	
	if ((v = find_variable(plugin_handler,line)) != NULL) {
	    if (v->type == VAR_INT)
	        config_var_set(plugin_handler, line, (gpointer)atoi(val));

	    if (v->type == VAR_STR || v->type == VAR_IMG)
	        config_var_set(plugin_handler, line, val);

	    if (v->type == VAR_BOOL) {
		if (!g_str_has_prefix("on", val))
		    val = g_strdup("1");
		    
	        config_var_set(plugin_handler, line, (gpointer)atoi(val));
	    }
	}
	
    }
	
    fclose(f);
    g_free(path);
    
    return TRUE;
}

gboolean config_save(GGaduPlugin *plugin_handler)
{
    gchar *path = NULL, *path_dest = NULL, *line;
    gsize length, terminator_pos, bytes_written;
    GSList *tmp = plugin_handler->variables;
    GIOChannel *ch = NULL;
    GIOChannel *ch_dest = NULL;
    
    path = g_strdup(plugin_handler->config_file);
    path_dest = g_strconcat(plugin_handler->config_file,".tmp_",NULL);
    
    
    ch_dest = g_io_channel_new_file(path_dest,"w",NULL);
    
    if (!ch_dest) return FALSE;
    g_io_channel_set_encoding(ch_dest,NULL,NULL);
    
    while (tmp)
    {
	gchar *line1 = NULL;
	GGaduVar *var = (GGaduVar *)tmp->data;
	
	if (var->ptr != NULL)
	{
	    if (var->type == VAR_STR || var->type == VAR_IMG)
	    {
		if (!g_strcasecmp(var->name,"password"))
		    line1 = g_strdup_printf("%s \x001%s\n",var->name,base64_encode((gchar *)var->ptr));
		else 
		{
		    if (strlen((gchar *)var->ptr) > 0)
			line1 = g_strdup_printf("%s %s\n",var->name,(gchar *)var->ptr);
		}
	    }
	
	    if ((var->type == VAR_INT || var->type == VAR_BOOL) && (var->ptr != NULL))
		line1 = g_strdup_printf("%s %d\n",var->name,(int)var->ptr);


	    if (line1) {
		print_debug("%s %d\n",line1,var->type);
		g_io_channel_write_chars(ch_dest, line1, -1, &bytes_written, NULL);
	    }
	}
	g_free(line1);
	
	tmp = tmp->next;
    }
    
    g_io_channel_shutdown(ch_dest,TRUE,NULL);
    
    ch_dest = g_io_channel_new_file(path_dest,"a+",NULL);
    g_io_channel_set_encoding(ch_dest,NULL,NULL);
    
    ch = g_io_channel_new_file(path,"r",NULL);
    if (ch) {
    
     g_io_channel_set_encoding(ch,NULL,NULL);
     
     while (g_io_channel_read_line(ch, &line,  &length, &terminator_pos, NULL) != G_IO_STATUS_EOF)
     {
	if (!g_str_has_prefix(line,"#")) {
	    gchar **spl = g_strsplit(line," ",5);
	    
	    if (spl && (config_var_check(plugin_handler,spl[0]) == -1)) {
		g_io_channel_write_chars(ch_dest, line, -1, &bytes_written, NULL);
		print_debug("set new entry value in file :%s: %s",spl[0],line);
	    }
	    
	    if (spl) g_strfreev(spl);
	} else {
	g_io_channel_write_chars(ch_dest, line, -1, &bytes_written, NULL);
	}

     g_free(line);
     }
    g_io_channel_shutdown(ch,TRUE,NULL);
    }
    
    g_io_channel_shutdown(ch_dest,TRUE,NULL);
    
    if (rename(path_dest,path) == -1) 
    {
	print_debug("Failed to rename tmp to config_file");
	return FALSE;
    }
    
    g_free(path);
    g_free(path_dest);

    return TRUE;
}



void register_signal_receiver(GGaduPlugin *plugin_handler,void (*sgr)(gpointer,gpointer)) 
{
    if ((plugin_handler == NULL) || (sgr == NULL)) return;
    
    print_debug("core : register_signal_receiver for %s\n",plugin_handler->name);

    plugin_handler->signal_receive_func = sgr;
}

/*
 *    Rejestruje nowy protokol o nazwie (name) oraz o strukturze (struct_ptr) wrzucajac go do listy protokolow
 *    Zwraca : handler zarejestrowanego protokolu
 */

GGaduPlugin *register_plugin(gchar *name, gchar *desc) 
{
    GGaduPlugin *plugin_handler = NULL;

    if (name == NULL) return NULL;
    
    print_debug("core : register_plugin %s\n",name);

    plugin_handler = g_new0(GGaduPlugin,1); /* tu jest tworzony plugin tak naprawde */

    plugin_handler->name = g_strdup(name);
    plugin_handler->description = g_strdup(desc);

    config->plugins = g_slist_append(config->plugins, plugin_handler);

    return (GGaduPlugin *)plugin_handler;
}


/*
 *    Zwraca handler protokolu o podanej nazwie
 */
 
GGaduPlugin *find_plugin_by_name(gchar *name) 
{
    GSList 	*tmp		= config->plugins;
    GGaduPlugin *plugin_handler = NULL;

    if (name == NULL) return NULL;
    
    while (tmp) 
    {
	plugin_handler = (GGaduPlugin *)tmp->data;
	
	if (plugin_handler)
	if (!ggadu_strcasecmp(plugin_handler->name, name)) 
	    return plugin_handler;
	    
	tmp = tmp->next;
    }
    return NULL;
}

/* 
 *  zwraca liste handlerow pasujacych do wzorca 
 */
GSList *find_plugin_by_pattern(gchar *pattern) 
{
    GSList 	*tmp		= config->plugins;
    GGaduPlugin *plugin_handler = NULL;
    GSList	*found_list	= NULL;

    if (pattern == NULL) return NULL;
    
    while (tmp) 
    {
	plugin_handler = (GGaduPlugin *)tmp->data;
	
	/* print_debug("pattern %s, name %s\n",pattern, plugin_handler->name); */
	
	if (g_pattern_match_simple(pattern,plugin_handler->name)) 
	    found_list = g_slist_append(found_list,plugin_handler);
	    
	tmp = tmp->next;
    }
    
    return found_list;
}


/* 
 *    Inicjalizuje plik do zczytania z pliku specyficzne dla tego protokolu
 *    config_file 	- nazwa pliku konfiguracyjnego
 *    encoding 		- kodowanie w jakim jest plik
 */

void set_config_file_name(GGaduPlugin *plugin_handler, gchar *config_file)
{
    if (plugin_handler == NULL) return;
    
    if (config_file == NULL) 
	config_file = plugin_handler->name;
    
    print_debug("core : config_init_register for %s\n", plugin_handler->name);
    
    plugin_handler->config_file = config_file;
}
