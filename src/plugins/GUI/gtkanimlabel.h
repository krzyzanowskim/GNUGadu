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
