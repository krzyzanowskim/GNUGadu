/* $Id: gui_dialogs.c,v 1.10 2003/04/02 10:10:14 krzyzak Exp $ */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "unified-types.h"
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

void gui_add_user_window_response(GtkDialog *dialog, int resid, gpointer user_data) {
	GGaduSignal *signal = (GGaduSignal *)user_data;
	GSList *kvlist = (GSList *)signal->data;

	if (resid == GTK_RESPONSE_OK) {

		while (kvlist) 
		{
		GGaduKeyValue *kv = (GGaduKeyValue *)kvlist->data;

		if ((kv != NULL) && (kv->user_data != NULL))
			kv->value = (gpointer)g_strdup(gtk_entry_get_text(GTK_ENTRY(kv->user_data)));
			/* gtk_widget_unref(GTK_WIDGET(kv->user_data)); */

		kvlist = kvlist->next;
		}

	signal_emit("main-gui", "add user", signal->data, signal->source_plugin_name);
	}

	GGaduSignal_free(signal);
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

void gui_change_user_window_response(GtkDialog *dialog, int resid, gpointer user_data) {
	GGaduSignal *signal = (GGaduSignal *)user_data;
	GSList *kvlist = (GSList *)signal->data;

	if (resid == GTK_RESPONSE_OK) {

		while (kvlist) {
		GGaduKeyValue *kv = (GGaduKeyValue *)kvlist->data;

			kv->value = (gpointer)g_strdup(gtk_entry_get_text(GTK_ENTRY(kv->user_data)));
			/* gtk_widget_unref(GTK_WIDGET(kv->user_data)); */

			kvlist = kvlist->next;
		}

	signal_emit("main-gui", "change user", signal->data, signal->source_plugin_name);
	}

	GGaduSignal_free(signal);
	gtk_widget_destroy(GTK_WIDGET(dialog));

}

void gui_dialog_show_filename(GtkWidget *txt_entry)
{
    GtkWidget *file_selector = NULL;
    GGaduKeyValue *kv = (GGaduKeyValue *)g_object_get_data(G_OBJECT(txt_entry), "kv");
    gchar     *filename = NULL;
    gint       response;
    
    file_selector = gtk_file_selection_new(_("Select file"));

    response = gtk_dialog_run(GTK_DIALOG(file_selector));
    
    if (response == GTK_RESPONSE_OK) {
	filename = (gchar *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selector));
	gtk_entry_set_text( GTK_ENTRY(txt_entry), filename );
	kv->value = (gpointer)filename;
    }
    
    gtk_widget_destroy(file_selector);
}

GtkWidget *gui_build_dialog_gtk_table(GSList *list, gint cols)
{
    GSList *listtmp = list;
    gint ielements = g_slist_position(list,g_slist_last(list));
    gint rows = ((ielements + 1) / cols);
    GtkWidget *tab = gtk_table_new(rows, cols, FALSE);
    gint actC = 0, actR = 0;
    
    while (listtmp) 
    {
	GGaduKeyValue *kv = (GGaduKeyValue *)listtmp->data;
	GtkWidget *entry = NULL;
	gboolean need_label = TRUE;
		    
	switch (kv->type) {
		case VAR_STR:
			entry = gtk_entry_new();
			if (kv->value) 
			    gtk_entry_set_text( GTK_ENTRY(entry), kv->value );
			break;
		case VAR_INT: {
			    gchar *tmp;
			    entry = gtk_entry_new();
			    if (kv->value) {
				tmp = g_strdup_printf("%d", (gint)kv->value);
				gtk_entry_set_text( GTK_ENTRY(entry), tmp);
				g_free(tmp);
			    }
			}
			break;
		case VAR_BOOL:
			need_label = FALSE;
			entry = gtk_check_button_new_with_label(kv->description);
			if (kv->value)
			    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry), TRUE);
			break;
		case VAR_FILE_CHOOSER: {
			GtkWidget *txt_entry = NULL;
			GtkWidget *button_entry = NULL;

			entry = gtk_hbox_new(FALSE,2);

			txt_entry = gtk_entry_new();
			if (kv->value) 
			    gtk_entry_set_text( GTK_ENTRY(txt_entry), kv->value );

			g_object_set_data(G_OBJECT(txt_entry), "kv", kv);
			g_object_set_data(G_OBJECT(entry), "txt_entry", txt_entry);


			button_entry = gtk_button_new_from_stock("gtk-open");
			
			gtk_box_pack_start_defaults(GTK_BOX(entry),txt_entry);
			gtk_box_pack_start_defaults(GTK_BOX(entry),button_entry);

			g_signal_connect_swapped(button_entry, "clicked", G_CALLBACK(gui_dialog_show_filename), txt_entry);
			}
			break;
		case VAR_IMG:
			need_label = FALSE;
			entry = gtk_image_new_from_file(kv->value);
			gtk_table_set_homogeneous(GTK_TABLE(tab), FALSE);
			break;
		case VAR_LIST:
			entry = gtk_combo_new();
			gtk_combo_set_popdown_strings(GTK_COMBO(entry), kv->value);
			break;
	}

    	if ((kv->flag & VAR_FLAG_SENSITIVE) != 0)
		gtk_editable_set_editable( GTK_EDITABLE(entry), TRUE);
		
		if ((kv->flag & VAR_FLAG_INSENSITIVE) != 0)
			gtk_editable_set_editable( GTK_EDITABLE(entry), FALSE);

    	if ((kv->flag & VAR_FLAG_PASSWORD) != 0)
		gtk_entry_set_visibility( GTK_ENTRY(entry), FALSE);
	
	kv->user_data = (gpointer)entry;	
        
	if (need_label) {
	    GtkWidget *vbox = gtk_alignment_new(0,0,0,0);
	    GtkWidget *label = gtk_label_new(kv->description);
	    
	    gtk_container_add(GTK_CONTAINER(vbox), label);
	    gtk_table_attach_defaults(GTK_TABLE(tab), vbox, 0, 1, actR, actR+1);
	    gtk_table_attach_defaults(GTK_TABLE(tab), entry, 1, 2, actR, actR+1);
	} else {
	    gtk_table_attach(GTK_TABLE(tab), entry, actC, actC+2, actR, actR+1, GTK_FILL, GTK_SHRINK, 0, 0);
	}
		
	if ((actC + 1) < cols) actC++; else  { actC = 0; actR++; }

	listtmp = listtmp->next;
    }

    return tab;
}

void gui_user_data_window(gpointer signal, gboolean change) 
{
    GGaduSignal *sig = (GGaduSignal *)signal;
    GtkWidget *adduserwindow = NULL;
    GtkWidget *table;
    adduserwindow = gtk_dialog_new_with_buttons((change) ? _("Change User") : _("Add User"),GTK_WINDOW(window),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
						GTK_STOCK_OK,GTK_RESPONSE_OK,NULL);
    gtk_window_set_resizable(GTK_WINDOW(adduserwindow), FALSE);
    table = gui_build_dialog_gtk_table((GSList *)sig->data, 1);
    
    gtk_table_set_row_spacings(GTK_TABLE(table),5);
    gtk_table_set_col_spacings(GTK_TABLE(table),3);

    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(adduserwindow)->vbox), table);

    g_signal_connect(G_OBJECT(adduserwindow), "response", 
		    (change) ? G_CALLBACK(gui_change_user_window_response) : G_CALLBACK(gui_add_user_window_response), 
		    signal_cpy(signal));

    gtk_widget_show_all(adduserwindow);
}

void gui_dialog_response(GtkDialog *dialog, int resid, gpointer user_data) {
    GGaduSignal *signal = (GGaduSignal *)user_data;
    GGaduDialog *d = signal->data;
    GSList *kvlist = d->optlist;

    if (resid == GTK_RESPONSE_OK) {

	while (kvlist) 
	{
	    GGaduKeyValue *kv = (GGaduKeyValue *)kvlist->data;
	    
	    
	    switch (kv->type) {
		case VAR_STR: {
			    gchar *tmp = (gchar *)g_strdup(gtk_entry_get_text(GTK_ENTRY(kv->user_data)));
			    if (strlen(tmp) > 0)
	    			kv->value = (gpointer)tmp;
			    else {
				kv->value = NULL;
				g_free(tmp);
			    }
			}
			break;
		case VAR_INT:
			kv->value = (gpointer)atoi(gtk_entry_get_text(GTK_ENTRY(kv->user_data)));
			break;
		case VAR_BOOL:
			kv->value = (gpointer)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(kv->user_data));
			break;
		case VAR_FILE_CHOOSER: {
			gchar *tmp = NULL;
			GtkWidget *hbox = (GtkWidget *)kv->user_data;
			GtkWidget *entry = (GtkWidget *)g_object_get_data(G_OBJECT(hbox),"txt_entry");
			
			tmp = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
			if (strlen(tmp) > 0)
	    		    kv->value = (gpointer)tmp;
			else {
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
	
	signal_emit("main-gui", d->callback_signal, d, signal->source_plugin_name);
    } 
	else if (resid == GTK_RESPONSE_CANCEL) {
			GGaduDialog_free(d);	
    }

    GGaduSignal_free(signal);
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

void gui_show_message_box(gint type, gpointer signal)
{
    GtkWidget 	*warning = NULL;
    gchar	*txt = ((GGaduSignal *)signal)->data;
    gchar 	*title;
    gui_protocol *gp;
	
    warning = gtk_message_dialog_new(GTK_WINDOW(window),
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					type,
					GTK_BUTTONS_OK,
					txt);
	
    gp = gui_find_protocol(((GGaduSignal *)signal)->source_plugin_name, protocols);
	
    title = g_strdup_printf("%s: %s", 
    	(gp) ? gp->p->display_name : (char *)((GGaduSignal *)signal)->source_plugin_name, 
    
    gtk_window_get_title (GTK_WINDOW(warning)));
	
    gtk_window_set_title(GTK_WINDOW(warning), title);
    gtk_widget_show_all (warning);
    g_signal_connect_swapped(GTK_OBJECT(warning), 
				"response", 
				G_CALLBACK (gtk_widget_destroy),
				GTK_OBJECT(warning));
    g_free(txt);
    g_free(title);
}

void gui_show_window_with_text(gpointer signal) 
{
    GGaduSignal *sig = (GGaduSignal *)signal;
    GtkWidget *dialog = NULL;
    GtkWidget *gtv = NULL;
    GtkTextBuffer *buf = NULL;
    GtkWidget *sw = NULL;
    
    dialog = gtk_dialog_new_with_buttons("",GTK_WINDOW(window),GTK_DIALOG_DESTROY_WITH_PARENT,GTK_STOCK_OK,GTK_RESPONSE_OK,NULL);
    gtk_window_resize(GTK_WINDOW(dialog),400,400);
    
    buf = gtk_text_buffer_new(NULL);
    gtk_text_buffer_set_text(buf,(gchar *)sig->data,-1);
    gtv = gtk_text_view_new_with_buffer(buf);

    gtk_text_view_set_editable(GTK_TEXT_VIEW(gtv),FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(gtv),FALSE);

    sw = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), sw);
    gtk_container_add(GTK_CONTAINER(sw), gtv);

    gtk_widget_show_all(dialog);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
}

void gui_show_dialog(gpointer signal, gboolean change) 
{
    GGaduSignal *sig = (GGaduSignal *)signal;
    GtkWidget *dialog = NULL;
    GtkWidget *image = NULL;
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *hbox;
    GGaduDialog *d = sig->data;
    gchar *markup;
    
    dialog = gtk_dialog_new_with_buttons(d->title,GTK_WINDOW(window),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK,GTK_RESPONSE_OK,NULL);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    
    hbox = gtk_hbox_new(FALSE, 0);
    
    if (d->type) {
	print_debug("d->type = %d\n", d->type);
	switch (d->type) {
	    case GGADU_DIALOG_CONFIG:
		image = create_image("preferences.png");
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
    
    gtk_table_set_row_spacings(GTK_TABLE(table),5);
    gtk_table_set_col_spacings(GTK_TABLE(table),3);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);

    g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gui_dialog_response), signal_cpy(signal));

    gtk_widget_show_all(dialog);
}

static
gint timeout(gpointer user_data)
{
    GtkWidget *image = user_data;
    PangoLayout *layout;
    GdkRectangle area;
    gint offset_x = 0;
        
    if (about_timeout < 0)
	return FALSE;

    layout = gtk_widget_create_pango_layout(image, NULL);
 
    pango_layout_set_markup(layout, about_text, -1);
/*
    pango_layout_set_markup(layout, _(
    "<b>GNU Gadu " VERSION "</b>\n"
    "Copyright (C) 2001-2003 GNU Gadu Team\n"
    "License" ": GPL\n"
    "Homepage" ": http://www.gadu.gnu.pl\n\n"

    "<b>Main Programmers:</b>\n"
    "Igor Popik &lt;thrull@slackware.pl&gt;\n"
    "Marcin Krzyzanowski &lt;krzak@hakore.com&gt;\n\n"

    "<b>Patches:</b>\n"
    "Mateusz Papiernik\n"
    "Pawel Jan Maczewski\n"
    "HelDoRe\n"
    "Bartosz Zapalowski\n"
    "Jakub 'shasta' Jankowski\n\n"
    
    "<b>Thanks to:</b>\n"
    "Dwuziu\nAflinta\nGammaRay\nPlavi\n"
    "see AUTHORS file for details\n\n"
    
    "<i>Compile time:\n" __DATE__ " " __TIME__ "</i>"
    ), -1);
*/
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    pango_layout_set_justify(layout, TRUE);

    if (about_y <= 0 && about_area_y <= 0) {
	about_y = image->allocation.width - 80;
	pango_layout_get_pixel_size(layout, &about_area_x, &about_area_y);
	if (about_area_x > image->allocation.width) {
	    GdkPixmap *old = pixmap;
	    gtk_widget_set_size_request(image, about_area_x, 200);
	    
	    pixmap = gdk_pixmap_new(image->window, about_area_x,
					   image->allocation.height, 
					   -1);
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
    
    gdk_draw_rectangle(pixmap,
			gc,
			TRUE,
			0, 0,
			image->allocation.width,
			image->allocation.height);
    
    gtk_paint_layout(image->style, 
		     pixmap, 
		     GTK_WIDGET_STATE(image), 
		     FALSE, 
		     &area, 
		     image, 
		     "about", 
		     image->allocation.x + offset_x, about_y, 
		     layout);

    gdk_draw_drawable(image->window,
			gc,
			pixmap,
			0,0,
			0,0,
			image->allocation.width, image->allocation.height);

    g_object_unref(layout);

    return TRUE;
}

static
void about_response(GtkWidget *widget, gpointer user_data)
{
    g_source_remove(about_timeout);
    about_timeout = -1;
    gtk_widget_destroy(widget);
    gdk_pixmap_unref(pixmap);
    pixmap = NULL;
    g_free(about_text);
}

static
gboolean about_configure_event(GtkWidget *image, GdkEventConfigure *event, gpointer data)
{
    if (pixmap)
	return TRUE;
    
    if (!gc) {
	GdkColor color;
	gdk_color_parse("#ffffff", &color);
	gc = gdk_gc_new(image->window);
	gdk_gc_set_rgb_fg_color(gc, &color);
    }
    
    pixmap = gdk_pixmap_new(image->window, image->allocation.width,
					   image->allocation.height, 
					   -1);
    
    gdk_draw_rectangle(pixmap,
			gc,
			TRUE,
			0, 0,
			image->allocation.width,
			image->allocation.height);
    return TRUE;
}

static
gboolean about_expose_event(GtkWidget *image, GdkEventExpose *event, gpointer data)
{
    gdk_draw_drawable(image->window,
			gc,
			pixmap,
			event->area.x, event->area.y,
			event->area.x, event->area.y,
			event->area.width, event->area.height);
    return TRUE;
}

void gui_about(GtkWidget *widget, gpointer data) 
{
    GtkWidget *about;
    GtkWidget *table;
    GtkWidget *image;
            
    print_debug("About\n");
    
    about = gtk_dialog_new_with_buttons(_("About"), 
					GTK_WINDOW(window),
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_OK,
					GTK_RESPONSE_NONE,
    					NULL);
    gtk_window_set_resizable(GTK_WINDOW(about), FALSE);
    table = gtk_table_new(2, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(about)->vbox), table);
    
    image = create_image("gg-about.png");
    gtk_table_attach_defaults(GTK_TABLE(table), image, 0, 1, 0, 1);

    image = gtk_drawing_area_new();
    gtk_widget_set_size_request(image, 200, 200);
    g_signal_connect(G_OBJECT(image), "configure_event", G_CALLBACK(about_configure_event), NULL);
    g_signal_connect(G_OBJECT(image), "expose_event", G_CALLBACK(about_expose_event), NULL);
    gtk_table_attach_defaults(GTK_TABLE(table), image, 0, 1, 1, 2);
    
    g_signal_connect(GTK_OBJECT(about), 
				"response", 
				G_CALLBACK (about_response),
				NULL);

    gtk_window_set_default_size(GTK_WINDOW(about), 200, 200);

    gtk_widget_show_all (about);

    about_y = 0;
    about_area_y = 0;
    about_text = g_strdup_printf (_(
	  "<b>GNU Gadu %s</b>\n"
	  "Copyright (C) 2001-2003 GNU Gadu Team\n"
	  "License: GPL\n"
	  "Homepage: http://www.gadu.gnu.pl\n\n"
	  
	  "<b>Main Programmers:</b>\n"
	  "Igor Popik &lt;thrull@slackware.pl&gt;\n"
	  "Marcin Krzyzanowski &lt;krzak@hakore.com&gt;\n\n"
	  
	  "<b>Patches:</b>\n"
	  "Mateusz Papiernik\n"
	  "Pawel Jan Maczewski\n"
	  "HelDoRe\n"
	  "Bartosz Zapalowski\n"
	  "Jakub 'shasta' Jankowski\n\n"
	  
	  "<b>Thanks to:</b>\n"
	  "Dwuziu\nAflinta\nGammaRay\nPlavi\n"
	  "see AUTHORS file for details\n\n"
	  
	  "<i>Compite time:\n%s %s</i>"),
	VERSION, __DATE__, __TIME__);
    about_timeout = g_timeout_add(50, timeout, image);
}
