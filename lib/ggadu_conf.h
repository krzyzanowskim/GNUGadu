/* $Id: ggadu_conf.h,v 1.5 2004/01/17 00:44:58 shaster Exp $ */

#ifndef GGadu_CONF_H
#define GGadu_CONF_H 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gg-types.h"

void ggadu_config_set_filename(GGaduPlugin * plugin_handler, gchar * config_file);

gboolean ggadu_config_read(GGaduPlugin * plugin_handler);

gboolean ggadu_config_save(GGaduPlugin * plugin_handler);

gpointer ggadu_config_var_get(GGaduPlugin * handler, gchar * name);

gint ggadu_config_var_get_type(GGaduPlugin * handler, gchar * name);

void ggadu_config_var_add(GGaduPlugin * handler, gchar * name, gint type);

void ggadu_config_var_add_with_default(GGaduPlugin * handler, gchar * name, gint type, gpointer default_value);

void ggadu_config_var_set(GGaduPlugin * handler, gchar * name, gpointer val);

gint ggadu_config_var_check(GGaduPlugin * handler, gchar * name);

#endif
