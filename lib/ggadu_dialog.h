/* $Id: ggadu_dialog.h,v 1.12 2004/12/26 22:23:16 shaster Exp $ */

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

#ifndef GGadu_DIALOG_H
#define GGadu_DIALOG_H

#ifndef GG2_CORE_H 
#include "ggadu_types.h"
#endif

typedef gboolean(*watch_func) (gpointer);

typedef enum
{
	GGADU_DIALOG_GENERIC,
	GGADU_DIALOG_CONFIG,
	GGADU_DIALOG_YES_NO/*,
	GGADU_DIALOG_PROGRESS*/
} GGaduDialogType;

typedef enum
{
	GGADU_DIALOG_FLAG_NONE = 0,
	GGADU_DIALOG_FLAG_PROGRESS = 1,
	GGADU_DIALOG_FLAG_ONLY_OK = 2
} GGaduDialogFlags;

typedef struct
{
	gchar *title;
	gchar *callback_signal;
	GSList *optlist;	/* List of GGaduKeyValue's */
	gpointer user_data;
	gint type;
	guint flags; /* GGaduDialogFlags */
	
	gboolean(*watch_func) (gpointer);
	
	/* private */
	gint response;
} GGaduDialog;

#define ggadu_dialog_new(type, title, callback_signal) \
		ggadu_dialog_new_full(type, title, callback_signal, NULL)

GGaduDialog*		ggadu_dialog_new_full			(GGaduDialogType type, gchar * title, gchar * callback_signal, gpointer user_data);

void 			ggadu_dialog_add_entry			(GGaduDialog * dialog, gint key, gchar * desc, GGaduVarType var_type, gpointer value, gint flags);

GSList*			ggadu_dialog_get_entries		(GGaduDialog * dialog);

void			ggadu_dialog_set_callback_signal	(GGaduDialog * dialog, const gchar * signal);

void			ggadu_dialog_set_progress_watch		(GGaduDialog * dialog, watch_func watch);

void			ggadu_dialog_set_title			(GGaduDialog * dialog, const gchar * title);

const gchar*		ggadu_dialog_get_title			(GGaduDialog * dialog);

void			ggadu_dialog_set_type			(GGaduDialog * dialog, GGaduDialogType dialog_type);

GGaduDialogType		ggadu_dialog_get_type			(GGaduDialog * dialog);

gint 			ggadu_dialog_get_response		(GGaduDialog * dialog);

void			ggadu_dialog_set_flags			(GGaduDialog * dialog, guint flags);

guint			ggadu_dialog_get_flags			(GGaduDialog * dialog);

void			GGaduDialog_free			(GGaduDialog * dialog);

#endif
