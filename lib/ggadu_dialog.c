/* $Id: ggadu_dialog.c,v 1.1 2004/02/14 16:46:53 krzyzak Exp $ */

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

#include "ggadu_dialog.h"
#include "support.h"
#include "gg-types.h"
#include "unified-types.h"

GGaduDialog *ggadu_dialog_new_full(guint type, gchar * title, gchar * callback_signal, gpointer user_data)
{
	GGaduDialog *dialog = g_new0(GGaduDialog, 1);
	ggadu_dialog_set_callback_signal(dialog, callback_signal);
	ggadu_dialog_set_title(dialog, title);
	ggadu_dialog_set_type(dialog, type);
	dialog->user_data = user_data;
	return dialog;
}

void ggadu_dialog_add_entry(GGaduDialog * dialog, gint key, gchar * desc, gint type, gpointer value, gint flags)
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

	dialog->optlist = g_slist_append(ggadu_dialog_get_entries(dialog), kv);
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

void ggadu_dialog_set_callback_signal(GGaduDialog * dialog, const gchar * title)
{
	g_return_if_fail(dialog != NULL);
	dialog->callback_signal = g_strdup(title);
}

void ggadu_dialog_set_title(GGaduDialog * dialog, const gchar * title)
{
	g_return_if_fail(dialog != NULL);
	dialog->title = g_strdup(title);
}

const gchar *ggadu_dialog_get_title(GGaduDialog * dialog)
{
	g_return_if_fail(dialog != NULL);
	return dialog->title;
}

void ggadu_dialog_set_type(GGaduDialog * dialog, GGaduDialogType type)
{
	g_return_if_fail(dialog != NULL);
	dialog->type = type;
}

void GGaduDialog_free(GGaduDialog * dialog)
{
	GSList *e = NULL;

	if (!dialog)
		return;

	g_free(dialog->title);
	g_free(dialog->callback_signal);

	e = ggadu_dialog_get_entries(dialog);
	while (e)
	{
		GGaduKeyValue *kv = (GGaduKeyValue *) e->data;
		GGaduKeyValue_free(kv);
		e = e->next;
	}
	g_slist_free(dialog->optlist);
	g_free(dialog);
	return;
}
