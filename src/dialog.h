#ifndef GGadu_DIALOG_H
#define GGadu_DIALOG_H

#include "unified-types.h"

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

		      
#endif

