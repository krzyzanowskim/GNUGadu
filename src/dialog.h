/* $Id: dialog.h,v 1.4 2003/04/04 15:17:30 thrulliq Exp $ */

#ifndef GGadu_DIALOG_H
#define GGadu_DIALOG_H

#include "unified-types.h"

enum {
    GGADU_DIALOG_GENERIC,
    GGADU_DIALOG_CONFIG,
    GGADU_DIALOG_YES_NO
};

GGaduDialog *ggadu_dialog_new();

void ggadu_dialog_add_entry(
		      GSList **prefs, 
		      gint key, 
		      gchar *desc, 
		      gint type, 
		      gpointer value, 
		      gint flags);

void ggadu_dialog_callback_signal(GGaduDialog *,const gchar *);

void ggadu_dialog_set_title(GGaduDialog *,const gchar *);

void ggadu_dialog_set_type(GGaduDialog *, gint);
		      
#endif

