/* $Id: signals.h,v 1.3 2003/06/09 01:28:17 krzyzak Exp $ */
#ifndef GGadu_SIGNALS_H
#define GGadu_SIGNALS_H 1

#include "gg-types.h"

/* DEPRECATED */
#define signal_emit(src_name,name,data,dest_data) \
    signal_emit_full(src_name,name,data,dest_data,NULL)
    
GGaduSignalinfo *find_signal(gpointer signal_name);

void *signal_emit_full(gpointer src_name, gpointer name, gpointer data, gpointer dest_name, void (*signal_free) (gpointer));

//void register_signal(GGaduPlugin *plugin_handler,gpointer name, void (*signal_free)());
GQuark register_signal(GGaduPlugin *plugin_handler,gpointer name);
void register_signal_perl (gpointer name, void (*perl_handler) (GGaduSignal *, gchar *, void *));

void hook_signal (gpointer name, void (*hook) (GGaduSignal *signal, void (*perl_handler) (GGaduSignal *, gchar *, void *)));

void flush_queued_signals();

GGaduSignal *signal_cpy(GGaduSignal *sig);

void GGaduSignal_free(GGaduSignal *sig);

#endif

