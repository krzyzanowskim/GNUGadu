/* $Id: plugins.h,v 1.4 2003/12/20 23:17:18 krzyzak Exp $ */
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

GGaduPlugin *register_plugin(gchar *name, gchar *desc);

void register_signal_receiver(GGaduPlugin *plugin_handler,void (*sgr)(gpointer,gpointer));

void register_extension_for_plugins(GGaduPluginExtension *ext);

void unregister_extension_for_plugins(GGaduPluginExtension *ext);

GGaduPlugin *find_plugin_by_name(gchar *name);

GSList *find_plugin_by_pattern(gchar *pattern);

GSList *get_list_modules_load();

#endif
