/* $Id: plugins.h,v 1.1 2003/06/03 21:30:08 krzyzak Exp $ */
#ifndef GGadu_PROTOCOLS_H
#define GGadu_PROTOCOLS_H 1

#include "gg-types.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define GGadu_CONFIG_DEFINE_PLUGIN GGaduConfig *config;

#define GGadu_PLUGIN_INIT(x,type) \
	GGadu_CONFIG_DEFINE_PLUGIN \
	guint GGADU_PLUGIN_TYPE = type; \
	gchar *ggadu_plugin_name() { return x; } \
	gint  ggadu_plugin_type() { return type; }

#define GGadu_PLUGIN_NAME ggadu_plugin_name()
#define GGadu_PLUGIN_ACTIVATE(x) config = x;
    
#define GGadu_MAX_PLUGIN_NAME 100



gboolean load_plugin(gchar *path);

void unload_plugin(gchar *name);

gboolean config_read(GGaduPlugin *plugin_handler);

gboolean config_save(GGaduPlugin *plugin_handler);

gpointer config_var_get(GGaduPlugin *handler, gchar *name);

void config_var_add(GGaduPlugin *handler, gchar * name, gint type);

void config_var_set(GGaduPlugin *handler, gchar *name, gpointer val);

gint config_var_get_type(GGaduPlugin *handler, gchar *name);

gint config_var_check(GGaduPlugin *handler, gchar *name);

void set_config_file_name(GGaduPlugin *plugin_handler, gchar *config_file);

GGaduPlugin *register_plugin(gchar *name, gchar *desc);

void register_signal_receiver(GGaduPlugin *plugin_handler,void (*sgr)(gpointer,gpointer));

GGaduPlugin *find_plugin_by_name(gchar *name);

GSList *find_plugin_by_pattern(gchar *pattern);

#endif
