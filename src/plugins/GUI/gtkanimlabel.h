/* $Id: gtkanimlabel.h,v 1.6 2004/01/28 23:40:03 shaster Exp $ */

/* 
 * GUI (gtk+) plugin for GNU Gadu 2 
 * 
 * Copyright (C) 2003 Marcin Krzy¿anowski <krzak@hakore.com>
 * Copyright (C) 2003-2004 GNU Gadu Team 
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


#ifndef __GTK_ANIM_LABEL_H__
#define __GTK_ANIM_LABEL_H__

#include <gdk/gdk.h>
#include <gtk/gtkmisc.h>

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */


#define GTK_ANIM_LABEL(obj)		GTK_CHECK_CAST (obj, gtk_anim_label_get_type(), GtkAnimLabel)
#define GTK_ANIM_LABEL_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, gtk_anim_label_get_type(), GtkAnimLabelClass)
#define GTK_IS_ANIM_LABEL(obj)		GTK_CHECK_TYPE (obj, gtk_anim_label_get_type())

    typedef struct _GtkAnimLabel GtkAnimLabel;
    typedef struct _GtkAnimLabelClass GtkAnimLabelClass;

    struct _GtkAnimLabel
    {
	GtkMisc misc;

	/* displayed text */
	gchar *txt;
	PangoLayout *layout;
	gint timeout_value;	/* timeout value */
	gint timeout_source;	/* GSource */
	gint pos_x;		/* actual position of label */
	gboolean animate;	/* current status TRUE || FALSE */
	gboolean auto_animate;	/* default TRUE */
	gboolean auto_reset;	/* default TRUE */
	gint alignment;		/* default LEFT */
	GdkPixmap *pixmap;	/* pixmap for drawing */
	GTimer *timer;		/* GTimer  */
	guint delay_sec;
    };

    struct _GtkAnimLabelClass
    {
	GtkMiscClass parent_class;
    };

    GtkType gtk_anim_label_get_type(void);

    GtkWidget *gtk_anim_label_new(void);
    GtkWidget *gtk_anim_label_new_with_text(const gchar * txt);
    void gtk_anim_label_set_text(GtkAnimLabel * anim_label, const gchar * txt);
    void gtk_anim_label_animate(GtkAnimLabel * anim_label, gboolean state);
    void gtk_anim_label_set_timeout(GtkAnimLabel * anim_label, gint timeout);
    gint gtk_anim_label_get_timeout(GtkAnimLabel * anim_label);
    void gtk_anim_label_set_delay(GtkAnimLabel * anim_label, guint delay);
    gint gtk_anim_label_get_delay(GtkAnimLabel * anim_label);
    void gtk_anim_label_auto_reset_position(GtkAnimLabel * anim_label, gboolean state);	/* default TRUE */
    void gtk_anim_label_set_alignment(GtkAnimLabel * anim_label, gint alignment);	/* default LEFT */



#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* __GTK_ANIM_LABEL */
