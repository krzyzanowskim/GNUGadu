/* $Id: gui_dialogs.c,v 1.30 2004/01/28 18:32:00 thrulliq Exp $ */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>

#include "unified-types.h"
#include "dialog.h"
#include "signals.h"
#include "support.h"
#include "gui_support.h"
#include "gui_main.h"

extern GtkWidget *window;
extern GSList *protocols;

static gint about_y;
static gint about_area_y;
static gint about_area_x;
static gint about_timeout;
static GdkPixmap *pixmap = NULL;
static GdkGC *gc;

static gchar *about_text = NULL;

void gui_add_user_window_response(GtkDialog * dialog, int resid, gpointer user_data)
{
	GGaduSignal *signal = (GGaduSignal *) user_data;
	GSList *kvlist = (GSList *) signal->data;

	if (resid == GTK_RESPONSE_OK)
	{
		while (kvlist)
		{
			GGaduKeyValue *kv = (GGaduKeyValue *) kvlist->data;

			if ((kv != NULL) && (kv->user_data != NULL))
				kv->value = (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(kv->user_data)));

			kvlist = kvlist->next;
		}

		signal_emit("main-gui", "add user", signal->data, signal->source_plugin_name);
	}

	GGaduSignal_free(signal);
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

void gui_change_user_window_response(GtkDialog * dialog, int resid, gpointer user_data)
{
	GGaduSignal *signal = (GGaduSignal *) user_data;
	GSList *kvlist = (GSList *) signal->data;

	if (resid == GTK_RESPONSE_OK)
	{

		while (kvlist)
		{
			GGaduKeyValue *kv = (GGaduKeyValue *) kvlist->data;

			kv->value = (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(kv->user_data)));
			/* gtk_widget_unref(GTK_WIDGET(kv->user_data)); */

			kvlist = kvlist->next;
		}

		signal_emit("main-gui", "change user", signal->data, signal->source_plugin_name);
	}

	GGaduSignal_free(signal);
	gtk_widget_destroy(GTK_WIDGET(dialog));

}

void gui_dialog_show_filename(GtkWidget * txt_entry)
{
	GtkWidget *file_selector = NULL;
	GGaduKeyValue *kv = (GGaduKeyValue *) g_object_get_data(G_OBJECT(txt_entry), "kv");
	gchar *filename = NULL;
	gint response;

	file_selector = gtk_file_selection_new(_("Select file"));

	response = gtk_dialog_run(GTK_DIALOG(file_selector));

	if (response == GTK_RESPONSE_OK)
	{
		filename = (gchar *) gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selector));
		gtk_entry_set_text(GTK_ENTRY(txt_entry), filename);
		kv->value = (gpointer) filename;
	}

	gtk_widget_destroy(file_selector);
}

/* Does not work as expected yet. Need to know how to make it
   return XLFD. */
void gui_dialog_show_fontchooser(GtkWidget * txt_entry)
{
	GtkWidget *font_selector = NULL;
	GGaduKeyValue *kv = (GGaduKeyValue *) g_object_get_data(G_OBJECT(txt_entry), "kv");
	gchar *font_name = NULL;
	gint response;

	font_selector = gtk_font_selection_dialog_new(_("Select font"));

	response = gtk_dialog_run(GTK_DIALOG(font_selector));

	if (response == GTK_RESPONSE_OK)
	{
		font_name =
			(gchar *)
			gtk_font_selection_get_font_name(GTK_FONT_SELECTION
							 (GTK_FONT_SELECTION_DIALOG(font_selector)->fontsel));
		gtk_entry_set_text(GTK_ENTRY(txt_entry), font_name);
		kv->value = (gpointer) font_name;
	}

	gtk_widget_destroy(font_selector);
}

void gui_dialog_show_colorchooser(GtkWidget * txt_entry)
{
	GtkWidget *color_selector = NULL;
	GGaduKeyValue *kv = (GGaduKeyValue *) g_object_get_data(G_OBJECT(txt_entry), "kv");
	GdkColor color;
	gchar *color_txt = NULL;
	gint response;

	color_selector = gtk_color_selection_dialog_new(_("Select color"));

	color_txt = (gchar *) gtk_entry_get_text(GTK_ENTRY(txt_entry));
	gdk_color_parse(color_txt, &color);
	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(color_selector)->colorsel),
					      &color);

	response = gtk_dialog_run(GTK_DIALOG(color_selector));

	if (response == GTK_RESPONSE_OK)
	{
		gtk_color_selection_get_current_color(GTK_COLOR_SELECTION
						      (GTK_COLOR_SELECTION_DIALOG(color_selector)->colorsel), &color);
		color_txt = (gchar *) gtk_color_selection_palette_to_string(&color, 1);
		gtk_entry_set_text(GTK_ENTRY(txt_entry), color_txt);
		kv->value = (gpointer) color_txt;
	}

	gtk_widget_destroy(color_selector);
}

GtkWidget *gui_build_dialog_gtk_table(GSList * list, gint cols)
{
	GSList *listtmp = list;
	gint ielements = g_slist_position(list, g_slist_last(list));
	gint rows = ((ielements + 1) / cols);
	GtkWidget *tab = gtk_table_new(rows, cols, FALSE);
	gint actC = 0, actR = 0;
	GtkWidget *to_grab_focus = NULL;

	gtk_container_set_border_width(GTK_CONTAINER(tab), 15);

	while (listtmp)
	{
		GGaduKeyValue *kv = (GGaduKeyValue *) listtmp->data;
		GtkWidget *entry = NULL;
		gboolean need_label = TRUE;

		switch (kv->type)
		{
		case VAR_STR:
			entry = gtk_entry_new();
			if (kv->value)
				gtk_entry_set_text(GTK_ENTRY(entry), kv->value);

			gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
			break;
		case VAR_INT:
		{
			entry = gtk_spin_button_new_with_range(0, 999999999, 1);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry), (gint) kv->value);
			gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
		}
			break;
		case VAR_INT_WITH_NEGATIVE:
		{
			entry = gtk_spin_button_new_with_range(-999999999, 999999999, 1);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry), (gint) kv->value);
			gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
		}
			break;
		case VAR_BOOL:
			need_label = FALSE;
			entry = gtk_check_button_new_with_label(kv->description);
			if (kv->value)
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry), TRUE);
			break;
		case VAR_FILE_CHOOSER:
		{
			GtkWidget *txt_entry = NULL;
			GtkWidget *button_entry = NULL;

			entry = gtk_hbox_new(FALSE, 2);

			txt_entry = gtk_entry_new();
			if (kv->value)
				gtk_entry_set_text(GTK_ENTRY(txt_entry), kv->value);

			g_object_set_data(G_OBJECT(txt_entry), "kv", kv);
			g_object_set_data(G_OBJECT(entry), "txt_entry", txt_entry);


			button_entry = gtk_button_new_from_stock("gtk-open");

			gtk_box_pack_start_defaults(GTK_BOX(entry), txt_entry);
			gtk_box_pack_start_defaults(GTK_BOX(entry), button_entry);

			g_signal_connect_swapped(button_entry, "clicked", G_CALLBACK(gui_dialog_show_filename),
						 txt_entry);
		}
			break;
		case VAR_FONT_CHOOSER:
		{
			GtkWidget *txt_entry = NULL;
			GtkWidget *button_entry = NULL;

			entry = gtk_hbox_new(FALSE, 2);

			txt_entry = gtk_entry_new();
			if (kv->value)
				gtk_entry_set_text(GTK_ENTRY(txt_entry), kv->value);

			g_object_set_data(G_OBJECT(txt_entry), "kv", kv);
			g_object_set_data(G_OBJECT(entry), "txt_entry", txt_entry);

			button_entry = gtk_button_new_from_stock("gtk-select-font");

			gtk_box_pack_start_defaults(GTK_BOX(entry), txt_entry);
			gtk_box_pack_start_defaults(GTK_BOX(entry), button_entry);

			g_signal_connect_swapped(button_entry, "clicked", G_CALLBACK(gui_dialog_show_fontchooser),
						 txt_entry);
		}
			break;
		case VAR_COLOUR_CHOOSER:
		{
			GtkWidget *txt_entry = NULL;
			GtkWidget *button_entry = NULL;

			entry = gtk_hbox_new(FALSE, 2);

			txt_entry = gtk_entry_new();
			if (kv->value)
				gtk_entry_set_text(GTK_ENTRY(txt_entry), kv->value);

			g_object_set_data(G_OBJECT(txt_entry), "kv", kv);
			g_object_set_data(G_OBJECT(entry), "txt_entry", txt_entry);

			button_entry = gtk_button_new_from_stock("gtk-select-color");

			gtk_box_pack_start_defaults(GTK_BOX(entry), txt_entry);
			gtk_box_pack_start_defaults(GTK_BOX(entry), button_entry);

			g_signal_connect_swapped(button_entry, "clicked", G_CALLBACK(gui_dialog_show_colorchooser),
						 txt_entry);
		}
			break;
		case VAR_IMG:
			need_label = FALSE;
			entry = gtk_image_new_from_file(kv->value);
			gtk_table_set_homogeneous(GTK_TABLE(tab), FALSE);
			break;
		case VAR_LIST:
			entry = gtk_combo_new();
			gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(entry)->entry), FALSE);
			gtk_combo_set_popdown_strings(GTK_COMBO(entry), kv->value);
			break;
		}

		if ((kv->flag & VAR_FLAG_SENSITIVE) != 0)
			gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);

		if ((kv->flag & VAR_FLAG_INSENSITIVE) != 0)
			gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);

		if ((kv->flag & VAR_FLAG_PASSWORD) != 0)
			gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);

		if ((kv->flag & VAR_FLAG_FOCUS) != 0)
			to_grab_focus = entry;

		kv->user_data = (gpointer) entry;

		if (need_label)
		{
			GtkWidget *vbox = gtk_alignment_new(0, 0.5, 0, 0);
			GtkWidget *label = gtk_label_new(kv->description);

			gtk_container_add(GTK_CONTAINER(vbox), label);
			gtk_table_attach_defaults(GTK_TABLE(tab), vbox, 0, 1, actR, actR + 1);

			if (entry)
				gtk_table_attach_defaults(GTK_TABLE(tab), entry, 1, 2, actR, actR + 1);

		}
		else
		{
			gtk_table_attach(GTK_TABLE(tab), entry, actC, actC + 2, actR, actR + 1, GTK_FILL, GTK_SHRINK, 0,
					 0);
		}

		if ((actC + 1) < cols)
			actC++;
		else
		{
			actC = 0;
			actR++;
		}

		listtmp = listtmp->next;
	}

	if (to_grab_focus)
		gtk_widget_grab_focus(GTK_WIDGET(to_grab_focus));

	return tab;
}

void gui_user_data_window(gpointer signal, gboolean change)
{
	GGaduSignal *sig = (GGaduSignal *) signal;
	GtkWidget *adduserwindow = NULL;
	GtkWidget *table;

	adduserwindow =
		gtk_dialog_new_with_buttons((change) ? _("Change User") : _("Add User"), NULL,
					    GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					    GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	gtk_window_set_resizable(GTK_WINDOW(adduserwindow), FALSE);

	table = gui_build_dialog_gtk_table((GSList *) sig->data, 1);

	gtk_table_set_row_spacings(GTK_TABLE(table), 7);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(adduserwindow)->vbox), table);

	g_signal_connect(G_OBJECT(adduserwindow), "response",
			 (change) ? G_CALLBACK(gui_change_user_window_response) :
			 G_CALLBACK(gui_add_user_window_response), signal_cpy(signal));

	gtk_widget_show_all(adduserwindow);
}

void gui_dialog_response(GtkDialog * dialog, int resid, gpointer user_data)
{
	GGaduSignal *signal = (GGaduSignal *) user_data;
	GGaduDialog *d = signal->data;
	GSList *kvlist = d->optlist;


	while (kvlist)
	{
		GGaduKeyValue *kv = (GGaduKeyValue *) kvlist->data;

		switch (kv->type)
		{
		case VAR_STR:
		{
			gchar *tmp = (gchar *) g_strdup(gtk_entry_get_text(GTK_ENTRY(kv->user_data)));
			if (strlen(tmp) > 0)
				kv->value = (gpointer) tmp;
			else
			{
				kv->value = NULL;
				g_free(tmp);
			}
		}
			break;
		case VAR_INT:
		case VAR_INT_WITH_NEGATIVE:
			kv->value = (gpointer) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(kv->user_data));
			break;
		case VAR_BOOL:
			kv->value = (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(kv->user_data));
			break;
		case VAR_FILE_CHOOSER:
		case VAR_FONT_CHOOSER:
		case VAR_COLOUR_CHOOSER:
		{
			gchar *tmp = NULL;
			GtkWidget *hbox = (GtkWidget *) kv->user_data;
			GtkWidget *entry = (GtkWidget *) g_object_get_data(G_OBJECT(hbox), "txt_entry");

			tmp = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
			if (strlen(tmp) > 0)
				kv->value = (gpointer) tmp;
			else
			{
				kv->value = NULL;
				g_free(tmp);
			}
		}
			break;
		case VAR_IMG:
			kv->value = NULL;
			break;
		case VAR_LIST:
			kv->value = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(kv->user_data)->entry), 0, -1);
			break;
		}
		kvlist = kvlist->next;
	}

	switch (resid)
	{
	case GTK_RESPONSE_OK:
		d->response = GGADU_OK;
		break;
	case GTK_RESPONSE_CANCEL:
		d->response = GGADU_CANCEL;
		break;
	case GTK_RESPONSE_YES:
		d->response = GGADU_YES;
		break;
	case GTK_RESPONSE_NO:
		d->response = GGADU_NO;
		break;
	default:
		d->response = GGADU_NONE;
		break;
	}

	signal_emit("main-gui", d->callback_signal, d, signal->source_plugin_name);

	GGaduSignal_free(signal);
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

void gui_show_message_box(gint type, gpointer signal)
{
	GtkWidget *warning = NULL;
	gchar *txt = ((GGaduSignal *) signal)->data;
	gchar *title;
	gui_protocol *gp;

	warning =
		gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, type,
				       GTK_BUTTONS_OK, txt);

	gp = gui_find_protocol(((GGaduSignal *) signal)->source_plugin_name, protocols);

	/* *INDENT-OFF* */
    title =	g_strdup_printf("%s: %s", (gp) ? gp->p->display_name : (char *) ((GGaduSignal *) signal)->source_plugin_name,
			gtk_window_get_title(GTK_WINDOW(warning)));
	/* *INDENT-ON* */

	gtk_window_set_title(GTK_WINDOW(warning), title);
	gtk_widget_show_all(warning);
	g_signal_connect_swapped(GTK_OBJECT(warning), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(warning));
	g_free(txt); /* ZONK there shouldnt be free(txt) imho */
	g_free(title);
}

void gui_show_window_with_text(gpointer signal)
{
	GGaduSignal *sig = (GGaduSignal *) signal;
	GtkWidget *dialog = NULL;
	GtkWidget *gtv = NULL;
	GtkTextBuffer *buf = NULL;
	GtkWidget *sw = NULL;

	dialog = gtk_dialog_new_with_buttons("", NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK,
					     NULL);
	gtk_window_resize(GTK_WINDOW(dialog), 400, 400);

	buf = gtk_text_buffer_new(NULL);
	gtk_text_buffer_set_text(buf, (gchar *) sig->data, -1);
	gtv = gtk_text_view_new_with_buffer(buf);

	gtk_text_view_set_editable(GTK_TEXT_VIEW(gtv), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(gtv), FALSE);

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), sw);
	gtk_container_add(GTK_CONTAINER(sw), gtv);

	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), dialog);
	gtk_widget_show_all(dialog);

}

void gui_show_dialog(gpointer signal, gboolean change)
{
	GGaduSignal *sig = (GGaduSignal *) signal;
	GtkWidget *dialog = NULL;
	GtkWidget *image = NULL;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *hbox;
	GdkPixbuf *windowicon = NULL;
	GGaduDialog *d = (sig) ? sig->data : NULL;
	gchar *markup;

	if (!sig)
		return;

	dialog = gtk_dialog_new_with_buttons(d->title, NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

	if ((windowicon = create_pixbuf(GGADU_DEFAULT_ICON_FILENAME)) != NULL)
	{
		gtk_window_set_icon(GTK_WINDOW(dialog), windowicon);
		gdk_pixbuf_unref(windowicon);
	}

	hbox = gtk_hbox_new(FALSE, 0);

	if (d->type)
	{
		print_debug("d->type = %d\n", d->type);
		switch (d->type)
		{
		case GGADU_DIALOG_CONFIG:
			image = gtk_image_new();
			gtk_image_set_from_stock(GTK_IMAGE(image), "gtk-preferences", GTK_ICON_SIZE_DND);
			break;
		case GGADU_DIALOG_YES_NO:
			image = gtk_image_new();
			gtk_image_set_from_stock(GTK_IMAGE(image), "gtk-dialog-question", GTK_ICON_SIZE_DND);
			break;
		default:
			break;
		}
		gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 5);
	}

	label = gtk_label_new(NULL);
	markup = g_strdup_printf("<span weight=\"bold\">%s</span>", d->title);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);

	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 10);

	table = gui_build_dialog_gtk_table(d->optlist, 1);

	gtk_table_set_row_spacings(GTK_TABLE(table), 7);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gui_dialog_response), signal_cpy(signal));

	gtk_widget_show_all(dialog);
}

static gint timeout(gpointer user_data)
{
	GtkWidget *image = user_data;
	PangoLayout *layout;
	GdkRectangle area;
	gint offset_x = 0;

	if (about_timeout < 0)
		return FALSE;

	layout = gtk_widget_create_pango_layout(image, NULL);

	pango_layout_set_markup(layout, about_text, -1);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
	pango_layout_set_justify(layout, TRUE);

	if (about_y <= 0 && about_area_y <= 0)
	{
		about_y = image->allocation.width - 80;
		pango_layout_get_pixel_size(layout, &about_area_x, &about_area_y);
		if (about_area_x > image->allocation.width)
		{
			GdkPixmap *old = pixmap;
			gtk_widget_set_size_request(image, about_area_x, 200);

			pixmap = gdk_pixmap_new(image->window, about_area_x, image->allocation.height, -1);
			gdk_pixmap_unref(old);
		}
	}

	if (image->allocation.width > about_area_x)
		offset_x = (image->allocation.width - about_area_x) / 2;

	if (about_y <= 0)
		about_area_y--;

	about_y--;

	area.x = 0;
	area.y = 0;
	area.width = image->allocation.width;
	area.height = image->allocation.height;

	gdk_draw_rectangle(pixmap, gc, TRUE, 0, 0, image->allocation.width, image->allocation.height);

	gtk_paint_layout(image->style, pixmap, GTK_WIDGET_STATE(image), FALSE, &area, image, "about",
			 image->allocation.x + offset_x, about_y, layout);

	gdk_draw_drawable(image->window, gc, pixmap, 0, 0, 0, 0, image->allocation.width, image->allocation.height);

	g_object_unref(layout);

	return TRUE;
}

static void about_response(GtkWidget * widget, gpointer user_data)
{
	g_source_remove(about_timeout);
	about_timeout = -1;
	gtk_widget_destroy(widget);
	gdk_pixmap_unref(pixmap);
	pixmap = NULL;
	g_free(about_text);
}

static gboolean about_configure_event(GtkWidget * image, GdkEventConfigure * event, gpointer data)
{
	if (pixmap)
		return TRUE;

	if (!gc)
	{
		GdkColor color;
		gdk_color_parse("#ffffff", &color);
		gc = gdk_gc_new(image->window);
		gdk_gc_set_rgb_fg_color(gc, &color);
	}

	pixmap = gdk_pixmap_new(image->window, image->allocation.width, image->allocation.height, -1);

	gdk_draw_rectangle(pixmap, gc, TRUE, 0, 0, image->allocation.width, image->allocation.height);
	return TRUE;
}

static gboolean about_expose_event(GtkWidget * image, GdkEventExpose * event, gpointer data)
{
	gdk_draw_drawable(image->window, gc, pixmap, event->area.x, event->area.y, event->area.x, event->area.y,
			  event->area.width, event->area.height);
	return TRUE;
}

void gui_about(GtkWidget * widget, gpointer data)
{
	GtkWidget *about;
	GtkWidget *table;
	GtkWidget *image;
	GdkPixbuf *windowicon = NULL;

	print_debug("About\n");

	about = gtk_dialog_new_with_buttons(_("About"), NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					    GTK_STOCK_OK, GTK_RESPONSE_NONE, NULL);
	gtk_window_set_resizable(GTK_WINDOW(about), FALSE);
	table = gtk_table_new(2, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(about)->vbox), table);

	if ((windowicon = create_pixbuf(GGADU_DEFAULT_ICON_FILENAME)) != NULL)
	{
		gtk_window_set_icon(GTK_WINDOW(about), windowicon);
		gdk_pixbuf_unref(windowicon);
	}

	image = create_image("gg-about.png");
	gtk_table_attach_defaults(GTK_TABLE(table), image, 0, 1, 0, 1);

	image = gtk_drawing_area_new();
	gtk_widget_set_size_request(image, 200, 200);
	g_signal_connect(G_OBJECT(image), "configure_event", G_CALLBACK(about_configure_event), NULL);
	g_signal_connect(G_OBJECT(image), "expose_event", G_CALLBACK(about_expose_event), NULL);
	gtk_table_attach_defaults(GTK_TABLE(table), image, 0, 1, 1, 2);

	g_signal_connect(GTK_OBJECT(about), "response", G_CALLBACK(about_response), NULL);

	gtk_window_set_default_size(GTK_WINDOW(about), 200, 200);

	gtk_widget_show_all(about);

	about_y = 0;
	about_area_y = 0;
	about_text =
		g_strdup_printf(_
				("<b>GNU Gadu %s</b>\n" "Copyright (C) 2001-2003 GNU Gadu Team\n" "License: GPL\n"
				 "Homepage: http://www.gadu.gnu.pl/\n\n" "<b>Main Programmers:</b>\n"
				 "Igor Popik &lt;thrull@slackware.pl&gt;\n"
				 "Marcin Krzyzanowski &lt;krzak@hakore.com&gt;\n"
				 "Jakub 'shasta' Jankowski &lt;shasta@atn.pl&gt;\n\n" "<b>Also:</b>\n"
				 "Bartosz Zapalowski\n" "Mateusz Papiernik\n" "HelDoRe\n" 
				 "Pawel Jan Maczewski\n\n" "<b>Thanks to:</b>\n" "Aflinta\n" "GammaRay\n" "Plavi\n"
				 "Dwuziu\n" "see AUTHORS file for details\n\n" "<i>Compile time:\n%s %s</i>"), VERSION,
				__DATE__, __TIME__);

	about_timeout = g_timeout_add(50, timeout, image);
}

void gui_show_about(gpointer signal)
{
	gui_about(NULL, NULL);
}
