/* $Id: ggadu_conf.h,v 1.12 2004/12/26 22:23:16 shaster Exp $ */

/* 
 * GNU Gadu 2 
 * 
 * Copyright (C) 2001-2005 GNU Gadu Team 
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

#ifndef GGadu_CONF_H
#define GGadu_CONF_H 1

#ifndef GG2_CORE_H
#include "ggadu_types.h"
#endif

void		ggadu_config_set_filename	(GGaduPlugin * plugin_handler, gchar *config_file);

gboolean	ggadu_config_read_from_file	(GGaduPlugin * plugin_handler, gchar *file_path);

gboolean	ggadu_config_read		(GGaduPlugin * plugin_handler);

gboolean	ggadu_config_save		(GGaduPlugin * plugin_handler);

gpointer	ggadu_config_var_get		(GGaduPlugin * handler, gchar * name);

void		ggadu_config_var_set		(GGaduPlugin * handler, gchar * name, gpointer val);

GGaduVarType	ggadu_config_var_get_type	(GGaduPlugin * handler, gchar * name);

gint		ggadu_config_var_check		(GGaduPlugin * handler, gchar * name);

void		ggadu_config_var_add		(GGaduPlugin * handler, gchar * name, gint type);

void		ggadu_config_var_add_with_default(GGaduPlugin * handler, gchar * name, gint type, gpointer default_value);

#endif
