/* $Id: dialog.c,v 1.4 2004/01/21 17:43:40 thrulliq Exp $ */
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "dialog.h"
#include "support.h"
#include "gg-types.h"
#include "unified-types.h"

GGaduDialog *ggadu_dialog_new()
{
    return g_new0(GGaduDialog, 1);
}

void ggadu_dialog_add_entry(GSList ** prefs, gint key, const gchar * desc, gint type, gpointer value, gint flags)
{
    GGaduKeyValue *kv = g_new0(GGaduKeyValue, 1);
    kv->key = key;
    kv->description = g_strdup(desc);
    kv->type = type;
    kv->value = value;
    kv->flag = flags;

    *prefs = g_slist_append(*prefs, kv);
}

void ggadu_dialog_callback_signal(GGaduDialog * d, const gchar * t)
{
    g_return_if_fail(d != NULL);
    d->callback_signal = g_strdup(t);
}

void ggadu_dialog_set_title(GGaduDialog * d, const gchar * t)
{
    g_return_if_fail(d != NULL);
    d->title = g_strdup(t);
}

void ggadu_dialog_set_type(GGaduDialog * d, gint type)
{
    g_return_if_fail(d != NULL);
    d->type = type;
}

void GGaduDialog_free(GGaduDialog * d)
{
    GSList *e = NULL;

    if (!d)
	return;

    g_free(d->title);
    g_free(d->callback_signal);

    e = d->optlist;
    while (e)
    {
	GGaduKeyValue *kv = (GGaduKeyValue *) e->data;
/*
	g_free(kv->value);
*/
	g_free(kv->description);
	g_free(e->data);
/*
	GGaduKeyValue_free(kv);
*/
	e = e->next;
    }
    g_slist_free(d->optlist);

    g_free(d);
    return;
}
