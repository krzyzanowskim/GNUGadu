#include <glib.h>
#include <glib-object.h>
#include <string.h>
#include "gui_chat_session.h"
#include "unified-types.h"
#include "support.h"
#include "plugins.h"
#include "gui_support.h"

extern GGaduPlugin *gui_handler;

/* 
 * public 
 */

GUIChatSession *gui_chat_session_new (void)
{
    return g_object_new (GUI_CHAT_SESSION_TYPE, NULL);
}

GtkWidget *gui_chat_session_get_widget (GUIChatSession * gcs)
{
    g_return_val_if_fail (GUI_CHAT_SESSION_IS_SESSION (gcs), NULL);
    return gcs->widget;
}

/* you can g_free(id) after that if you want */
void gui_chat_session_add_recipient (GUIChatSession * gcs, gchar * id)
{
    g_return_if_fail (id != NULL);
    g_return_if_fail (GUI_CHAT_SESSION_IS_SESSION (gcs));
    gcs->recipients = g_list_append (gcs->recipients, g_strdup (id));
}

GList *gui_chat_session_get_recipients_list (GUIChatSession * gcs)
{
    g_return_val_if_fail (GUI_CHAT_SESSION_IS_SESSION (gcs), NULL);
    return gcs->recipients;
}

guint gui_chat_session_get_session_type (GUIChatSession * gcs)
{
    g_return_val_if_fail (GUI_CHAT_SESSION_IS_SESSION (gcs), 0);
    g_return_val_if_fail (gcs->recipients != NULL, 0);

    if (g_list_length (gcs->recipients) > 1)
	return GGADU_CLASS_CONFERENCE;

    return GGADU_CLASS_CHAT;
}

void gui_chat_session_add_message (GUIChatSession * gcs, gchar * text, GTimeVal * send_date)
{
    gcs_history *history_entry = NULL;

    g_return_if_fail (text != NULL);
    g_return_if_fail (GUI_CHAT_SESSION_IS_SESSION (gcs));

    history_entry = g_new0 (gcs_history, sizeof (gcs_history));

    history_entry->text = g_strdup (text);
    g_get_current_time (&history_entry->date1);
    g_get_current_time (&history_entry->date2);

    if (send_date != NULL)
	memcpy (send_date, &history_entry->date2, sizeof (GTimeVal));

    gcs->history_list = g_list_append (gcs->history_list, history_entry);
}

static void gui_chat_session_create_gtk_widget (GUIChatSession * gcs)
{
    GtkWidget *widget = NULL;
    GtkWidget *history;
    GtkWidget *input;
    GtkWidget *paned;
    GtkWidget *sw;
    GtkWidget *hbox_buttons;
    GtkWidget *button_send;
    GtkWidget *button_autosend;
    GtkWidget *button_find;
    GtkWidget *button_close;
    GtkWidget *button_emoticons;
    GtkWidget *button_stick;
    GtkTextBuffer *buf = NULL;
    gchar *colorstr = NULL;
    gchar *fontstr = NULL;

    g_return_if_fail (GUI_CHAT_SESSION_IS_SESSION (gcs));

    widget = gtk_vbox_new (FALSE, 0);	/* main container */

    /*
     * history 
     */
    history = gtk_text_view_new ();
    gtk_widget_set_name (GTK_WIDGET (history), "GGHistory");

    gtk_text_view_set_editable (GTK_TEXT_VIEW (history), FALSE);
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (history), GTK_WRAP_CHAR);
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (history), FALSE);
    gtk_text_view_set_left_margin (GTK_TEXT_VIEW (history), 2);

    gtk_widget_ref (history);
    g_object_set_data_full (G_OBJECT ( widget ), "history", history, (GDestroyNotify) gtk_widget_unref);

    buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (history));

    colorstr = config_var_get (gui_handler, "msg_header_color");
    fontstr = config_var_get (gui_handler, "msg_header_font");

    gtk_text_buffer_create_tag (buf, "incoming_header", "foreground",
				(colorstr &&
				 (strlen (colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR, "font",
				(fontstr) ? fontstr : DEFAULT_FONT, NULL);

    colorstr = config_var_get (gui_handler, "msg_body_color");
    fontstr = config_var_get (gui_handler, "msg_body_font");

    gtk_text_buffer_create_tag (buf, "incoming_text", "foreground",
				(colorstr &&
				 (strlen (colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR, "font",
				(fontstr) ? fontstr : DEFAULT_FONT, NULL);

    colorstr = config_var_get (gui_handler, "msg_out_header_color");
    fontstr = config_var_get (gui_handler, "msg_out_header_font");

    gtk_text_buffer_create_tag (buf, "outgoing_header", "foreground",
				(colorstr &&
				 (strlen (colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR, "font",
				(fontstr) ? fontstr : DEFAULT_FONT, NULL);

    colorstr = config_var_get (gui_handler, "msg_out_body_color");
    fontstr = config_var_get (gui_handler, "msg_out_body_font");

    gtk_text_buffer_create_tag (buf, "outgoing_text", "foreground",
				(colorstr &&
				 (strlen (colorstr) > 0)) ? colorstr : DEFAULT_TEXT_COLOR, "font",
				(fontstr) ? fontstr : DEFAULT_FONT, NULL);

    /*
     * input 
     */
    input = gtk_text_view_new ();
    gtk_widget_set_name (GTK_WIDGET (input), "GGInput");
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (input), GTK_WRAP_WORD);
    gtk_text_view_set_left_margin (GTK_TEXT_VIEW (input), 2);

    gtk_widget_ref (input);
    g_object_set_data_full (G_OBJECT ( widget ), "input", input, (GDestroyNotify) gtk_widget_unref);

    /*
     * paned 
     */
    paned = gtk_vpaned_new ();

    /*
     * SW1 
     */
    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

    gtk_container_add (GTK_CONTAINER (sw), history);
    gtk_paned_add1 (GTK_PANED (paned), sw);

    /*
     * SW2 
     */
    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add (GTK_CONTAINER (sw), input);
    gtk_paned_add2 (GTK_PANED (paned), sw);

    /* attach paned to widget */
    gtk_box_pack_start (GTK_BOX (widget), paned, TRUE, TRUE, 0);
    
    gcs->widget = widget;
}

/* 
 * private 
 */

static GObject *gui_chat_session_constructor (GType type, guint n_construct_properties,
					      GObjectConstructParam * construct_properties)
{
    GObject *obj;

    {
	/* Invoke parent constructor. */
	GUIChatSessionClass *klass;
	GObjectClass *parent_class;
	klass = GUI_CHAT_SESSION_CLASS (g_type_class_peek (GUI_CHAT_SESSION_TYPE));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
    }

    /* do stuff. */


    return obj;
}

static void gui_chat_session_instance_init (GTypeInstance * instance, gpointer g_class)
{
    GUIChatSession *self = (GUIChatSession *) instance;
    /* do stuff */
}

static void gui_chat_session_class_init (gpointer g_class, gpointer g_class_data)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
    GUIChatSession *klass = GUI_CHAT_SESSION_CLASS (g_class);
    gobject_class->constructor = gui_chat_session_constructor;
}

GType gui_chat_session_get_type (void)
{
    static GType type = 0;

    if (type == 0)
      {
	  static const GTypeInfo info = {
	      sizeof (GUIChatSessionClass),
	      NULL,		/* base_init */
	      NULL,		/* base_finalize */
	      gui_chat_session_class_init,	/* class_init */
	      NULL,		/* class_finalize */
	      NULL,		/* class_data */
	      sizeof (GUIChatSession),
	      0,		/* n_preallocs */
	      gui_chat_session_instance_init	/* instance_init */
	  };

	  type = g_type_register_static (G_TYPE_OBJECT, "GUIChatSessionType", &info, 0);
      }

    return type;
}
