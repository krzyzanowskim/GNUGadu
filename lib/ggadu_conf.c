/* $Id: ggadu_conf.c,v 1.7 2004/01/17 00:44:58 shaster Exp $ */

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "support.h"
#include "ggadu_conf.h"
#include "unified-types.h"
#include "gg-types.h"

static GGaduVar *ggadu_find_variable(GGaduPlugin * plugin_handler, gchar * name)
{
    GSList *tmp = plugin_handler->variables;

    while (tmp)
    {
	GGaduVar *v = (GGaduVar *) tmp->data;
	if (!g_strcasecmp(name, v->name))
	    return v;
	tmp = tmp->next;
    }
    return NULL;
}

gint ggadu_config_var_check(GGaduPlugin * handler, gchar * name)
{
    GGaduVar *var = NULL;
    GSList *tmp = handler->variables;

    if (!handler || !name || !handler->variables)
	return 0;

    while (tmp)
    {
	var = (GGaduVar *) tmp->data;

	if (!g_strcasecmp(var->name, name))
	{
	    if (var->ptr)
		return 1;
	    else
		return 0;
	}

	tmp = tmp->next;
    }
    return -1;
}

/* caution : can return NULL and it not mean that there is no such variable */
gpointer ggadu_config_var_get(GGaduPlugin * handler, gchar * name)
{
    GGaduVar *var = NULL;
    GSList *tmp = NULL;

    if (!handler || !name || !handler->variables)
	return NULL;

    tmp = handler->variables;

    while (tmp)
    {
	var = (GGaduVar *) tmp->data;

	if (var && (!g_strcasecmp(var->name, name)))
	{
	    return var->ptr ? var->ptr : var->def;
	}

	tmp = tmp->next;
    }
    return NULL;
}


gint ggadu_config_var_get_type(GGaduPlugin * handler, char *name)
{
    GGaduVar *var = NULL;
    GSList *tmp = handler->variables;

    if ((handler == NULL) || (name == NULL) || (handler->variables == NULL))
	return -1;

    while (tmp)
    {
	var = (GGaduVar *) tmp->data;

	if (!ggadu_strcasecmp(var->name, name))
	    return (var->type);

	tmp = tmp->next;
    }
    return -1;
}

void ggadu_config_var_set(GGaduPlugin * handler, gchar * name, gpointer val)
{
    GSList *tmp = NULL;

    if ((!name) || (!handler) || (!handler->variables))
	return;

    tmp = handler->variables;

    while (tmp)
    {
	GGaduVar *var = (GGaduVar *) tmp->data;

	if ((var) && (var->name) && (!ggadu_strcasecmp(var->name, name)))
	{

	    if (!val)
	    {
		var->ptr = (var->def) ? var->def : NULL;
		break;
	    }

	    print_debug("VAR \"%s\" ", var->name);
	    switch (var->type)
	    {
	    case VAR_STR:
	    case VAR_IMG:	/* VAR_IMG is a path to image file */
	    case VAR_LIST:

		print_debug("VAR_STR %s\n", (gchar *) val);

		if (val && (strlen((gchar *) val) == 0))
		{
		    var->ptr = var->def;
		    print_debug(" SET DEFAULT : %s\n", (gchar *) val);
		    break;
		}

		if (*(gchar *) val == 1)
		    var->ptr = (gpointer) base64_decode(g_strstrip((gchar *) val + 1));
		else
		    var->ptr = (gpointer) g_strdup(g_strstrip((gchar *) val));

		break;
	    case VAR_INT:
	    case VAR_BOOL:
		if (val)
		    print_debug("VAR_INT||BOOL %d", val);
		var->ptr = (gpointer) val;
		break;
	    }

	    break;

	}
	tmp = tmp->next;
    }
}

void ggadu_config_var_add(GGaduPlugin * handler, gchar * name, gint type)
{
    GGaduVar *var = NULL;

    if (!name || !handler)
	return;

    var = g_new0(GGaduVar, 1);
    var->name = g_strdup(name);
    var->type = type;
    var->ptr = NULL;
    var->def = NULL;

    handler->variables = g_slist_append(handler->variables, var);
}

void ggadu_config_var_add_with_default(GGaduPlugin * handler, gchar * name, gint type, gpointer default_value)
{
    GGaduVar *var = NULL;

    if ((name == NULL) || (handler == NULL))
	return;

    var = g_new0(GGaduVar, 1);
    var->name = g_strdup(name);
    var->type = type;
    var->ptr = NULL;
    var->def = default_value;

    handler->variables = g_slist_append(handler->variables, var);
}

/*
 * Uruchamia czytanie plikow konfiguracyjnych dla wszystkich zarejestrowanych protokolow
 * Czyta pliki konfiguracyjne i ustawia wartosci zmiennych
 * Jesli plik nie zostal zdefiniowany to proboje czytac plik o nazwie pluginu, jesli i takiego pliku nie ma to nie wczytuje nic
 */
gboolean ggadu_config_read(GGaduPlugin * plugin_handler)
{
    FILE *f;
    gchar line[1024], *val, *path;
    GGaduVar *v = NULL;

    print_debug("Reading configuration file %s\n", plugin_handler->name);

    /* plik trzeba ustalic */
    path = g_strdup(plugin_handler->config_file);

    print_debug("core : trying to read file %s\n", path);

    f = fopen(path, "r");

    g_free(path);

    if (!f)
    {
	print_debug("core : there is no such file\n");
	return FALSE;
    }

    while (fgets(line, 1023, f))
    {

	if (line[0] == '#' || line[0] == ';')
	    continue;

	if (((val = strchr(line, ' ')) == NULL) && ((val = strchr(line, '=')) == NULL))
	    continue;

	*val = 0;

	val++;

	if ((v = ggadu_find_variable(plugin_handler, line)) != NULL)
	{
	    if (v->type == VAR_INT)
		ggadu_config_var_set(plugin_handler, line, (gpointer) atoi(val));

	    if (v->type == VAR_STR || v->type == VAR_IMG)
		ggadu_config_var_set(plugin_handler, line, val);

	    if (v->type == VAR_BOOL)
	    {
		if (!g_str_has_prefix("on", val))
		    val = g_strdup("1");

		ggadu_config_var_set(plugin_handler, line, (gpointer) atoi(val));
	    }
	}

    }

    fclose(f);

    return TRUE;
}

gboolean ggadu_config_save(GGaduPlugin * plugin_handler)
{
    gchar *path = NULL, *path_dest = NULL, *line;
    gsize length, terminator_pos, bytes_written;
    GSList *tmp = plugin_handler->variables;
    GIOChannel *ch = NULL;
    GIOChannel *ch_dest = NULL;

    path = g_strdup(plugin_handler->config_file);
    path_dest = g_strconcat(plugin_handler->config_file, ".tmp_", NULL);

    if (!g_file_test(g_path_get_dirname(path), G_FILE_TEST_IS_DIR))
	mkdir(g_path_get_dirname(path), 0700);

    ch_dest = g_io_channel_new_file(path_dest, "w", NULL);

    if (!ch_dest)
    {
	g_free(path);
	g_free(path_dest);
	return FALSE;
    }

    g_io_channel_set_encoding(ch_dest, NULL, NULL);

    while (tmp)
    {
	gchar *line1 = NULL;
	GGaduVar *var = (GGaduVar *) tmp->data;

	if (var->ptr != NULL)
	{
	    if (var->type == VAR_STR || var->type == VAR_IMG)
	    {
		if (g_strrstr(var->name, "password") != NULL)
		    line1 = g_strdup_printf("%s \x001%s\n", var->name, base64_encode((gchar *) var->ptr));
		else
		{
		    if (strlen((gchar *) var->ptr) > 0)
			line1 = g_strdup_printf("%s %s\n", var->name, (gchar *) var->ptr);
		}
	    }

	    if ((var->type == VAR_INT || var->type == VAR_BOOL) && (var->ptr != NULL))
		line1 = g_strdup_printf("%s %d\n", var->name, (int) var->ptr);


	    if (line1)
	    {
		print_debug("%s %d\n", line1, var->type);
		g_io_channel_write_chars(ch_dest, line1, -1, &bytes_written, NULL);
	    }
	}
	g_free(line1);

	tmp = tmp->next;
    }

    g_io_channel_shutdown(ch_dest, TRUE, NULL);

    ch_dest = g_io_channel_new_file(path_dest, "a+", NULL);
    g_io_channel_set_encoding(ch_dest, NULL, NULL);

    ch = g_io_channel_new_file(path, "r", NULL);
    if (ch)
    {

	g_io_channel_set_encoding(ch, NULL, NULL);

	while (g_io_channel_read_line(ch, &line, &length, &terminator_pos, NULL) != G_IO_STATUS_EOF)
	{
	    if (!g_str_has_prefix(line, "#"))
	    {
		gchar **spl = g_strsplit(line, " ", 5);

		if (spl && (ggadu_config_var_check(plugin_handler, spl[0]) == -1))
		{
		    g_io_channel_write_chars(ch_dest, line, -1, &bytes_written, NULL);
		    print_debug("set new entry value in file :%s: %s", spl[0], line);
		}

		if (spl)
		    g_strfreev(spl);
	    }
	    else
	    {
		g_io_channel_write_chars(ch_dest, line, -1, &bytes_written, NULL);
	    }

	    g_free(line);
	}
	g_io_channel_shutdown(ch, TRUE, NULL);
    }

    g_io_channel_shutdown(ch_dest, TRUE, NULL);

    if (rename(path_dest, path) == -1)
    {
	print_debug("Failed to rename %s to %s", path_dest, path);
	g_free(path);
	g_free(path_dest);
	return FALSE;
    }

    g_free(path);
    g_free(path_dest);

    return TRUE;
}

/* 
 *    Inicjalizuje plik do zczytania z pliku specyficzne dla tego protokolu
 *    config_file 	- nazwa pliku konfiguracyjnego
 *    encoding 		- kodowanie w jakim jest plik
 */

void ggadu_config_set_filename(GGaduPlugin * plugin_handler, gchar * config_file)
{
    if (plugin_handler == NULL)
	return;

    if (config_file == NULL)
	config_file = plugin_handler->name;

    print_debug("core : config_init_register for %s\n", plugin_handler->name);

    plugin_handler->config_file = config_file;
}
