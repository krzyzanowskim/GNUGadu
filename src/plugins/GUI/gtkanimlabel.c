#include <stdio.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <pango/pango.h>

#include "gtkanimlabel.h"

static void gtk_anim_label_class_init (GtkAnimLabelClass *klass);
static void gtk_anim_label_init (GtkAnimLabel *anim_label);
static void gtk_anim_label_destroy (GtkObject *object);
static void gtk_anim_label_realize (GtkWidget *widget);
static gint gtk_anim_label_expose (GtkWidget *widget, GdkEventExpose *event);
static void gtk_anim_label_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void gtk_anim_label_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static void anim_label_create_layout(GtkAnimLabel *anim_label,const gchar *txt);

static GtkWidgetClass *parent_class = NULL;

GType
gtk_anim_label_get_type ()
{
  static GType anim_label_type = 0;

  if (!anim_label_type)
    {
      static const GTypeInfo anim_label_info =
      {
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

      anim_label_type = g_type_register_static (GTK_TYPE_WIDGET, "GtkAnimLabel", &anim_label_info, 0);
    }

  return anim_label_type;
}

static void
gtk_anim_label_class_init (GtkAnimLabelClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	
	object_class = (GtkObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;
	
	parent_class = gtk_type_class (gtk_widget_get_type());
	
	object_class->destroy = gtk_anim_label_destroy;
	
	widget_class->realize = gtk_anim_label_realize;
	widget_class->expose_event = gtk_anim_label_expose;
	widget_class->size_request = gtk_anim_label_size_request;
	widget_class->size_allocate = gtk_anim_label_size_allocate;
}


static void
gtk_anim_label_init (GtkAnimLabel *anim_label)
{
	anim_label->txt = NULL;
	anim_label->layout = NULL;
	anim_label->timeout_value = 100;
	anim_label->pos_x = 0;
	anim_label->auto_reset = TRUE;
	anim_label->alignment = PANGO_ALIGN_LEFT;
}


static void
gtk_anim_label_destroy (GtkObject *object)
{
	GtkAnimLabel *anim_label;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (GTK_IS_ANIM_LABEL (object));
	
	anim_label = GTK_ANIM_LABEL(object);
	
	g_free(anim_label->txt);
	/* g_object_unref(G_OBJECT(anim_label->layout)); */
	if (anim_label->timeout_source > 0)
		g_source_remove(anim_label->timeout_source);
	
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy) (object);
}

GtkWidget*
gtk_anim_label_new_with_text (const gchar *txt)
{
	GtkAnimLabel *anim_label;
	
	anim_label = g_object_new (gtk_anim_label_get_type (), NULL);
	
	gtk_anim_label_set_text(anim_label,g_strdup(txt)); /* get a copy of txt */
	
	return GTK_WIDGET(anim_label);
}


GtkWidget*
gtk_anim_label_new (void)
{
	return GTK_WIDGET(gtk_anim_label_new_with_text(NULL));
}


void
gtk_anim_label_set_text (GtkAnimLabel *anim_label, const gchar *txt)
{
	g_return_if_fail (anim_label != NULL);
	g_return_if_fail (GTK_IS_ANIM_LABEL(anim_label));
	
	if (!txt) return;
	
	

	if (anim_label->txt) g_free(anim_label->txt);
	
	anim_label->txt = g_strdup(txt);
	
	if (anim_label->auto_reset) anim_label->pos_x = 0;
	
	if (anim_label->layout) {
		g_object_unref(G_OBJECT(anim_label->layout));
		anim_label->layout = NULL;
	} else {
		anim_label->layout = gtk_widget_create_pango_layout (GTK_WIDGET (anim_label), (anim_label->txt) ? anim_label->txt : "");
		pango_layout_set_markup (anim_label->layout, (anim_label->txt) ? anim_label->txt : "", -1);
	}
		
	/* gtk_anim_label_animate(anim_label,FALSE); */
	gtk_widget_queue_resize (GTK_WIDGET (anim_label));
}

static void
gtk_anim_label_realize (GtkWidget *widget)
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
  attributes.event_mask = gtk_widget_get_events (widget) | 
    GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | 
    GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
    GDK_POINTER_MOTION_HINT_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window, &attributes, attributes_mask);

  widget->style = gtk_style_attach (widget->style, widget->window);

  gdk_window_set_user_data (widget->window, widget);

  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);
}


static void
gtk_anim_label_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	GtkAnimLabel *anim_label = NULL;
	PangoRectangle prect;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_ANIM_LABEL(widget));
	g_return_if_fail (requisition != NULL);
	
	anim_label = GTK_ANIM_LABEL(widget);

/*	if (!anim_label->layout) {
		anim_label->layout = gtk_widget_create_pango_layout (GTK_WIDGET (anim_label), (anim_label->txt) ? anim_label->txt : "");
		pango_layout_set_markup (anim_label->layout, (anim_label->txt) ? anim_label->txt : "", -1);
	}
*/
	anim_label_create_layout(anim_label, (anim_label->txt) ? anim_label->txt : "");
	pango_layout_get_extents (anim_label->layout, NULL, &prect);
	
/*	requisition->width = PANGO_PIXELS (prect.width);
	requisition->height = PANGO_PIXELS (prect.height);
*/
	requisition->width = 10;
	requisition->height = PANGO_PIXELS (prect.height);
}


static void 
gtk_anim_label_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	GtkAnimLabel *anim_label;
	
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_ANIM_LABEL(widget));
	g_return_if_fail (allocation != NULL);
	
	widget->allocation = *allocation;
	if (GTK_WIDGET_REALIZED (widget))
		{
		anim_label = GTK_ANIM_LABEL(widget);
		
		gdk_window_move_resize (widget->window, allocation->x, allocation->y,
					allocation->width, allocation->height);
					

		}
}

static gint
gtk_anim_label_expose (GtkWidget *widget, GdkEventExpose *event)
{
	GtkAnimLabel *anim_label;
	
	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_ANIM_LABEL (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);
	
	anim_label = GTK_ANIM_LABEL(widget);
	
	gdk_window_clear_area (widget->window, 0,0, widget->allocation.width, widget->allocation.height);
	
	anim_label_create_layout(anim_label, (anim_label->txt) ? anim_label->txt : "");
	
	gdk_draw_layout(widget->window,
			widget->style->fg_gc[widget->state],
			anim_label->pos_x,0,
			anim_label->layout);
	
	return FALSE;
}

static gint anim_label_timeout_callback (gpointer user_data)
{
	GtkAnimLabel *anim_label = NULL;
	GtkWidget *widget = NULL;
	PangoRectangle prect;

	g_return_val_if_fail (user_data != NULL,FALSE);
	g_return_val_if_fail (GTK_IS_ANIM_LABEL(user_data),FALSE);
	
	
	anim_label = GTK_ANIM_LABEL(user_data);
	if (anim_label->animate == FALSE) return FALSE;
	
	widget = GTK_WIDGET(anim_label);
	anim_label->pos_x = anim_label->pos_x - 1;
	
	pango_layout_get_extents (anim_label->layout, NULL, &prect);
	
	if ( (anim_label->pos_x + (PANGO_PIXELS(prect.width))) < 1) 
		anim_label->pos_x = widget->allocation.width - 1;
	
	gdk_window_clear_area (widget->window, 0,0, widget->allocation.width, widget->allocation.height);
	gdk_draw_layout(widget->window,
			widget->style->fg_gc[widget->state],
			anim_label->pos_x,0,
			anim_label->layout);
			
	return TRUE;
}


void
gtk_anim_label_animate	(GtkAnimLabel *anim_label, gboolean state)
{
	g_return_if_fail (anim_label != NULL);
	g_return_if_fail (GTK_IS_ANIM_LABEL(anim_label));

	if ((anim_label->animate == TRUE) && (anim_label->timeout_source > 0))
	{
		g_source_remove(anim_label->timeout_source); /* or g_source_destroy ??? */
	}
	
//	if ((state == TRUE) && (GTK_WIDGET_VISIBLE(GTK_WIDGET(anim_label))))
	if (state == TRUE)
	{
		anim_label->timeout_source = g_timeout_add(anim_label->timeout_value,anim_label_timeout_callback,anim_label);
	} 
	else if (anim_label->timeout_source > 0)
	{
		g_source_remove(anim_label->timeout_source); /* or g_source_destroy ??? */
	}
	
	anim_label->animate = state;

}


void
gtk_anim_label_set_timeout (GtkAnimLabel *anim_label, gint timeout)
{
	g_return_if_fail (anim_label != NULL);
	g_return_if_fail (GTK_IS_ANIM_LABEL(anim_label));
	
	gtk_anim_label_animate(anim_label,FALSE);
	anim_label->timeout_value = timeout;
	gtk_anim_label_animate(anim_label,TRUE);
}

gint
gtk_anim_label_get_timeout (GtkAnimLabel *anim_label)
{
	g_return_val_if_fail (anim_label != NULL,-1);
	g_return_val_if_fail (GTK_IS_ANIM_LABEL(anim_label),-1);

	return anim_label->timeout_value;
}

void
gtk_anim_label_auto_reset_position (GtkAnimLabel *anim_label, gboolean state)
{
	g_return_if_fail (anim_label != NULL);
	g_return_if_fail (GTK_IS_ANIM_LABEL(anim_label));

	anim_label->auto_reset = state;
}

void
gtk_anim_label_set_alignment	(GtkAnimLabel *anim_label, gint alignment) 
{
	g_return_if_fail (anim_label != NULL);
	g_return_if_fail (GTK_IS_ANIM_LABEL(anim_label));
	
	if (!anim_label->layout) return;
	
	anim_label->alignment = alignment;
	pango_layout_set_alignment (anim_label->layout, alignment);
	
}

static void 
anim_label_create_layout(GtkAnimLabel *anim_label,const gchar *txt)
{
	g_return_if_fail (anim_label != NULL);
	g_return_if_fail (GTK_IS_ANIM_LABEL(anim_label));
	
	if (!anim_label->layout) {
		anim_label->layout = gtk_widget_create_pango_layout (GTK_WIDGET (anim_label), (anim_label->txt) ? anim_label->txt : "");
		pango_layout_set_markup (anim_label->layout, (anim_label->txt) ? anim_label->txt : "", -1);
		pango_layout_set_alignment (anim_label->layout, anim_label->alignment);
	}

}
