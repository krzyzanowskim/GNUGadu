/* $Id: ggadu_types.c,v 1.4 2004/11/17 11:14:49 krzyzak Exp $ */

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

#ifndef GGadu_UNIFIED_TYPES_FREE_H
#define GGadu_UNIFIED_TYPES_FREE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include "ggadu_types.h"
#include "ggadu_support.h"

void GGaduContact_free(GGaduContact * k)
{
	if (k == NULL)
		return;

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
	g_free(k->resource);
	g_free(k->gender);

	g_free(k);
	return;
}

GGaduContact *GGaduContact_copy(GGaduContact * k)
{
	GGaduContact *kdest;
	
	if (k == NULL)
		return NULL;
		
	kdest = g_new0(GGaduContact,1);

	kdest->id = g_strdup(k->id);
	kdest->first_name = g_strdup(k->first_name);
	kdest->last_name = g_strdup(k->last_name);
	kdest->nick = g_strdup(k->nick);
	kdest->mobile = g_strdup(k->mobile);
	kdest->email = g_strdup(k->email);
	kdest->gender = g_strdup(k->gender);
	kdest->group = g_strdup(k->group);
	kdest->comment = g_strdup(k->comment);
	kdest->birthdate = g_strdup(k->birthdate);
	kdest->status = k->status;
	kdest->status_descr = g_strdup(k->status_descr);
	kdest->city = g_strdup(k->city);
	kdest->age = g_strdup(k->age);
	kdest->resource = g_strdup(k->resource);
	kdest->gender = g_strdup(k->gender);
	return kdest;
}

void GGaduMsg_free(gpointer msg)
{
	GGaduMsg *m = msg;
	
	print_debug("GGaduMsg_free");
	
	if (!m)
		return;

	g_free(m->id);
	g_free(m->message);
	g_slist_free(m->recipients);
	g_free(m);
	return;
}

void GGaduNotify_free(GGaduNotify * n)
{
	if (!n)
		return;

	g_free(n->id);

	g_free(n);
	return;
}


void GGaduStatusPrototype_free(GGaduStatusPrototype * s)
{
	if (!s)
		return;

	g_free(s->description);
	g_free(s->image);

	g_free(s);
	return;
}

void GGaduKeyValue_free(GGaduKeyValue * kv)
{
	if (!kv)
		return;
	
	if ((kv->type == VAR_STR) || (kv->type == VAR_IMG))
	{
		g_free(kv->value);
	} else if (kv->type == VAR_LIST) {
		g_slist_free(kv->value);
	}

	/* ZONK add more */
	
	g_free(kv->description);
	g_free(kv);
}

#endif
