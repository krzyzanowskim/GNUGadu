/* $Id: plugins.h,v 1.7 2004/05/04 21:39:08 krzyzak Exp $ */

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

#ifndef GGadu_PROTOCOLS_H
#define GGadu_PROTOCOLS_H 1

#include "ggadu_types.h"

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

gboolean load_plugin(gchar * path);

void unload_plugin(gchar * name);

GGaduPlugin *register_plugin(gchar * name, gchar * desc);

void register_signal_receiver(GGaduPlugin * plugin_handler, void (*sgr) (gpointer, gpointer));

void register_extension_for_plugins(GGaduPluginExtension * ext);

void unregister_extension_for_plugins(GGaduPluginExtension * ext);

GGaduPlugin *find_plugin_by_name(gchar * name);

GSList *find_plugin_by_pattern(gchar * pattern);

GSList *get_list_modules_load();

#endif
