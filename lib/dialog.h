/* $Id: dialog.h,v 1.2 2003/06/21 03:57:35 krzyzak Exp $ */

#ifndef GGadu_DIALOG_H
#define GGadu_DIALOG_H

#include "unified-types.h"

enum {
    GGADU_DIALOG_GENERIC,
    GGADU_DIALOG_CONFIG,
    GGADU_DIALOG_YES_NO
};

typedef struct {
    gchar *title;
    gchar *callback_signal;
    gint response;
    GSList *optlist; // lista elementów GGaduKeyValue
    gpointer user_data;
    gint type;
} GGaduDialog;

void GGaduDialog_free(GGaduDialog *d);

GGaduDialog *ggadu_dialog_new();


void ggadu_dialog_add_entry(
		      GSList **prefs, 
		      gint key, 
		      const gchar *desc, 
		      gint type, 
		      gpointer value, 
		      gint flags);

void ggadu_dialog_callback_signal(GGaduDialog *,const gchar *);

void ggadu_dialog_set_title(GGaduDialog *,const gchar *);

void ggadu_dialog_set_type(GGaduDialog *, gint);
		      
#endif

