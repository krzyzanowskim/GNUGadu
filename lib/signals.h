/* $Id: signals.h,v 1.9 2004/05/04 21:39:08 krzyzak Exp $ */

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

#ifndef GGadu_SIGNALS_H
#define GGadu_SIGNALS_H 1

#include "ggadu_types.h"

/* DEPRECATED */
#define signal_emit(src_name,name,data,dest_data) \
    signal_emit_full(src_name,name,data,dest_data,NULL)

#define signal_emit_from_thread(src_name,name,data,dest_data) \
    signal_emit_from_thread_full(src_name,name,data,dest_data,NULL)

GGaduSignalinfo *find_signal(gpointer signal_name);

void *signal_emit_full(gpointer src_name, gpointer name, gpointer data, gpointer dest_name, void (*signal_free) (gpointer));

GGaduSigID register_signal(GGaduPlugin * plugin_handler, gpointer name);

void register_signal_perl(gchar * name, void (*perl_handler) (GGaduSignal *, gchar *, void *));

void hook_signal(GGaduSigID q_name, void (*hook) (GGaduSignal * signal, void (*perl_handler) (GGaduSignal *, gchar *, void *)));

void flush_queued_signals();

GGaduSignal *signal_cpy(GGaduSignal * sig);

void GGaduSignal_free(GGaduSignal * sig);

gboolean signal_from_thread_enabled();

void signal_emit_from_thread_full(gpointer src_name, gpointer name, gpointer data, gpointer dest_name, void (*signal_free) (gpointer));

#endif
