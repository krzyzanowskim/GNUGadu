/* $Id: signals.h,v 1.1 2003/03/20 10:37:05 krzyzak Exp $ */
#ifndef GGadu_SIGNALS_H
#define GGadu_SIGNALS_H 1

#include "gg-types.h"

/* DEPRECATED */
#define signal_emit(src_name,name,data,dest_data) \
    signal_emit_full(src_name,name,data,dest_data,NULL)
    
GGaduSignalinfo *find_signal(gpointer signal_name);

void *signal_emit_full(gpointer src_name, gpointer name, gpointer data, gpointer dest_name, void (*signal_free) (gpointer));

//void register_signal(GGaduPlugin *plugin_handler,gpointer name, void (*signal_free)());
void register_signal(GGaduPlugin *plugin_handler,gpointer name);

void flush_queued_signals();

GGaduSignal *signal_cpy(GGaduSignal *sig);

void GGaduSignal_free(GGaduSignal *sig);

#endif

