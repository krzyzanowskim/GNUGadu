/*
 *  (C) Copyright 2003 Marcin Krzy¿anowski <krzak@hakore.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

 /*
    GObject
    +----GtkObject
    +----GtkWidget
    +----GtkMisc
    +----GtkAnimLabel
  */

#include <stdio.h>
#include <string.h>

#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkclipboard.h>
#include <gtk/gtkmisc.h>
#include <pango/pango.h>

#include "gtkanimlabel.h"

static void gtk_anim_label_class_init (GtkAnimLabelClass * klass);
static void gtk_anim_label_init (GtkAnimLabel * anim_label);
static void gtk_anim_label_destroy (GtkObject * object);
static void gtk_anim_label_finalize (GObject * object);
static void gtk_anim_label_realize (GtkWidget * widget);
static gint gtk_anim_label_expose (GtkWidget * widget, GdkEventExpose * event);
static void gtk_anim_label_size_request (GtkWidget * widget, GtkRequisition * requisition);
static void gtk_anim_label_size_allocate (GtkWidget * widget, GtkAllocation * allocation);
static gint gtk_anim_label_button_press (GtkWidget * widget, GdkEventButton * event);
/* private */
static void anim_label_create_layout (GtkAnimLabel * anim_label, const gchar * txt);
static void anim_label_create_pixmap (GtkAnimLabel * anim_label);

static GtkMiscClass *parent_class = NULL;

GType gtk_anim_label_get_type ()
{
    static GType anim_label_type = 0;

    if (!anim_label_type)
      {
	  static const GTypeInfo anim_label_info = {
	      sizeof (GtkAnimLabelClass),
	      NULL,
	      NULL,
	      (GClassInitFunc) gtk_anim_label_class_init,
	      NULL,
	      NULL,
	      sizeof (GtkAnimLabel),
	      0,
	      (GInstanceInitFunc) gtk_anim_label_init,
	  };

	  anim_label_type = g_type_register_static (GTK_TYPE_MISC, "GtkAnimLabel", &anim_label_info, 0);
      }

    return anim_label_type;
}

static void gtk_anim_label_class_init (GtkAnimLabelClass * klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);

    object_class->destroy = gtk_anim_label_destroy;
    gobject_class->finalize = gtk_anim_label_finalize;

    widget_class->realize = gtk_anim_label_realize;
    widget_class->expose_event = gtk_anim_label_expose;
    widget_class->size_request = gtk_anim_label_size_request;
    widget_class->size_allocate = gtk_anim_label_size_allocate;
    widget_class->button_press_event = gtk_anim_label_button_press;

}


static void gtk_anim_label_init (GtkAnimLabel * anim_label)
{
    anim_label->txt = NULL;
    anim_label->layout = NULL;
    anim_label->timeout_value = 100;
    anim_label->pos_x = 0;
    anim_label->auto_reset = TRUE;
    anim_label->alignment = PANGO_ALIGN_LEFT;
    anim_label->auto_animate = TRUE;
    anim_label->animate = FALSE;
    anim_label->pixmap = NULL;
    anim_label->timer = NULL;
    anim_label->delay_sec = 0;
}


static void gtk_anim_label_destroy (GtkObject * object)
{
    GtkAnimLabel *anim_label;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GTK_IS_ANIM_LABEL (object));

    anim_label = GTK_ANIM_LABEL (object);

    gtk_anim_label_animate (anim_label, FALSE);
    
    if (anim_label->timer)
       g_timer_stop(anim_label->timer);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
	(*GTK_OBJECT_CLASS (parent_class)->destroy) (object);

}

static void gtk_anim_label_finalize (GObject * object)
{
    GtkAnimLabel *anim_label;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GTK_IS_ANIM_LABEL (object));

    anim_label = GTK_ANIM_LABEL (object);

    g_free (anim_label->txt);

    if (anim_label->layout)
	g_object_unref (anim_label->layout);

    if (anim_label->pixmap)
	g_object_unref (anim_label->pixmap);

    if (anim_label->timer)
	g_timer_destroy (anim_label->timer);

    G_OBJECT_CLASS (parent_class)->finalize (object);
}


GtkWidget *gtk_anim_label_new_with_text (const gchar * txt)
{
    GtkAnimLabel *anim_label;

    anim_label = g_object_new (gtk_anim_label_get_type (), NULL);

    gtk_anim_label_set_text (anim_label, txt);	/* get a copy of txt */

    return GTK_WIDGET (anim_label);
}


GtkWidget *gtk_anim_label_new (void)
{
    return GTK_WIDGET (gtk_anim_label_new_with_text (NULL));
}


void gtk_anim_label_set_text (GtkAnimLabel * anim_label, const gchar * txt)
{
    g_return_if_fail (anim_label != NULL);
    g_return_if_fail (GTK_IS_ANIM_LABEL (anim_label));


    if (anim_label->txt)
      {
	  g_free (anim_label->txt);
	  anim_label->txt = NULL;
      }

    if ((txt == NULL) || (strlen (txt) <= 0))
	return;

    if (txt != NULL)
	anim_label->txt = g_strdup (txt);

    if (anim_label->auto_reset)
	anim_label->pos_x = 0;

    if ((!anim_label->timer) && (anim_label->delay_sec > 0))
	anim_label->timer = g_timer_new ();
    else if (anim_label->delay_sec > 0)
	g_timer_start (anim_label->timer);


    if (anim_label->layout)
      {
	  g_object_unref (G_OBJECT (anim_label->layout));
	  anim_label->layout = NULL;
      }

    if (anim_label->pixmap)
      {
	  g_object_unref (G_OBJECT (anim_label->pixmap));
	  anim_label->pixmap = NULL;
      }

    anim_label_create_layout (anim_label, (anim_label->txt) ? anim_label->txt : "");

    if (!anim_label->pixmap && GTK_WIDGET_REALIZED (GTK_WIDGET (anim_label)))
      {
	  anim_label_create_pixmap (anim_label);
      }

    gtk_widget_queue_resize (GTK_WIDGET (anim_label));
}

static void gtk_anim_label_realize (GtkWidget * widget)
{
    GtkAnimLabel *anim_label;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_ANIM_LABEL (widget));

    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
    anim_label = GTK_ANIM_LABEL (widget);

    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK;
/*    attributes.event_mask =
	gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
	GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK;
*/
    attributes.visual = gtk_widget_get_visual (widget);
    attributes.colormap = gtk_widget_get_colormap (widget);

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    widget->window = gdk_window_new (widget->parent->window, &attributes, attributes_mask);

    widget->style = gtk_style_attach (widget->style, widget->window);

    gdk_window_set_user_data (widget->window, widget);

    gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
}


static void gtk_anim_label_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
    GtkAnimLabel *anim_label = NULL;
    PangoRectangle prect;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_ANIM_LABEL (widget));
    g_return_if_fail (requisition != NULL);

    anim_label = GTK_ANIM_LABEL (widget);

    requisition->width = 1;
    requisition->height = 1;

    if (anim_label->layout && GTK_WIDGET_MAPPED (widget) && (anim_label->txt) && (strlen (anim_label->txt) > 0))
      {
	  pango_layout_get_extents (anim_label->layout, NULL, &prect);
	  requisition->height = PANGO_PIXELS (prect.height);
	  return;
      }
}


static void gtk_anim_label_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
    PangoRectangle prect;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_ANIM_LABEL (widget));
    g_return_if_fail (allocation != NULL);

    widget->allocation = *allocation;

    if (GTK_WIDGET_REALIZED (widget))
      {
	  gdk_window_move_resize (widget->window, allocation->x, allocation->y, allocation->width, allocation->height);

	  if (((GTK_ANIM_LABEL (widget)->animate) || (GTK_ANIM_LABEL (widget)->auto_animate)) &&
	      (GTK_ANIM_LABEL (widget)->layout))
	    {
		pango_layout_get_extents (GTK_ANIM_LABEL (widget)->layout, NULL, &prect);
		if (PANGO_PIXELS (prect.width) < widget->allocation.width)
		  {
		      GTK_ANIM_LABEL (widget)->pos_x = 0;
		      gtk_anim_label_animate (GTK_ANIM_LABEL (widget), FALSE);
		  }
		else if ((GTK_ANIM_LABEL (widget)->auto_animate) && (!GTK_ANIM_LABEL (widget)->animate))
		  {
		      GTK_ANIM_LABEL (widget)->pos_x = 0;
		      gtk_anim_label_animate (GTK_ANIM_LABEL (widget), TRUE);
		  }
	    }
      }

}

static gint gtk_anim_label_expose (GtkWidget * widget, GdkEventExpose * event)
{
    GtkAnimLabel *anim_label = NULL;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_ANIM_LABEL (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    anim_label = GTK_ANIM_LABEL (widget);

    anim_label_create_layout (anim_label, (anim_label->txt) ? anim_label->txt : "");
    anim_label_create_pixmap (anim_label);

    gdk_draw_drawable (widget->window, widget->style->bg_gc[widget->state], anim_label->pixmap, 0, 0, anim_label->pos_x,
		       0, widget->allocation.width, widget->allocation.height);

    return FALSE;
}

static gint anim_label_timeout_callback (gpointer user_data)
{
    GtkAnimLabel *anim_label = NULL;
    GtkWidget *widget = NULL;
    PangoRectangle prect;

    g_return_val_if_fail (user_data != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_ANIM_LABEL (user_data), FALSE);

    anim_label = GTK_ANIM_LABEL (user_data);

    if (anim_label->animate == FALSE)
	return FALSE;


    if (anim_label->delay_sec > 0)
      {
	  gint sec;
	  gulong msec;

	  sec = (gint) g_timer_elapsed (anim_label->timer, &msec);

	  if (sec >= anim_label->delay_sec)
	      g_timer_stop (anim_label->timer);
	  else
	      return TRUE;
      }

    widget = GTK_WIDGET (anim_label);

    pango_layout_get_extents (anim_label->layout, NULL, &prect);

    /* stop animate if whole label can be drawn in window */
    if (PANGO_PIXELS (prect.width) < widget->allocation.width)
	return FALSE;

    anim_label->pos_x = anim_label->pos_x - 1;

    if ((anim_label->pos_x + (PANGO_PIXELS (prect.width))) < 0)
	anim_label->pos_x = widget->allocation.width - 1;

    gdk_draw_drawable (widget->window, widget->style->bg_gc[widget->state], anim_label->pixmap, 0, 0, anim_label->pos_x,
		       0, PANGO_PIXELS (prect.width) + 5, PANGO_PIXELS (prect.height));

    return TRUE;
}


void gtk_anim_label_animate (GtkAnimLabel * anim_label, gboolean state)
{
    g_return_if_fail (anim_label != NULL);
    g_return_if_fail (GTK_IS_ANIM_LABEL (anim_label));

    if ((anim_label->animate == TRUE) && (anim_label->timeout_source > 0))
      {
	  g_source_remove (anim_label->timeout_source);	/* or g_source_destroy ??? */
      }

    if (state == TRUE)
      {
	  anim_label->timeout_source =
	      g_timeout_add (anim_label->timeout_value, anim_label_timeout_callback, anim_label);
      }
    else if (anim_label->timeout_source > 0)
      {
	  g_source_remove (anim_label->timeout_source);	/* or g_source_destroy ??? */
      }

    anim_label->animate = state;
}


void gtk_anim_label_set_timeout (GtkAnimLabel * anim_label, gint timeout)
{
    g_return_if_fail (anim_label != NULL);
    g_return_if_fail (GTK_IS_ANIM_LABEL (anim_label));

    gtk_anim_label_animate (anim_label, FALSE);
    anim_label->timeout_value = timeout;
    gtk_anim_label_animate (anim_label, TRUE);
}


gint gtk_anim_label_get_timeout (GtkAnimLabel * anim_label)
{
    g_return_val_if_fail (anim_label != NULL, -1);
    g_return_val_if_fail (GTK_IS_ANIM_LABEL (anim_label), -1);

    return anim_label->timeout_value;
}

void gtk_anim_label_auto_reset_position (GtkAnimLabel * anim_label, gboolean state)
{
    g_return_if_fail (anim_label != NULL);
    g_return_if_fail (GTK_IS_ANIM_LABEL (anim_label));

    anim_label->auto_reset = state;
}

void gtk_anim_label_set_alignment (GtkAnimLabel * anim_label, gint alignment)
{
    g_return_if_fail (anim_label != NULL);
    g_return_if_fail (GTK_IS_ANIM_LABEL (anim_label));
    g_return_if_fail (anim_label->layout != NULL);

    anim_label->alignment = alignment;
    pango_layout_set_alignment (anim_label->layout, alignment);

}

/* helper functions */
static void anim_label_create_layout (GtkAnimLabel * anim_label, const gchar * txt)
{
    g_return_if_fail (anim_label != NULL);
    g_return_if_fail (GTK_IS_ANIM_LABEL (anim_label));

    if (!anim_label->layout)
      {
	  anim_label->layout = gtk_widget_create_pango_layout (GTK_WIDGET (anim_label), NULL);
	  pango_layout_set_markup (anim_label->layout, txt, -1);
	  /* pango_layout_set_alignment (anim_label->layout, anim_label->alignment); */
      }


}

static void anim_label_create_pixmap (GtkAnimLabel * anim_label)
{

    PangoRectangle prect;
    GtkWidget *widget = GTK_WIDGET (anim_label);

    if (anim_label->pixmap)
	return;

    /* tworze bitmape pod ten tekst */
    pango_layout_get_extents (anim_label->layout, NULL, &prect);
    anim_label->pixmap =
	gdk_pixmap_new (widget->window, PANGO_PIXELS (prect.width) + 5, PANGO_PIXELS (prect.height), -1);

    /* czyscze pixmape */
    gdk_draw_rectangle (anim_label->pixmap, widget->style->bg_gc[widget->state], TRUE, 0, 0,
			PANGO_PIXELS (prect.width) + 5, PANGO_PIXELS (prect.height));

    /* maluje na niej tekst */
    gdk_draw_layout (anim_label->pixmap, widget->style->fg_gc[widget->state], 0, 0, anim_label->layout);

}


void gtk_anim_label_set_delay (GtkAnimLabel * anim_label, guint delay)
{
    g_return_if_fail (anim_label != NULL);
    g_return_if_fail (GTK_IS_ANIM_LABEL (anim_label));
    g_return_if_fail (delay >= 0);

    anim_label->delay_sec = delay - 1;
}

gint gtk_anim_label_get_delay (GtkAnimLabel * anim_label)
{
    g_return_val_if_fail (anim_label != NULL, 0);
    g_return_val_if_fail (GTK_IS_ANIM_LABEL (anim_label), 0);

    return anim_label->delay_sec;
}

static gint gtk_anim_label_button_press (GtkWidget * widget, GdkEventButton * event)
{
    GtkAnimLabel *anim_label;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_ANIM_LABEL (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    anim_label = GTK_ANIM_LABEL (widget);

    if (anim_label->txt)
      {

	  char *plain = (char *) pango_layout_get_text (anim_label->layout);
	  gtk_clipboard_set_text (gtk_widget_get_clipboard (GTK_WIDGET (anim_label), GDK_SELECTION_CLIPBOARD), plain,
				  -1);

	  g_print ("BUTTON PRESS %s\n", plain);
      }

    return FALSE;
}
