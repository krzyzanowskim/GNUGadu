/* $Id: dialog.c,v 1.10 2004/02/14 02:08:08 krzyzak Exp $ */

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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "dialog.h"
#include "support.h"
#include "gg-types.h"
#include "unified-types.h"

/* GGaduDialog *ggadu_dialog_new()
{
	return g_new0(GGaduDialog, 1);
} */

GGaduDialog *ggadu_dialog_new1(guint type, gchar * title, gchar * callback_signal)
{
	GGaduDialog *dialog = g_new0(GGaduDialog, 1);
	ggadu_dialog_callback_signal(dialog, callback_signal);
	ggadu_dialog_set_title(dialog, title);
	ggadu_dialog_set_type(dialog, type);
	return dialog;
}

/* DEPRECATED form !!!!!!!!!!!!!!!! ZONK */
/*void ggadu_dialog_add_entry(GSList ** prefs, gint key, const gchar * desc, gint type, gpointer value, gint flags)
{
	GGaduKeyValue *kv = g_new0(GGaduKeyValue, 1);
	kv->key = key;
	kv->description = g_strdup(desc);
	kv->type = type;


	if ((type == VAR_STR) || (type == VAR_IMG))
		kv->value = g_strdup(value);
	else if (type == VAR_LIST)
		kv->value = g_slist_copy(value);
	else
		kv->value = value;	

	kv->flag = flags;

	*prefs = g_slist_append(*prefs, kv);
}*/


void ggadu_dialog_add_entry1(GGaduDialog * d, gint key, gchar * desc, gint type, gpointer value, gint flags)
{
	GGaduKeyValue *kv = g_new0(GGaduKeyValue, 1);
	kv->key = key;
	kv->description = g_strdup(desc);
	kv->flag = flags;
	
	kv->type = type;
	
	if ((type == VAR_STR) || (type == VAR_IMG))
		kv->value = g_strdup(value);
	else if (type == VAR_LIST)
		kv->value = g_slist_copy(value);
	else
		kv->value = value;

	d->optlist = g_slist_append(ggadu_dialog_get_entries(d), kv);
}

GSList *ggadu_dialog_get_entries(GGaduDialog * dialog)
{
	g_return_val_if_fail(dialog != NULL, NULL);
	return dialog->optlist;
}

GGaduDialogType ggadu_dialog_get_type(GGaduDialog * dialog)
{
	g_return_val_if_fail(dialog != NULL, -1);
	return dialog->type;
}

gint ggadu_dialog_get_response(GGaduDialog * dialog)
{
	g_return_val_if_fail(dialog != NULL, -1);
	return dialog->response;
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

void ggadu_dialog_set_type(GGaduDialog * d, GGaduDialogType type)
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

	e = ggadu_dialog_get_entries(d);
	while (e)
	{
		GGaduKeyValue *kv = (GGaduKeyValue *) e->data;
		GGaduKeyValue_free(kv);
		e = e->next;
	}

	g_slist_free(d->optlist);
	g_free(d);
	return;
}
