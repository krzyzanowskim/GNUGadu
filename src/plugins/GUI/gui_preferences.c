/* $Id: gui_preferences.c,v 1.101 2005/03/04 16:04:53 mkobierzycki Exp $ */

/* 
 * GUI (gtk+) plugin for GNU Gadu 2 
 * 
 * Copyright (C) 2003-2005 GNU Gadu Team 
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>
#include "ggadu_support.h"
#include "signals.h"
#include "ggadu_conf.h"
#include "plugins.h"
#include "gui_preferences.h"
#include "gui_support.h"
#include "GUI_plugin.h"
#include "gui_userview.h"
#include "gui_chat.h"
#include "gui_handlers.h"

GtkWidget *plugins_dialog = NULL;
GSList *selected_plugins = NULL;
GtkTreeStore *store = NULL;
GtkWidget *list = NULL;

extern GSList *protocols;
extern GGaduPlugin *gui_handler;
extern GtkWidget *toolbar_handle_box;


static gboolean plugins_updated = FALSE;

static gboolean save_selected_plugins(GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, gpointer data
)
{
	gboolean enable;
	gchar *name = NULL;
	gsize count;

	gtk_tree_model_get(model, iter, PLUGINS_MGR_ENABLE, &enable, -1);
	gtk_tree_model_get(model, iter, PLUGINS_MGR_NAME, &name, -1);

	if (enable)
	{
		GIOChannel *ch = (GIOChannel *) data;
		GSList *all_plugins = config->all_available_plugins;

		if (!ch)
			return TRUE;

		g_io_channel_write_chars(ch, name, -1, &count, NULL);
		g_io_channel_write_chars(ch, "\n", -1, &count, NULL);

		while (all_plugins)
		{
			GGaduPluginFile *pf = (GGaduPluginFile *) all_plugins->data;

			if (!ggadu_strcasecmp(pf->name, name) && (!find_plugin_by_name(name)))
			{
				load_plugin(pf->path);
			}

			all_plugins = all_plugins->next;
		}

	}
	else if (name != NULL)
	{
		if (!ggadu_strcasecmp(name, "main-gui"))
		{
			signal_emit_full("main-gui", "gui show warning",
					 g_strdup(_("GUI is selected as DISABLED\nIf you are sure, you have to restart GNU Gadu to take effect")), "main-gui", NULL);
			return FALSE;
		}
		unload_plugin(name);
	}

	return FALSE;
}

static void enable_toggled(GtkCellRendererToggle * cell, gchar * path_str, gpointer data
)
{
	GtkTreeIter iter;
	gboolean enable;
	GtkTreeModel *model = (GtkTreeModel *) data;
	GtkTreePath *path = gtk_tree_path_new_from_string(path_str);

	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, PLUGINS_MGR_ENABLE, &enable, -1);

	enable ^= 1;

	gtk_tree_store_set(GTK_TREE_STORE(model), &iter, PLUGINS_MGR_ENABLE, enable, -1);

	gtk_tree_path_free(path);

	plugins_updated = TRUE;
}

static void row_changed(GtkTreeModel * treemodel, GtkTreePath * arg1, GtkTreeIter * arg2, gpointer user_data
)
{
	plugins_updated = TRUE;
}

GtkWidget *gui_plugins_mgr_tab(
)
{
	GtkWidget *vbox;
	GSList *plugins_list = (config) ? config->all_available_plugins : NULL;
	GSList *modules_load = (config) ? get_list_modules_load(GGADU_PLUGIN_TYPE_ANY) : NULL;
	GtkTreeIter iter;
	GtkCellRenderer *renderer = NULL;
	GtkTreeViewColumn *column = NULL;

	plugins_updated = FALSE;

	vbox = gtk_vbox_new(FALSE, 5);
//      store = gtk_tree_store_new(PLUGINS_MGR_COUNT, G_TYPE_STRING, G_TYPE_BOOLEAN);
	store = gtk_tree_store_new(PLUGINS_MGR_COUNT, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING);
	g_signal_connect(G_OBJECT(store), "row-changed", G_CALLBACK(row_changed), NULL);

	/* zaladowane pluginy na poczzatku wedle kolejnosci zadanej w pliku modules.load */
	while (modules_load)
	{
		gboolean tmpvar = FALSE;
		GGaduPlugin *pl = (GGaduPlugin *) modules_load->data;

		if (pl && find_plugin_by_name((gchar *) pl->name))
			tmpvar = TRUE;

		print_debug("%s\n", pl->name);
		gtk_tree_store_append(GTK_TREE_STORE(store), &iter, NULL);
		gtk_tree_store_set(GTK_TREE_STORE(store), &iter, PLUGINS_MGR_NAME, (gchar *) pl->name, PLUGINS_MGR_ENABLE, tmpvar, PLUGINS_MGR_DESC, (gchar *) pl->description, -1);

		modules_load = modules_load->next;
	}

	/* pozostale */
	while (plugins_list)
	{
		gboolean tmpvar = FALSE;
		GGaduPluginFile *pf = (GGaduPluginFile *) plugins_list->data;

		if (pf && !find_plugin_by_name((gchar *) pf->name))
		{
			tmpvar = FALSE;

			print_debug("%s\n", pf->name);
			gtk_tree_store_append(GTK_TREE_STORE(store), &iter, NULL);
			gtk_tree_store_set(GTK_TREE_STORE(store), &iter, PLUGINS_MGR_NAME, (gchar *) pf->name, PLUGINS_MGR_ENABLE, tmpvar, -1);
		}
		plugins_list = plugins_list->next;
	}

	list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(list), TRUE);

	g_object_unref(G_OBJECT(store));

	renderer = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes(_("Enabled"), renderer, "active", PLUGINS_MGR_ENABLE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	g_signal_connect(renderer, "toggled", G_CALLBACK(enable_toggled), store);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", PLUGINS_MGR_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Description"), renderer, "text", PLUGINS_MGR_DESC, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(list), TRUE, TRUE, 0);

	gtk_widget_show_all(vbox);

	return vbox;
}

static void tree_toggled(GtkWidget * tree, gpointer data
)
{
	GtkWidget *expand = data;

	gtk_widget_set_sensitive(expand, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tree)));
}

static void color_select_dialog(GtkColorButton * color_button, gpointer entry
)
{
	GdkColor color;
	gchar *tmp = NULL;

	gtk_color_button_get_color(color_button, &color);
	tmp = gtk_color_selection_palette_to_string(&color, 1);
	gtk_entry_set_text(GTK_ENTRY(entry), tmp);
	g_free(tmp);
}

static GtkWidget *create_colors_tab(
)
{
	GtkWidget *colors_vbox;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *tabbox;
	GtkWidget *entry;
	GtkWidget *button;
	GtkWidget *frame;
	GdkColor color_msg_header, color_msg_body, color_out_msg_header, color_out_msg_body;

	colors_vbox = gtk_vbox_new(FALSE, 2);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(colors_vbox), hbox, FALSE, FALSE, 0);

	image = gtk_image_new();
	gtk_image_set_from_stock(GTK_IMAGE(image), "gtk-select-color", GTK_ICON_SIZE_DND);
	label = gtk_label_new(_("\nColors settings\n\n"));

	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	frame = gtk_frame_new(_("Incoming messages"));
	gtk_box_pack_start(GTK_BOX(colors_vbox), frame, FALSE, FALSE, 0);

	tabbox = gtk_table_new(3, 3, FALSE);
	gtk_container_add(GTK_CONTAINER(frame), tabbox);
	gtk_table_set_row_spacings(GTK_TABLE(tabbox), 7);
	gtk_table_set_col_spacings(GTK_TABLE(tabbox), 5);

	/* */
	label = gtk_label_new(_("Message header"));
	entry = gtk_entry_new();

	if (ggadu_config_var_get(gui_handler, "msg_header_color"))
		gtk_entry_set_text(GTK_ENTRY(entry), (gchar *) ggadu_config_var_get(gui_handler, "msg_header_color"));
	gdk_color_parse(gtk_entry_get_text(GTK_ENTRY(entry)), &color_msg_header);

	button = gtk_color_button_new_with_color(&color_msg_header);

	gtk_table_attach_defaults(GTK_TABLE(tabbox), label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), entry, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), button, 2, 3, 0, 1);

	g_signal_connect(G_OBJECT(button), "color-set", G_CALLBACK(color_select_dialog), entry);
	g_object_set_data(G_OBJECT(colors_vbox), "msg_header_color", entry);

	/* */
	label = gtk_label_new(_("Message body"));
	entry = gtk_entry_new();

	if (ggadu_config_var_get(gui_handler, "msg_body_color"))
		gtk_entry_set_text(GTK_ENTRY(entry), (gchar *) ggadu_config_var_get(gui_handler, "msg_body_color"));
	gdk_color_parse(gtk_entry_get_text(GTK_ENTRY(entry)), &color_msg_body);

	button = gtk_color_button_new_with_color(&color_msg_body);

	gtk_table_attach_defaults(GTK_TABLE(tabbox), label, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), entry, 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), button, 2, 3, 1, 2);
	g_signal_connect(G_OBJECT(button), "color-set", G_CALLBACK(color_select_dialog), entry);
	g_object_set_data(G_OBJECT(colors_vbox), "msg_body_color", entry);


	frame = gtk_frame_new(_("Outgoing messages"));
	gtk_box_pack_start(GTK_BOX(colors_vbox), frame, FALSE, FALSE, 0);

	tabbox = gtk_table_new(3, 3, FALSE);
	gtk_container_add(GTK_CONTAINER(frame), tabbox);
	gtk_table_set_row_spacings(GTK_TABLE(tabbox), 7);
	gtk_table_set_col_spacings(GTK_TABLE(tabbox), 5);

	/* */
	label = gtk_label_new(_("Message header"));
	entry = gtk_entry_new();

	if (ggadu_config_var_get(gui_handler, "msg_out_header_color"))
		gtk_entry_set_text(GTK_ENTRY(entry), (gchar *) ggadu_config_var_get(gui_handler, "msg_out_header_color"));

	gdk_color_parse(gtk_entry_get_text(GTK_ENTRY(entry)), &color_out_msg_header);

	button = gtk_color_button_new_with_color(&color_out_msg_header);

	gtk_table_attach_defaults(GTK_TABLE(tabbox), label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), entry, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), button, 2, 3, 0, 1);
	g_signal_connect(G_OBJECT(button), "color-set", G_CALLBACK(color_select_dialog), entry);
	g_object_set_data(G_OBJECT(colors_vbox), "msg_out_header_color", entry);

	/* */
	label = gtk_label_new(_("Message body"));
	entry = gtk_entry_new();

	if (ggadu_config_var_get(gui_handler, "msg_out_body_color"))
		gtk_entry_set_text(GTK_ENTRY(entry), (gchar *) ggadu_config_var_get(gui_handler, "msg_out_body_color"));

	gdk_color_parse(gtk_entry_get_text(GTK_ENTRY(entry)), &color_out_msg_body);
	button = gtk_color_button_new_with_color(&color_out_msg_body);

	gtk_table_attach_defaults(GTK_TABLE(tabbox), label, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), entry, 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), button, 2, 3, 1, 2);

	g_signal_connect(G_OBJECT(button), "color-set", G_CALLBACK(color_select_dialog), entry);
	g_object_set_data(G_OBJECT(colors_vbox), "msg_out_body_color", entry);

	return colors_vbox;
}

static void show_fonts_select_dialog(GtkWidget * widget, gpointer user_data
)
{
	GtkWidget *entry = (GtkWidget *) user_data;
	GtkWidget *font_selector;
	gint response;
	const gchar *font_txt;

	font_selector = gtk_font_selection_dialog_new(_("Select font"));

	font_txt = gtk_entry_get_text(GTK_ENTRY(entry));
	if (font_txt && *font_txt)
		gtk_font_selection_set_font_name(GTK_FONT_SELECTION(GTK_FONT_SELECTION_DIALOG(font_selector)->fontsel), font_txt);

	response = gtk_dialog_run(GTK_DIALOG(font_selector));

	if (response == GTK_RESPONSE_OK)
	{
		gchar *tmp;

		tmp = gtk_font_selection_get_font_name(GTK_FONT_SELECTION(GTK_FONT_SELECTION_DIALOG(font_selector)->fontsel));
		gtk_entry_set_text(GTK_ENTRY(entry), tmp);
		g_free(tmp);
	}

	gtk_widget_destroy(font_selector);
}

static GtkWidget *create_fonts_tab(
)
{
	GtkWidget *fonts_vbox;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *tabbox;
	GtkWidget *entry;
	GtkWidget *button;
	GtkWidget *frame;

	fonts_vbox = gtk_vbox_new(FALSE, 2);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(fonts_vbox), hbox, FALSE, FALSE, 0);

	image = gtk_image_new();
	gtk_image_set_from_stock(GTK_IMAGE(image), "gtk-select-font", GTK_ICON_SIZE_DND);
	label = gtk_label_new(_("\nFonts settings\n\n"));

	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	frame = gtk_frame_new(_("Incoming messages"));
	gtk_box_pack_start(GTK_BOX(fonts_vbox), frame, FALSE, FALSE, 0);

	tabbox = gtk_table_new(3, 3, FALSE);
	gtk_container_add(GTK_CONTAINER(frame), tabbox);
	gtk_table_set_row_spacings(GTK_TABLE(tabbox), 7);
	gtk_table_set_col_spacings(GTK_TABLE(tabbox), 5);

	label = gtk_label_new(_("Message header"));
	entry = gtk_entry_new();
	button = gtk_button_new_from_stock("gtk-select-font");

	gtk_table_attach_defaults(GTK_TABLE(tabbox), label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), entry, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), button, 2, 3, 0, 1);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(show_fonts_select_dialog), entry);
	g_object_set_data(G_OBJECT(fonts_vbox), "msg_header_font", entry);

	if (ggadu_config_var_get(gui_handler, "msg_header_font"))
		gtk_entry_set_text(GTK_ENTRY(entry), (gchar *) ggadu_config_var_get(gui_handler, "msg_header_font"));

	label = gtk_label_new(_("Message body"));
	entry = gtk_entry_new();
	button = gtk_button_new_from_stock("gtk-select-font");

	gtk_table_attach_defaults(GTK_TABLE(tabbox), label, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), entry, 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), button, 2, 3, 1, 2);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(show_fonts_select_dialog), entry);
	g_object_set_data(G_OBJECT(fonts_vbox), "msg_body_font", entry);

	if (ggadu_config_var_get(gui_handler, "msg_body_font"))
		gtk_entry_set_text(GTK_ENTRY(entry), (gchar *) ggadu_config_var_get(gui_handler, "msg_body_font"));

	frame = gtk_frame_new(_("Outgoing messages"));
	gtk_box_pack_start(GTK_BOX(fonts_vbox), frame, FALSE, FALSE, 0);

	tabbox = gtk_table_new(4, 3, FALSE);
	gtk_container_add(GTK_CONTAINER(frame), tabbox);
	gtk_table_set_row_spacings(GTK_TABLE(tabbox), 7);
	gtk_table_set_col_spacings(GTK_TABLE(tabbox), 5);

	label = gtk_label_new(_("Message header"));
	entry = gtk_entry_new();
	button = gtk_button_new_from_stock("gtk-select-font");

	gtk_table_attach_defaults(GTK_TABLE(tabbox), label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), entry, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), button, 2, 3, 0, 1);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(show_fonts_select_dialog), entry);
	g_object_set_data(G_OBJECT(fonts_vbox), "msg_out_header_font", entry);

	if (ggadu_config_var_get(gui_handler, "msg_out_header_font"))
		gtk_entry_set_text(GTK_ENTRY(entry), (gchar *) ggadu_config_var_get(gui_handler, "msg_out_header_font"));

	label = gtk_label_new(_("Message body"));
	entry = gtk_entry_new();
	button = gtk_button_new_from_stock("gtk-select-font");

	gtk_table_attach_defaults(GTK_TABLE(tabbox), label, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), entry, 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), button, 2, 3, 1, 2);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(show_fonts_select_dialog), entry);
	g_object_set_data(G_OBJECT(fonts_vbox), "msg_out_body_font", entry);

	if (ggadu_config_var_get(gui_handler, "msg_out_body_font"))
		gtk_entry_set_text(GTK_ENTRY(entry), (gchar *) ggadu_config_var_get(gui_handler, "msg_out_body_font"));

    label = gtk_label_new(_("Message editing"));
    entry = gtk_entry_new();
    button = gtk_button_new_from_stock("gtk-select-font");
                                                                                              gtk_table_attach_defaults(GTK_TABLE(tabbox), label, 0, 1, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(tabbox), entry, 1, 2, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(tabbox), button, 2, 3, 2, 3);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(show_fonts_select_dialog), entry);                                                                                             
    g_object_set_data(G_OBJECT(fonts_vbox), "msg_out_edit_font", entry);
        
    if (ggadu_config_var_get(gui_handler, "msg_out_edit_font"))
        gtk_entry_set_text(GTK_ENTRY(entry), (gchar *) ggadu_config_var_get(gui_handler, "msg_out_edit_font"));

	frame = gtk_frame_new(_("Contact list"));
	gtk_box_pack_start(GTK_BOX(fonts_vbox), frame, FALSE, FALSE, 0);

	tabbox = gtk_table_new(3, 3, FALSE);
	gtk_container_add(GTK_CONTAINER(frame), tabbox);
	gtk_table_set_row_spacings(GTK_TABLE(tabbox), 7);
	gtk_table_set_col_spacings(GTK_TABLE(tabbox), 5);

	label = gtk_label_new(_("Contact"));
	entry = gtk_entry_new();
	button = gtk_button_new_from_stock("gtk-select-font");

	gtk_table_attach_defaults(GTK_TABLE(tabbox), label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), entry, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), button, 2, 3, 0, 1);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(show_fonts_select_dialog), entry);
	g_object_set_data(G_OBJECT(fonts_vbox), "contact_list_contact_font", entry);

	if (ggadu_config_var_get(gui_handler, "contact_list_contact_font"))
		gtk_entry_set_text(GTK_ENTRY(entry), (gchar *) ggadu_config_var_get(gui_handler, "contact_list_contact_font"));

	label = gtk_label_new(_("Protocol\n(tree view)"));
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	entry = gtk_entry_new();
	button = gtk_button_new_from_stock("gtk-select-font");

	gtk_table_attach_defaults(GTK_TABLE(tabbox), label, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), entry, 1, 2, 1, 2);
    gtk_table_attach(GTK_TABLE(tabbox), button, 2, 3, 1, 2, GTK_FILL, GTK_SHRINK, 0, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(show_fonts_select_dialog), entry);
	g_object_set_data(G_OBJECT(fonts_vbox), "contact_list_protocol_font", entry);

	if (ggadu_config_var_get(gui_handler, "contact_list_protocol_font"))
		gtk_entry_set_text(GTK_ENTRY(entry), (gchar *) ggadu_config_var_get(gui_handler, "contact_list_protocol_font"));

	return fonts_vbox;
}

static void show_file_select_dialog(GtkWidget * widget, gpointer user_data
)
{
	GtkWidget *entry = (GtkWidget *) user_data;
	GtkWidget *file_chooser = NULL;
	GtkFileFilter *file_filter = NULL;
	const gchar *_filename = NULL;
	const gchar *filename = NULL;
	gint response;

	file_chooser = gtk_file_chooser_dialog_new(_("Select file"), NULL,
						   GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	_filename = gtk_entry_get_text(GTK_ENTRY(entry));

	if (_filename && *_filename)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(file_chooser), _filename);

	file_filter = gtk_file_filter_new();
	gtk_file_filter_add_mime_type(file_filter, "audio/x-wav");
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(file_chooser), file_filter);

	response = gtk_dialog_run(GTK_DIALOG(file_chooser));

	if (response == GTK_RESPONSE_OK)
	{
		filename = (gchar *) gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
		gtk_entry_set_text(GTK_ENTRY(entry), filename);
	}

	gtk_widget_destroy(file_chooser);
}

static GtkWidget *create_sound_tab()
{
	GtkWidget *sound_vbox;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *tabbox;
	GtkWidget *entry;
	GtkWidget *button;
	GtkWidget *frame;

	sound_vbox = gtk_vbox_new(FALSE, 2);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(sound_vbox), hbox, FALSE, FALSE, 0);

	image = gtk_image_new();
	gtk_image_set_from_stock(GTK_IMAGE(image), "gtk-cdrom", GTK_ICON_SIZE_DND);
	label = gtk_label_new(_("\nSound settings\n\n"));

	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	frame = gtk_frame_new(_("Messages"));
	gtk_box_pack_start(GTK_BOX(sound_vbox), frame, FALSE, FALSE, 0);

	tabbox = gtk_table_new(3, 3, FALSE);
	gtk_container_add(GTK_CONTAINER(frame), tabbox);
	gtk_table_set_row_spacings(GTK_TABLE(tabbox), 7);
	gtk_table_set_col_spacings(GTK_TABLE(tabbox), 5);

	label = gtk_label_new(_("Incoming message:"));
	entry = gtk_entry_new();
	button = gtk_button_new_from_stock("gtk-open");

	gtk_table_attach_defaults(GTK_TABLE(tabbox), label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), entry, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), button, 2, 3, 0, 1);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(show_file_select_dialog), entry);
	g_object_set_data(G_OBJECT(sound_vbox), "sound_msg_in", entry);

	if (ggadu_config_var_get(gui_handler, "sound_msg_in"))
		gtk_entry_set_text(GTK_ENTRY(entry), (gchar *) ggadu_config_var_get(gui_handler, "sound_msg_in"));

	label = gtk_label_new(_("Initial message:"));
	entry = gtk_entry_new();
	button = gtk_button_new_from_stock("gtk-open");

	gtk_table_attach_defaults(GTK_TABLE(tabbox), label, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), entry, 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), button, 2, 3, 1, 2);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(show_file_select_dialog), entry);
	g_object_set_data(G_OBJECT(sound_vbox), "sound_msg_in_first", entry);

	if (ggadu_config_var_get(gui_handler, "sound_msg_in_first"))
		gtk_entry_set_text(GTK_ENTRY(entry), (gchar *) ggadu_config_var_get(gui_handler, "sound_msg_in_first"));

	label = gtk_label_new(_("Outgoing message:"));
	entry = gtk_entry_new();
	button = gtk_button_new_from_stock("gtk-open");

	gtk_table_attach_defaults(GTK_TABLE(tabbox), label, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), entry, 1, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), button, 2, 3, 2, 3);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(show_file_select_dialog), entry);
	g_object_set_data(G_OBJECT(sound_vbox), "sound_msg_out", entry);

	if (ggadu_config_var_get(gui_handler, "sound_msg_out"))
		gtk_entry_set_text(GTK_ENTRY(entry), (gchar *) ggadu_config_var_get(gui_handler, "sound_msg_out"));

	return sound_vbox;
}

static GtkWidget *create_chat_tab()
{
	GtkWidget *chat_vbox;
	GtkWidget *vbox;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *tabbox;
	GtkWidget *emotic;
	GtkWidget *send_on_enter;
	GtkWidget *chatstyle;
	GtkWidget *chatwindowwidth;
	GtkWidget *chatwindowheight;
	GtkWidget *chatwindowshow;
	GtkWidget *chatwindowraise;
	GtkWidget *use_username;
	GtkWidget *chat_paned_size;
	GtkWidget *irc_msg_style = NULL;
#ifdef USE_GTKSPELL
	GtkWidget *tabbox_spell;
	GtkWidget *use_spell;
	GtkWidget *combo_spell;
#endif
	GtkWidget *label0_align, *label1_align, *label2_align;
	GtkWidget *label3_align;

	chat_vbox = gtk_vbox_new(FALSE, 5);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(chat_vbox), hbox, FALSE, FALSE, 0);

	image = gtk_image_new();
	gtk_image_set_from_stock(GTK_IMAGE(image), "gtk-justify-fill", GTK_ICON_SIZE_DND);
	label = gtk_label_new(_("\nChat window settings\n\n"));

	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(chat_vbox), vbox, FALSE, FALSE, 0);

	chatstyle = gtk_check_button_new_with_label(_("Tabbed chat window style"));
	gtk_box_pack_start(GTK_BOX(vbox), chatstyle, FALSE, FALSE, 0);

	g_object_set_data(G_OBJECT(chat_vbox), "chatstyle", chatstyle);

	irc_msg_style = gtk_check_button_new_with_label(_("IRC-like messages displaying"));
	gtk_box_pack_start(GTK_BOX(vbox), irc_msg_style, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(chat_vbox), "irc_msg_style", irc_msg_style);
	
	emotic = gtk_check_button_new_with_label(_("Enable emoticons"));
	gtk_box_pack_start(GTK_BOX(vbox), emotic, FALSE, FALSE, 0);

	g_object_set_data(G_OBJECT(chat_vbox), "emotic", emotic);

	send_on_enter = gtk_check_button_new_with_label(_("'Enter' - send message"));
	gtk_box_pack_start(GTK_BOX(vbox), send_on_enter, FALSE, FALSE, 0);

	g_object_set_data(G_OBJECT(chat_vbox), "send_on_enter", send_on_enter);


	chatwindowshow = gtk_check_button_new_with_label(_("Allways show new window on new message"));
	gtk_box_pack_start(GTK_BOX(vbox), chatwindowshow, FALSE, FALSE, 0);

	g_object_set_data(G_OBJECT(chat_vbox), "chatwindowshow", chatwindowshow);

	chatwindowraise = gtk_check_button_new_with_label(_("Raise window on new message"));
	gtk_box_pack_start(GTK_BOX(vbox), chatwindowraise, FALSE, FALSE, 0);

	g_object_set_data(G_OBJECT(chat_vbox), "chatwindowraise", chatwindowraise);

	use_username = gtk_check_button_new_with_label(_("Use system username instead of \"Me\" in chat window"));
	gtk_box_pack_start(GTK_BOX(vbox), use_username, FALSE, FALSE, 0);

	g_object_set_data(G_OBJECT(chat_vbox), "use_username", use_username);

#ifdef USE_GTKSPELL
	tabbox_spell = gtk_table_new(1, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(chat_vbox), tabbox_spell, FALSE, FALSE, 0);

	use_spell = gtk_check_button_new_with_label(_("Spell Checking"));

	g_object_set_data(G_OBJECT(chat_vbox), "spell", use_spell);

	label = gtk_label_new(_("Dictionary:"));
	combo_spell = gtk_combo_box_new_text();
	label3_align = gtk_alignment_new(0.9, 0.5, 0, 0);
	gtk_container_add(GTK_CONTAINER(label3_align), label);

	g_object_set_data(G_OBJECT(chat_vbox), "combo_spell", combo_spell);

	gtk_table_attach_defaults(GTK_TABLE(tabbox_spell), use_spell, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox_spell), label3_align, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox_spell), combo_spell, 2, 3, 0, 1);
#endif

	label0_align = gtk_alignment_new(0, 0.5, 0, 0);
	label1_align = gtk_alignment_new(0, 0.5, 0, 0);
	label2_align = gtk_alignment_new(0, 0.5, 0, 0);

	tabbox = gtk_table_new(1, 0, FALSE);

	gtk_table_set_row_spacings(GTK_TABLE(tabbox), 7);
	gtk_table_set_col_spacings(GTK_TABLE(tabbox), 0);
	
	GtkWidget *exp = gtk_expander_new(_("More advanced options"));
	GtkWidget *frm = gtk_frame_new(_("Default size of window"));
	gtk_frame_set_shadow_type(GTK_FRAME(frm),GTK_SHADOW_ETCHED_OUT);
	gtk_container_add(GTK_CONTAINER(frm), tabbox);
	gtk_container_add(GTK_CONTAINER(exp), frm);
	gtk_box_pack_start(GTK_BOX(chat_vbox), exp, FALSE, FALSE, 4);

	label = gtk_label_new(_("Width:"));
	gtk_container_add(GTK_CONTAINER(label1_align), label);
	gtk_table_attach(GTK_TABLE(tabbox), label1_align, 0, 1, 0, 1,GTK_FILL,GTK_EXPAND,0,1);

	chatwindowwidth = gtk_spin_button_new_with_range(100, 1000, 10);
	g_object_set_data(G_OBJECT(chat_vbox), "chatwindowwidth", chatwindowwidth);

	gtk_table_attach(GTK_TABLE(tabbox), chatwindowwidth, 1, 2, 0, 1,GTK_FILL,GTK_EXPAND,0,1);

	label = gtk_label_new(_("Height:"));
	gtk_container_add(GTK_CONTAINER(label2_align), label);
	gtk_table_attach(GTK_TABLE(tabbox), label2_align, 0, 1, 1, 2,GTK_FILL,GTK_EXPAND,0,1);

	chatwindowheight = gtk_spin_button_new_with_range(50, 1000, 10);
	g_object_set_data(G_OBJECT(chat_vbox), "chatwindowheight", chatwindowheight);

	gtk_table_attach(GTK_TABLE(tabbox), chatwindowheight, 1, 2, 1, 2,GTK_FILL,GTK_EXPAND,0,1);

	/* chat_paned_size */
	/* ZONK - how to name it ? */

	label = gtk_label_new(_("Window split (percent):"));
	label3_align = gtk_alignment_new(0, 0.5, 0, 0);
	chat_paned_size = gtk_spin_button_new_with_range(5, 100, 5);
	gtk_container_add(GTK_CONTAINER(label3_align), label);

	g_object_set_data(G_OBJECT(chat_vbox), "chat_paned_size", chat_paned_size);

	gtk_table_attach_defaults(GTK_TABLE(tabbox), label3_align, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), chat_paned_size, 1, 2, 2, 3);

	if (ggadu_config_var_get(gui_handler, "chat_paned_size"))
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(chat_paned_size), (gint) ggadu_config_var_get(gui_handler, "chat_paned_size"));

	if (ggadu_config_var_get(gui_handler, "irc_msg_style"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(irc_msg_style), TRUE);

	ggadu_config_var_set(gui_handler, "irc_msg_style", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(irc_msg_style)));


/*	label = gtk_label_new(_("percent"));
	label4_align = gtk_alignment_new(0, 0.5, 0, 0);
	gtk_container_add(GTK_CONTAINER(label4_align), label);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), label4_align, 2, 3, 2, 3);
*/

	return chat_vbox;
}

static GtkWidget *create_advanced_tab()
{
	GtkWidget *hide_on_start;
	GtkWidget *close_on_esc;
	GtkWidget *blink_interval = NULL;
	GtkWidget *blink = NULL;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *tabbox;
	GtkWidget *tabbox2;
	GtkWidget *label0_align;
	GtkWidget *label1_align = gtk_alignment_new(0, 0.5, 0, 0);
	GtkWidget *label2_align = gtk_alignment_new(0, 0.5, 0, 0);
	GtkWidget *label3_align = gtk_alignment_new(0, 0.5, 0, 0);
	GtkWidget *label4_align = gtk_alignment_new(0, 0.5, 0, 0);
	GtkWidget *combo_theme = NULL;
	GtkWidget *combo_skins = NULL;
	GtkWidget *combo_icons = NULL;
	GtkWidget *notify_status_changes = NULL;

	gchar *dirname = NULL;
	GDir *dir;
	GtkWidget *adv_vbox = gtk_vbox_new(FALSE, 5);
	GtkWidget *hbox = gtk_hbox_new(FALSE, 5);

	gtk_box_pack_start(GTK_BOX(adv_vbox), hbox, FALSE, FALSE, 0);

	image = gtk_image_new();
	gtk_image_set_from_stock(GTK_IMAGE(image), "gtk-properties", GTK_ICON_SIZE_DND);
	label = gtk_label_new(_("\nAdvanced settings\n\n"));

	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	/* hide_on_start */
	hide_on_start = gtk_check_button_new_with_label(_("Auto hide window on start"));
	gtk_box_pack_start(GTK_BOX(adv_vbox), hide_on_start, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(adv_vbox), "hide_on_start", hide_on_start);

	/* notify status changes in Chat window */
	notify_status_changes = gtk_check_button_new_with_label(_("Notify about status changes inside chat window"));
	gtk_box_pack_start(GTK_BOX(adv_vbox), notify_status_changes, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(adv_vbox), "notify_status_changes", notify_status_changes);

	/* close_on_esc */
	close_on_esc = gtk_check_button_new_with_label(_("'ESC' - close chat window"));
	gtk_box_pack_start(GTK_BOX(adv_vbox), close_on_esc, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(adv_vbox), "close_on_esc", close_on_esc);
	
	/* blink */
	blink = gtk_check_button_new_with_label(_("Blink status icon while connecting every:"));
	blink_interval = gtk_spin_button_new_with_range(0, 2000, 100);
	g_object_set_data(G_OBJECT(adv_vbox), "blink", blink);
	g_object_set_data(G_OBJECT(adv_vbox), "blink_interval", blink_interval);

	g_signal_connect(blink, "toggled", G_CALLBACK(tree_toggled), blink_interval);

	tabbox = gtk_table_new(3, 4, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(tabbox), 7);
	gtk_table_set_col_spacings(GTK_TABLE(tabbox), 5);

	gtk_box_pack_start(GTK_BOX(adv_vbox), tabbox, FALSE, FALSE, 0);

	label = gtk_label_new(_("miliseconds"));
	label0_align = gtk_alignment_new(0, 0.5, 0, 0);
	gtk_container_add(GTK_CONTAINER(label0_align), label);

	if (ggadu_config_var_get(gui_handler, "blink"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(blink), TRUE);
	else
		gtk_widget_set_sensitive(blink_interval, FALSE);

	gtk_table_attach_defaults(GTK_TABLE(tabbox), blink, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), blink_interval, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), label0_align, 2, 3, 0, 1);


	/* themes */
	combo_theme = gtk_combo_box_new_text();
	g_object_set_data(G_OBJECT(adv_vbox), "combo_theme", combo_theme);

	label = gtk_label_new(_("Selected theme:"));
	gtk_container_add(GTK_CONTAINER(label2_align), label);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), label2_align, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), combo_theme, 1, 3, 2, 3);

	dirname = g_build_filename(PACKAGE_DATA_DIR, "themes", NULL);
	dir = g_dir_open(dirname, 0, NULL);
	print_debug("tryin to read themes directory %s\n", dirname);
	g_free(dirname);

	if (dir)
	{
		GSList *list_theme = NULL;
		gchar *theme_current;
		gchar *theme_name_file;
		gchar *theme_name;
		gint i = 0;

		theme_current = ggadu_config_var_get(gui_handler, "theme");

		while ((theme_name_file = (gchar *) g_dir_read_name(dir)) != NULL)
		{
			print_debug("theme file : %s", theme_name_file);

			if (g_str_has_suffix(theme_name_file, ".theme"))
			{
				theme_name = g_strndup(theme_name_file, strlen(theme_name_file) - strlen(".theme"));

				list_theme = g_slist_append(list_theme, g_strdup(theme_name));
				gtk_combo_box_append_text(GTK_COMBO_BOX(combo_theme), g_strdup(theme_name));

				if (theme_current && !ggadu_strcasecmp(theme_name, theme_current))
					gtk_combo_box_set_active(GTK_COMBO_BOX(combo_theme), i);

				i++;
				g_free(theme_name);
			}

		}

		g_dir_close(dir);
		g_object_set_data(G_OBJECT(combo_theme), "combo_theme_slist", list_theme);

	}

	/* skins */
	combo_skins = gtk_combo_box_new_text();
	g_object_set_data(G_OBJECT(adv_vbox), "combo_skins", combo_skins);

	label = gtk_label_new(_("Selected skin:"));
	gtk_container_add(GTK_CONTAINER(label1_align), label);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), label1_align, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), combo_skins, 1, 3, 3, 4);

	dirname = g_build_filename(config->configdir, "skins", NULL);
g_print("DIRNAME %s\n", dirname);
	dir = g_dir_open(dirname, 0, NULL);

	if (!dir) {
	    g_free(dirname);
	    dirname = g_build_filename(PACKAGE_DATA_DIR, "skins", NULL);
	    dir = g_dir_open(dirname, 0, NULL);
	}
	
	print_debug("tryin to read skins directory %s\n", dirname);

	if (dir)
	{
		GSList *list_skins = NULL;
		gchar *skin_current;
		gchar *skin_dir;
		gint i = 0;
	
		list_skins = g_slist_append(list_skins, g_strdup(_("")));
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo_skins), g_strdup(_("default")));
		
		// select 1st one anyway...
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_skins), i);

		i++;
		
		while ((skin_dir = (gchar *) g_dir_read_name(dir)) != NULL)
		{
			gchar *full_skin_dir = g_build_filename(dirname, skin_dir, NULL);
			print_debug("skin file : %s", skin_dir);
			
			if (g_file_test(full_skin_dir, G_FILE_TEST_IS_DIR))
			{
				list_skins = g_slist_append(list_skins, g_strdup(skin_dir));
				gtk_combo_box_append_text(GTK_COMBO_BOX(combo_skins), g_strdup(skin_dir));

				if (skin_current && !ggadu_strcasecmp(skin_dir, skin_current))
					gtk_combo_box_set_active(GTK_COMBO_BOX(combo_skins), i);

				i++;
			}
			g_free(full_skin_dir);
		}

		g_dir_close(dir);
		g_object_set_data(G_OBJECT(combo_skins), "combo_skins_slist", list_skins);
	}

	g_free(dirname);

	/* iconset */
	combo_icons = gtk_combo_box_new_text();
	g_object_set_data(G_OBJECT(adv_vbox), "combo_icons", combo_icons);
	label = gtk_label_new(_("Selected icon set:"));
	gtk_container_add(GTK_CONTAINER(label3_align), label);

	gtk_table_attach_defaults(GTK_TABLE(tabbox), label3_align, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(tabbox), combo_icons, 1, 3, 4, 5);

	dirname = g_build_filename(PACKAGE_DATA_DIR, "pixmaps", "icons", NULL);
	print_debug("Trying to read icons directory %s", dirname);
	dir = g_dir_open(dirname, 0, NULL);
	g_free(dirname);

	if (dir)
	{
		GSList *list_icons = NULL;
		gchar *icons_current;
		gchar *icons_dir;
		gint i = 0;

		icons_current = ggadu_config_var_get(gui_handler, "icons");

		while ((icons_dir = (gchar *) g_dir_read_name(dir)) != NULL)
		{
			gchar *testdirname = g_build_filename(PACKAGE_DATA_DIR, "pixmaps", "icons", icons_dir, NULL);

			if (g_file_test(testdirname, G_FILE_TEST_IS_DIR))
			{
				print_debug("%s\n", icons_dir);
				list_icons = g_slist_append(list_icons, g_strdup(icons_dir));
				gtk_combo_box_append_text(GTK_COMBO_BOX(combo_icons), g_strdup(icons_dir));

				if (icons_dir && !ggadu_strcasecmp(icons_dir, icons_current))
					gtk_combo_box_set_active(GTK_COMBO_BOX(combo_icons), i);

				i++;
			}
			g_free(testdirname);
		}

		g_dir_close(dir);
		g_object_set_data(G_OBJECT(combo_icons), "combo_icons_slist", list_icons);
	}

	/* browsers */
	tabbox2 = gtk_table_new(1, 0, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(tabbox2), 7);
	gtk_table_set_col_spacings(GTK_TABLE(tabbox2), 5);

	gtk_box_pack_start(GTK_BOX(adv_vbox), tabbox2, TRUE, TRUE, 0);
	label = gtk_label_new(_("Web Browser:"));
	gtk_container_add(GTK_CONTAINER(label4_align), label);

	GtkWidget *entry = gtk_entry_new();
	gtk_table_attach_defaults(GTK_TABLE(tabbox2), label4_align, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tabbox2), entry, 1, 2, 0, 1);
	g_object_set_data(G_OBJECT(adv_vbox), "browser_exec", entry);

	
	/* set */
	if (ggadu_config_var_get(gui_handler, "blink_interval"))
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(blink_interval), (gint) ggadu_config_var_get(gui_handler, "blink_interval"));

	if (ggadu_config_var_get(gui_handler, "hide_on_start"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_on_start), TRUE);

	if (ggadu_config_var_get(gui_handler, "close_on_esc"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(close_on_esc), TRUE);

	if (ggadu_config_var_get(gui_handler, "notify_status_changes"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(notify_status_changes), TRUE);

	if (ggadu_config_var_get(gui_handler, "browser_exec"))
		gtk_entry_set_text(GTK_ENTRY(entry), (gchar *) ggadu_config_var_get(gui_handler, "browser_exec"));

	return adv_vbox;
}

void gui_preferences(GtkWidget * widget, gpointer data)
{
	GtkWidget *preferences;
	GtkWidget *notebook;
	GtkWidget *image;
	GtkWidget *general_vbox;
	GtkWidget *chat_vbox;
	GtkWidget *sound_vbox;
	GtkWidget *colors_vbox;
	GtkWidget *fonts_vbox;
	GtkWidget *adv_vbox;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *show_active;
	GtkWidget *tree;
	GtkWidget *expand;
	GtkWidget *usexosdfornewmsgs = NULL;
	GtkWidget *usexosdforstatuschange = NULL;
	GtkWidget *show_toolbar;
	GtkWidget *descr_on_list;
	GtkWidget *tabbox;
	GtkWidget *entry;
	GdkPixbuf *windowicon = NULL;
	gchar *previous_theme = ggadu_config_var_get(gui_handler, "theme");
	gchar *previous_icons = ggadu_config_var_get(gui_handler, "icons");
	gint response = -1;

	print_debug("Preferences");

	preferences =
		gtk_dialog_new_with_buttons(_("Preferences"), NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					    GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);

	gtk_window_set_resizable(GTK_WINDOW(preferences), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(preferences),2);

	if ((windowicon = create_pixbuf(GGADU_DEFAULT_ICON_FILENAME)) != NULL)
	{
		gtk_window_set_icon(GTK_WINDOW(preferences), windowicon);
		gdk_pixbuf_unref(windowicon);
	}


	notebook = gtk_notebook_new();
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(preferences)->vbox), notebook);

	general_vbox = gtk_vbox_new(FALSE, 4);
	gtk_container_set_border_width(GTK_CONTAINER(general_vbox), 10);

	chat_vbox = create_chat_tab();
	gtk_container_set_border_width(GTK_CONTAINER(chat_vbox), 10);

	sound_vbox = create_sound_tab();
	gtk_container_set_border_width(GTK_CONTAINER(sound_vbox), 10);

	colors_vbox = create_colors_tab();
	gtk_container_set_border_width(GTK_CONTAINER(colors_vbox), 10);

	fonts_vbox = create_fonts_tab();
	gtk_container_set_border_width(GTK_CONTAINER(fonts_vbox), 10);

	adv_vbox = create_advanced_tab();
	gtk_container_set_border_width(GTK_CONTAINER(adv_vbox), 10);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), general_vbox, gtk_label_new(_("General")));

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), chat_vbox, gtk_label_new(_("Chat window")));

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sound_vbox, gtk_label_new(_("Sound")));

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), colors_vbox, gtk_label_new(_("Colors")));

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), fonts_vbox, gtk_label_new(_("Fonts")));

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), adv_vbox, gtk_label_new(_("Advanced")));

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), gui_plugins_mgr_tab(), gtk_label_new(_("Plugins manager")));

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(general_vbox), hbox, FALSE, FALSE, 0);

	image = gtk_image_new();
	gtk_image_set_from_stock(GTK_IMAGE(image), "gtk-preferences", GTK_ICON_SIZE_DND);
	label = gtk_label_new(_("\nGUI plugin preferences\n\n"));

	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(general_vbox), vbox, FALSE, FALSE, 0);


	tree = gtk_check_button_new_with_label(_("Tree users list"));
	gtk_box_pack_start(GTK_BOX(vbox), tree, FALSE, FALSE, 0);

	expand = gtk_check_button_new_with_label(_("Expand Tree by default"));
	gtk_box_pack_start(GTK_BOX(vbox), expand, FALSE, FALSE, 0);

	g_signal_connect(tree, "toggled", G_CALLBACK(tree_toggled), expand);

	show_active = gtk_check_button_new_with_label(_("Show only active users"));
	gtk_box_pack_start(GTK_BOX(vbox), show_active, FALSE, FALSE, 0);

	if (find_plugin_by_name("xosd"))
	{
		usexosdfornewmsgs = gtk_check_button_new_with_label(_("Notify about new messages via XOSD"));
		gtk_box_pack_start(GTK_BOX(vbox), usexosdfornewmsgs, FALSE, FALSE, 0);

 		usexosdforstatuschange = gtk_check_button_new_with_label(_("Notify about status change via XOSD"));
 		gtk_box_pack_start(GTK_BOX(vbox), usexosdforstatuschange, FALSE, FALSE, 0);
	}


	show_toolbar = gtk_check_button_new_with_label(_("Show toolbar"));
	gtk_box_pack_start(GTK_BOX(vbox), show_toolbar, FALSE, FALSE, 0);

	descr_on_list = gtk_check_button_new_with_label(_("Display contacts descriptions on users list"));
	gtk_box_pack_start(GTK_BOX(vbox), descr_on_list, FALSE, FALSE, 0);

	tabbox = gtk_table_new(6, 2, FALSE);

	gtk_table_set_row_spacings(GTK_TABLE(tabbox), 7);
	gtk_table_set_col_spacings(GTK_TABLE(tabbox), 5);
	gtk_box_pack_start(GTK_BOX(general_vbox), tabbox, FALSE, FALSE, 0);



	entry = g_object_get_data(G_OBJECT(chat_vbox), "emotic");
	g_return_if_fail(entry != NULL);

	if (ggadu_config_var_get(gui_handler, "emot"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry), TRUE);

	print_debug("baaas? irc_msg_style %d\n", ggadu_config_var_get(gui_handler, "irc_msg_style"));
	print_debug("baaas? %d\n", ggadu_config_var_get(gui_handler, "show_toolbar"));
	print_debug("baaas? active %d\n", ggadu_config_var_get(gui_handler, "show_active"));

	if (ggadu_config_var_get(gui_handler, "show_active"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_active), TRUE);

	if (ggadu_config_var_get(gui_handler, "tree"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tree), TRUE);
	else
		gtk_widget_set_sensitive(expand, FALSE);

	if (ggadu_config_var_get(gui_handler, "expand"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(expand), TRUE);

	entry = g_object_get_data(G_OBJECT(chat_vbox), "chatstyle");
	g_return_if_fail(entry != NULL);

	if ((gint) ggadu_config_var_get(gui_handler, "chat_type") == CHAT_TYPE_TABBED)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry), TRUE);
	else if ((gint) ggadu_config_var_get(gui_handler, "chat_type") == CHAT_TYPE_CLASSIC)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry), FALSE);

	if (gui_check_for_sessions(protocols))
		gtk_widget_set_sensitive(GTK_WIDGET(entry), FALSE);

#ifdef USE_GTKSPELL
	entry = g_object_get_data(G_OBJECT(chat_vbox), "spell");
	g_return_if_fail(entry != NULL);

	if (ggadu_config_var_get(gui_handler, "use_spell"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry), FALSE);

	{
		GtkWidget *entry2 = g_object_get_data(G_OBJECT(chat_vbox), "combo_spell");
		gchar *cur_dict = ggadu_config_var_get(gui_handler, "dictionary");
		GSList *dict_list = NULL;

		AspellConfig *config;
		AspellDictInfoList *dlist;
		AspellDictInfoEnumeration *dels;
		const AspellDictInfo *dict_entry;
		gint i = 0;

		g_return_if_fail(entry2 != NULL);

		config = new_aspell_config();
		/* the returned pointer should _not_ need to be deleted */
		dlist = get_aspell_dict_info_list(config);
		/* config is no longer needed */
		delete_aspell_config(config);

		dels = aspell_dict_info_list_elements(dlist);

		while ((dict_entry = aspell_dict_info_enumeration_next(dels)) != 0)
		{
			gtk_combo_box_append_text(GTK_COMBO_BOX(entry2), dict_entry->name);
			dict_list = g_slist_append(dict_list, (gpointer) dict_entry->name);

			if (!ggadu_strcasecmp(cur_dict, dict_entry->name))
				gtk_combo_box_set_active(GTK_COMBO_BOX(entry2), i);

			i++;
		}

		if (i == 0)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry), FALSE);
			gtk_widget_set_sensitive(entry, FALSE);
		}

		delete_aspell_dict_info_enumeration(dels);
		g_object_set_data(G_OBJECT(entry2), "dictionary_slist", dict_list);
	}
#endif


	entry = g_object_get_data(G_OBJECT(chat_vbox), "chatwindowwidth");
	g_return_if_fail(entry != NULL);

	if (ggadu_config_var_get(gui_handler, "chat_window_width"))
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry), (gint) ggadu_config_var_get(gui_handler, "chat_window_width"));

	entry = g_object_get_data(G_OBJECT(chat_vbox), "chatwindowheight");
	g_return_if_fail(entry != NULL);

	if (ggadu_config_var_get(gui_handler, "chat_window_height"))
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry), (gint) ggadu_config_var_get(gui_handler, "chat_window_height"));

	entry = g_object_get_data(G_OBJECT(chat_vbox), "chatwindowshow");
	g_return_if_fail(entry != NULL);

	if (ggadu_config_var_get(gui_handler, "chat_window_auto_show"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry), TRUE);

	entry = g_object_get_data(G_OBJECT(chat_vbox), "chatwindowraise");
	g_return_if_fail(entry != NULL);

	if (ggadu_config_var_get(gui_handler, "chat_window_auto_raise"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry), TRUE);


	entry = g_object_get_data(G_OBJECT(chat_vbox), "use_username");
	g_return_if_fail(entry != NULL);

	if (ggadu_config_var_get(gui_handler, "use_username"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry), TRUE);

	entry = g_object_get_data(G_OBJECT(chat_vbox), "send_on_enter");
	g_return_if_fail(entry != NULL);

	if (ggadu_config_var_get(gui_handler, "send_on_enter"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry), TRUE);

	if (usexosdfornewmsgs)
		if (ggadu_config_var_get(gui_handler, "use_xosd_for_new_msgs"))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(usexosdfornewmsgs), TRUE);

 	if (usexosdforstatuschange)
 		if (ggadu_config_var_get(gui_handler, "use_xosd_for_status_change"))
 			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(usexosdforstatuschange), TRUE);


	if (ggadu_config_var_get(gui_handler, "show_toolbar"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_toolbar), TRUE);

	if (ggadu_config_var_get(gui_handler, "descr_on_list"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(descr_on_list), TRUE);



//      gtk_window_set_default_size(GTK_WINDOW(preferences), 250, 200);

	gtk_widget_show_all(preferences);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);
	response = gtk_dialog_run(GTK_DIALOG(preferences));
	if (response == GTK_RESPONSE_ACCEPT)
	{
		if (plugins_updated)
		{
			GIOChannel *ch = g_io_channel_new_file(g_build_filename(config->configdir, "modules.load", NULL), "w",
							       NULL);
			gtk_tree_model_foreach(GTK_TREE_MODEL(store), save_selected_plugins, ch);
			g_io_channel_shutdown(ch, TRUE, NULL);
			g_io_channel_unref(ch);
		}
		entry = g_object_get_data(G_OBJECT(chat_vbox), "emotic");
		g_return_if_fail(entry != NULL);

		ggadu_config_var_set(gui_handler, "emot", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));
		ggadu_config_var_set(gui_handler, "show_active", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_active)));
		ggadu_config_var_set(gui_handler, "tree", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tree)));
		ggadu_config_var_set(gui_handler, "expand", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(expand)));

		entry = g_object_get_data(G_OBJECT(chat_vbox), "chatstyle");
		g_return_if_fail(entry != NULL);

		ggadu_config_var_set(gui_handler, "chat_type", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));

		entry = g_object_get_data(G_OBJECT(chat_vbox), "irc_msg_style");
		g_return_if_fail(entry != NULL);

		ggadu_config_var_set(gui_handler, "irc_msg_style", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));

#ifdef USE_GTKSPELL
		entry = g_object_get_data(G_OBJECT(chat_vbox), "spell");
		g_return_if_fail(entry != NULL);

		ggadu_config_var_set(gui_handler, "use_spell", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));

		entry = g_object_get_data(G_OBJECT(chat_vbox), "combo_spell");
		GSList *dict_slist = g_object_get_data(G_OBJECT(entry), "dictionary_slist");

		g_return_if_fail(entry != NULL);

		ggadu_config_var_set(gui_handler, "dictionary", (gpointer) g_strdup(g_slist_nth_data(dict_slist, gtk_combo_box_get_active(GTK_COMBO_BOX(entry)))));

		g_slist_free(dict_slist);
#endif
		entry = g_object_get_data(G_OBJECT(chat_vbox), "chatwindowshow");
		g_return_if_fail(entry != NULL);

		ggadu_config_var_set(gui_handler, "chat_window_auto_show", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));

		entry = g_object_get_data(G_OBJECT(chat_vbox), "chatwindowraise");
		g_return_if_fail(entry != NULL);

		ggadu_config_var_set(gui_handler, "chat_window_auto_raise", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));

		entry = g_object_get_data(G_OBJECT(chat_vbox), "chat_paned_size");
		g_return_if_fail(entry != NULL);

		ggadu_config_var_set(gui_handler, "chat_paned_size", (gpointer) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entry)));


		entry = g_object_get_data(G_OBJECT(chat_vbox), "chatwindowwidth");
		g_return_if_fail(entry != NULL);

		ggadu_config_var_set(gui_handler, "chat_window_width", (gpointer) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entry)));

		entry = g_object_get_data(G_OBJECT(chat_vbox), "chatwindowheight");
		g_return_if_fail(entry != NULL);

		ggadu_config_var_set(gui_handler, "chat_window_height", (gpointer) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entry)));

		entry = g_object_get_data(G_OBJECT(chat_vbox), "use_username");
		g_return_if_fail(entry != NULL);

		ggadu_config_var_set(gui_handler, "use_username", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));


		entry = g_object_get_data(G_OBJECT(adv_vbox), "hide_on_start");
		ggadu_config_var_set(gui_handler, "hide_on_start", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));

		entry = g_object_get_data(G_OBJECT(adv_vbox), "close_on_esc");
		ggadu_config_var_set(gui_handler, "close_on_esc", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));

		entry = g_object_get_data(G_OBJECT(adv_vbox), "blink");
		ggadu_config_var_set(gui_handler, "blink", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));

		entry = g_object_get_data(G_OBJECT(adv_vbox), "blink_interval");
		ggadu_config_var_set(gui_handler, "blink_interval", (gpointer) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entry)));

		entry = g_object_get_data(G_OBJECT(chat_vbox), "send_on_enter");
		g_return_if_fail(entry != NULL);

		ggadu_config_var_set(gui_handler, "send_on_enter", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));

		entry = g_object_get_data(G_OBJECT(adv_vbox), "notify_status_changes");
		g_return_if_fail(entry != NULL);

		ggadu_config_var_set(gui_handler, "notify_status_changes", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));

		if (usexosdfornewmsgs)
			ggadu_config_var_set(gui_handler, "use_xosd_for_new_msgs", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(usexosdfornewmsgs)));
			
 		if (usexosdforstatuschange)
 			ggadu_config_var_set(gui_handler, "use_xosd_for_status_change", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(usexosdforstatuschange)));
			
		ggadu_config_var_set(gui_handler, "show_toolbar", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_toolbar)));

		ggadu_config_var_set(gui_handler, "descr_on_list", (gpointer) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(descr_on_list)));


		entry = g_object_get_data(G_OBJECT(adv_vbox), "combo_theme");
		g_return_if_fail(entry != NULL);

		GSList *combo_theme_slist = g_object_get_data(G_OBJECT(entry), "combo_theme_slist");
		ggadu_config_var_set(gui_handler, "theme", (gpointer) g_strdup(g_slist_nth_data(combo_theme_slist, gtk_combo_box_get_active(GTK_COMBO_BOX(entry)))));

		g_slist_foreach(combo_theme_slist, (GFunc) g_free, NULL);
		g_slist_free(combo_theme_slist);

		entry = g_object_get_data(G_OBJECT(adv_vbox), "combo_skins");
		g_return_if_fail(entry != NULL);

		GSList *combo_skins_slist = g_object_get_data(G_OBJECT(entry), "combo_skins_slist");

		if (gtk_combo_box_get_active(GTK_COMBO_BOX(entry))) {
		    ggadu_config_var_set(gui_handler, "skin", (gpointer) g_strdup(g_slist_nth_data(combo_skins_slist, gtk_combo_box_get_active(GTK_COMBO_BOX(entry)))));
		} else {
		    ggadu_config_var_set(gui_handler, "skin", g_strdup(""));
		}

		g_slist_foreach(combo_skins_slist, (GFunc) g_free, NULL);
		g_slist_free(combo_skins_slist);

		entry = g_object_get_data(G_OBJECT(adv_vbox), "combo_icons");
		g_return_if_fail(entry != NULL);
		GSList *combo_icons_slist = g_object_get_data(G_OBJECT(entry), "combo_icons_slist");
		ggadu_config_var_set(gui_handler, "icons", (gpointer) g_strdup(g_slist_nth_data(combo_icons_slist, gtk_combo_box_get_active(GTK_COMBO_BOX(entry)))));
		g_slist_free(combo_icons_slist);

		entry = g_object_get_data(G_OBJECT(colors_vbox), "msg_header_color");
		g_return_if_fail(entry != NULL);
		ggadu_config_var_set(gui_handler, "msg_header_color", (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));

		entry = g_object_get_data(G_OBJECT(colors_vbox), "msg_body_color");
		g_return_if_fail(entry != NULL);
		ggadu_config_var_set(gui_handler, "msg_body_color", (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));

		entry = g_object_get_data(G_OBJECT(fonts_vbox), "msg_header_font");
		g_return_if_fail(entry != NULL);
		ggadu_config_var_set(gui_handler, "msg_header_font", (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));

		entry = g_object_get_data(G_OBJECT(fonts_vbox), "msg_body_font");
		g_return_if_fail(entry != NULL);
		ggadu_config_var_set(gui_handler, "msg_body_font", (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));

		entry = g_object_get_data(G_OBJECT(colors_vbox), "msg_out_header_color");
		g_return_if_fail(entry != NULL);
		ggadu_config_var_set(gui_handler, "msg_out_header_color", (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));

		entry = g_object_get_data(G_OBJECT(colors_vbox), "msg_out_body_color");
		g_return_if_fail(entry != NULL);
		ggadu_config_var_set(gui_handler, "msg_out_body_color", (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));

		entry = g_object_get_data(G_OBJECT(fonts_vbox), "msg_out_header_font");
		g_return_if_fail(entry != NULL);
		ggadu_config_var_set(gui_handler, "msg_out_header_font", (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));

		entry = g_object_get_data(G_OBJECT(fonts_vbox), "msg_out_body_font");
		g_return_if_fail(entry != NULL);
		ggadu_config_var_set(gui_handler, "msg_out_body_font", (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));

        entry = g_object_get_data(G_OBJECT(fonts_vbox), "msg_out_edit_font");
        g_return_if_fail(entry != NULL);
        ggadu_config_var_set(gui_handler, "msg_out_edit_font", (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));

		entry = g_object_get_data(G_OBJECT(sound_vbox), "sound_msg_out");
		g_return_if_fail(entry != NULL);
		ggadu_config_var_set(gui_handler, "sound_msg_out", (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));

		entry = g_object_get_data(G_OBJECT(sound_vbox), "sound_msg_in");
		g_return_if_fail(entry != NULL);
		ggadu_config_var_set(gui_handler, "sound_msg_in", (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));

		entry = g_object_get_data(G_OBJECT(sound_vbox), "sound_msg_in_first");
		g_return_if_fail(entry != NULL);
		ggadu_config_var_set(gui_handler, "sound_msg_in_first", (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));

		entry = g_object_get_data(G_OBJECT(fonts_vbox), "contact_list_contact_font");
		g_return_if_fail(entry != NULL);
		ggadu_config_var_set(gui_handler, "contact_list_contact_font", (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));

		entry = g_object_get_data(G_OBJECT(fonts_vbox), "contact_list_protocol_font");
		g_return_if_fail(entry != NULL);
		ggadu_config_var_set(gui_handler, "contact_list_protocol_font", (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));

		entry = g_object_get_data(G_OBJECT(adv_vbox), "browser_exec");
		g_return_if_fail(entry != NULL);
		ggadu_config_var_set(gui_handler, "browser_exec", (gpointer) g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));

		ggadu_config_save(gui_handler);

		if (ggadu_strcasecmp(previous_theme, ggadu_config_var_get(gui_handler, "theme")))
			gui_load_theme(NULL);

		gui_config_emoticons();

		if (ggadu_strcasecmp(previous_icons, ggadu_config_var_get(gui_handler, "icons")))
			gui_reload_images();

		if (!ggadu_config_var_get(gui_handler, "show_toolbar"))
		{
			gtk_widget_hide(toolbar_handle_box);
		}
		else
		{
			gtk_widget_show(toolbar_handle_box);
		}

		gui_user_view_refresh();
		gui_chat_update_tags();
	}

	gtk_widget_destroy(preferences);
}
