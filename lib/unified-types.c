/* $Id: unified-types.c,v 1.1 2003/06/03 21:30:11 krzyzak Exp $ */
#ifndef GGadu_UNIFIED_TYPES_FREE_H
#define GGadu_UNIFIED_TYPES_FREE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include "unified-types.h"

void GGaduContact_free(GGaduContact *k)
{
    if (k == NULL) return;
    
    g_free(k->id);
    g_free(k->first_name);
    g_free(k->last_name);
    g_free(k->nick);
    g_free(k->mobile);
    g_free(k->email);
    g_free(k->gender);
    g_free(k->group);
    g_free(k->comment);
    g_free(k->birthdate);
    g_free(k->status_descr);
    g_free(k->city);
    g_free(k->age);
    g_free(k->gender);
    
    g_free(k);
    return;
}

void GGaduMsg_free(GGaduMsg *m)
{
    if (!m) return;
    
    g_free(m->id);
    g_free(m->message);
    
    g_free(m);
    return;
}

void GGaduNotify_free(GGaduNotify *n)
{
    if (!n) return;

    g_free(n->id);
    
    g_free(n);
    return;
}


void GGaduStatusPrototype_free(GGaduStatusPrototype *s)
{
    if (!s) return;
    
    g_free(s->description);
    g_free(s->image);
    
    g_free(s);
    return;
}

void GGaduKeyValue_free(GGaduKeyValue *kv)
{
    if (!kv) return;
    g_free(kv->value);
    g_free(kv->description);
    g_free(kv);
}

void GGaduDialog_free(GGaduDialog *d)
{
    GSList *e = NULL;
    
    if (!d) return;
    return;
    g_free(d->title);
    g_free(d->callback_signal);
    
    e = d->optlist;
    while (e) 
    {
	GGaduKeyValue *kv = (GGaduKeyValue *)e->data;
//	g_free(kv->value);
	g_free(kv->description);
	g_free(e->data);
//	GGaduKeyValue_free(kv);
	e = e->next;
    }
    g_slist_free(d->optlist);

    g_free(d);
    return;
}

#endif
