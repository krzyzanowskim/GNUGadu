#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "support.h"
#include "gg-types.h"
#include "unified-types.h"

GGaduDialog *ggadu_dialog_new() {
    return g_new0(GGaduDialog,1);
}

void ggadu_dialog_add_entry(
		      GSList **prefs, 
		      gint key, 
		      gchar *desc, 
		      gint type, 
		      gpointer value, 
		      gint flags)
{
    GGaduKeyValue *kv = g_new0(GGaduKeyValue,1);
    kv->key		= key;
    kv->description	= g_strdup(desc);
    kv->type		= type;
    kv->value		= value;
    kv->flag		= flags;
    
    *prefs = g_slist_append(*prefs, kv);
}

void ggadu_dialog_callback_signal(GGaduDialog *d,const gchar *t)
{
    d->callback_signal = (gchar *)t;
}

void ggadu_dialog_set_title(GGaduDialog *d,const gchar *t)
{
    d->title = (gchar *)t; 
}
