/* $Id: gui_preferences.c,v 1.26 2003/11/06 20:03:27 thrulliq Exp $ */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>
#include "support.h"
#include "signals.h"
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
extern GGaduConfig *config;

static gboolean save_selected_plugins (GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, gpointer data)
{
    gboolean enable;
    gchar *name = NULL;
    gsize count;

    gtk_tree_model_get (model, iter, PLUGINS_MGR_ENABLE, &enable, -1);
    gtk_tree_model_get (model, iter, PLUGINS_MGR_NAME, &name, -1);

    if (enable)
      {
	  GIOChannel *ch = (GIOChannel *) data;
	  GSList *all_plugins = config->all_available_plugins;

	  if (!ch)
	      return TRUE;

	  g_io_channel_write_chars (ch, name, -1, &count, NULL);
	  g_io_channel_write_chars (ch, "\n", -1, &count, NULL);

	  while (all_plugins)
	    {
		GGaduPluginFile *pf = (GGaduPluginFile *) all_plugins->data;

		if (!ggadu_strcasecmp (pf->name, name) && (!find_plugin_by_name (name)))
		  {
		      load_plugin (pf->path);
		  }

		all_plugins = all_plugins->next;
	    }

      }
    else if (name != NULL)
      {
	  if (!ggadu_strcasecmp (name, "main-gui"))
	    {
		signal_emit_full ("main-gui", "gui show warning",
				  g_strdup ("ZONK! Bo zaraz zdechne! Zrestartuj gada, by zmiany odniosly efekt."),
				  "main-gui", NULL);
		return FALSE;
	    }
	  unload_plugin (name);
      }

    return FALSE;
}

static void enable_toggled (GtkCellRendererToggle * cell, gchar * path_str, gpointer data)
{
    GtkTreeIter iter;
    gboolean enable;
    GtkTreeModel *model = (GtkTreeModel *) data;
    GtkTreePath *path = gtk_tree_path_new_from_string (path_str);

    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, PLUGINS_MGR_ENABLE, &enable, -1);

    enable ^= 1;

    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, PLUGINS_MGR_ENABLE, enable, -1);

    gtk_tree_path_free (path);

}

GtkWidget *gui_plugins_mgr_tab ()
{
    GtkWidget *vbox;
    GSList *plugins_list = (config) ? config->all_available_plugins : NULL;
    GSList *modules_load = (config) ? get_list_modules_load () : NULL;
    GtkTreeIter iter;
    GtkCellRenderer *renderer = NULL;
    GtkTreeViewColumn *column = NULL;

    vbox = gtk_vbox_new (FALSE, 0);
    store = gtk_tree_store_new (PLUGINS_MGR_COUNT, G_TYPE_STRING, G_TYPE_BOOLEAN);

/* zaladowane pluginy na poczzatku wedle kolejnosci zadanej w pliku modules.load */
    while (modules_load)
      {
	  gboolean tmpvar = FALSE;
	  GGaduPlugin *pl = (GGaduPlugin *) modules_load->data;

	  if (find_plugin_by_name ((gchar *) pl->name))
	      tmpvar = TRUE;

	  print_debug ("%s\n", pl->name);
	  gtk_tree_store_append (GTK_TREE_STORE (store), &iter, NULL);
	  gtk_tree_store_set (GTK_TREE_STORE (store), &iter, PLUGINS_MGR_NAME, (gchar *) pl->name, PLUGINS_MGR_ENABLE,
			      tmpvar, -1);

	  modules_load = modules_load->next;
      }

/* pozostale */
    while (plugins_list)
      {
	  gboolean tmpvar = FALSE;
	  GGaduPluginFile *pf = (GGaduPluginFile *) plugins_list->data;

	  if (!find_plugin_by_name ((gchar *) pf->name))
	    {
		tmpvar = FALSE;

		print_debug ("%s\n", pf->name);
		gtk_tree_store_append (GTK_TREE_STORE (store), &iter, NULL);
		gtk_tree_store_set (GTK_TREE_STORE (store), &iter, PLUGINS_MGR_NAME, (gchar *) pf->name,
				    PLUGINS_MGR_ENABLE, tmpvar, -1);
	    }
	  plugins_list = plugins_list->next;
      }

    list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (list), TRUE);

    g_object_unref (G_OBJECT (store));

    renderer = gtk_cell_renderer_toggle_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Enable"), renderer, "active", PLUGINS_MGR_ENABLE, NULL);

    gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

    g_signal_connect (renderer, "toggled", G_CALLBACK (enable_toggled), store);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Plugin name"), renderer, "text", PLUGINS_MGR_NAME, NULL);

    gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (list), TRUE, TRUE, 0);

    gtk_widget_show_all (vbox);

    return vbox;
}

static void tree_toggled (GtkWidget * tree, gpointer data)
{
    GtkWidget *expand = data;

    gtk_widget_set_sensitive (expand, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tree)));
}

static void show_colors_select_dialog (GtkWidget * widget, gpointer user_data)
{
    GtkWidget *entry = (GtkWidget *) user_data;
    GtkWidget *color_selector;
    gint response;
    const gchar *color_txt;
    GdkColor color;

    color_selector = gtk_color_selection_dialog_new (_("Select color"));

    color_txt = gtk_entry_get_text (GTK_ENTRY (entry));
    gdk_color_parse (color_txt, &color);
    gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (color_selector)->colorsel),
					   &color);

    response = gtk_dialog_run (GTK_DIALOG (color_selector));

    if (response == GTK_RESPONSE_OK)
      {
	  gchar *tmp;

	  gtk_color_selection_get_current_color (GTK_COLOR_SELECTION
						 (GTK_COLOR_SELECTION_DIALOG (color_selector)->colorsel), &color);
	  tmp = gtk_color_selection_palette_to_string (&color, 1);
	  gtk_entry_set_text (GTK_ENTRY (entry), tmp);
	  g_free (tmp);
      }

    gtk_widget_destroy (color_selector);
}

static GtkWidget *create_colors_tab ()
{
    GtkWidget *colors_vbox;
    GtkWidget *image;
    GtkWidget *label;
    GtkWidget *hbox;
    GtkWidget *tabbox;
    GtkWidget *entry;
    GtkWidget *button;
    GtkWidget *frame;

    colors_vbox = gtk_vbox_new (FALSE, 2);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (colors_vbox), hbox, FALSE, FALSE, 0);

    image = gtk_image_new ();
    gtk_image_set_from_stock (GTK_IMAGE (image), "gtk-select-color", GTK_ICON_SIZE_DND);
    label = gtk_label_new (_("\nColors settings\n\n"));

    gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    frame = gtk_frame_new (_("Incoming messages"));
    gtk_box_pack_start (GTK_BOX (colors_vbox), frame, FALSE, FALSE, 0);

    tabbox = gtk_table_new (3, 3, FALSE);
    gtk_container_add (GTK_CONTAINER (frame), tabbox);

    label = gtk_label_new (_("Message header"));
    entry = gtk_entry_new ();
    button = gtk_button_new_from_stock ("gtk-select-color");

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), entry, 1, 2, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), button, 2, 3, 0, 1);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (show_colors_select_dialog), entry);
    g_object_set_data (G_OBJECT (colors_vbox), "msg_header_color", entry);

    if (config_var_get (gui_handler, "msg_header_color"))
	gtk_entry_set_text (GTK_ENTRY (entry), (gchar *) config_var_get (gui_handler, "msg_header_color"));

    label = gtk_label_new (_("Message body"));
    entry = gtk_entry_new ();
    button = gtk_button_new_from_stock ("gtk-select-color");

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), entry, 1, 2, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), button, 2, 3, 1, 2);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (show_colors_select_dialog), entry);
    g_object_set_data (G_OBJECT (colors_vbox), "msg_body_color", entry);

    if (config_var_get (gui_handler, "msg_body_color"))
	gtk_entry_set_text (GTK_ENTRY (entry), (gchar *) config_var_get (gui_handler, "msg_body_color"));

    frame = gtk_frame_new (_("Outgoing messages"));
    gtk_box_pack_start (GTK_BOX (colors_vbox), frame, FALSE, FALSE, 0);

    tabbox = gtk_table_new (3, 3, FALSE);
    gtk_container_add (GTK_CONTAINER (frame), tabbox);

    label = gtk_label_new (_("Message header"));
    entry = gtk_entry_new ();
    button = gtk_button_new_from_stock ("gtk-select-color");

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), entry, 1, 2, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), button, 2, 3, 0, 1);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (show_colors_select_dialog), entry);
    g_object_set_data (G_OBJECT (colors_vbox), "msg_out_header_color", entry);

    if (config_var_get (gui_handler, "msg_out_header_color"))
	gtk_entry_set_text (GTK_ENTRY (entry), (gchar *) config_var_get (gui_handler, "msg_out_header_color"));

    label = gtk_label_new (_("Message body"));
    entry = gtk_entry_new ();
    button = gtk_button_new_from_stock ("gtk-select-color");

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), entry, 1, 2, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), button, 2, 3, 1, 2);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (show_colors_select_dialog), entry);
    g_object_set_data (G_OBJECT (colors_vbox), "msg_out_body_color", entry);

    if (config_var_get (gui_handler, "msg_out_body_color"))
	gtk_entry_set_text (GTK_ENTRY (entry), (gchar *) config_var_get (gui_handler, "msg_out_body_color"));

    return colors_vbox;
}

static void show_fonts_select_dialog (GtkWidget * widget, gpointer user_data)
{
    GtkWidget *entry = (GtkWidget *) user_data;
    GtkWidget *font_selector;
    gint response;
    const gchar *font_txt;

    font_selector = gtk_font_selection_dialog_new (_("Select font"));

    font_txt = gtk_entry_get_text (GTK_ENTRY (entry));
    if (font_txt && *font_txt)
	gtk_font_selection_set_font_name (GTK_FONT_SELECTION (GTK_FONT_SELECTION_DIALOG (font_selector)->fontsel),
					  font_txt);

    response = gtk_dialog_run (GTK_DIALOG (font_selector));

    if (response == GTK_RESPONSE_OK)
      {
	  gchar *tmp;

	  tmp =
	      gtk_font_selection_get_font_name (GTK_FONT_SELECTION
						(GTK_FONT_SELECTION_DIALOG (font_selector)->fontsel));
	  gtk_entry_set_text (GTK_ENTRY (entry), tmp);
	  g_free (tmp);
      }

    gtk_widget_destroy (font_selector);
}

static GtkWidget *create_fonts_tab ()
{
    GtkWidget *fonts_vbox;
    GtkWidget *image;
    GtkWidget *label;
    GtkWidget *hbox;
    GtkWidget *tabbox;
    GtkWidget *entry;
    GtkWidget *button;
    GtkWidget *frame;

    fonts_vbox = gtk_vbox_new (FALSE, 2);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (fonts_vbox), hbox, FALSE, FALSE, 0);

    image = gtk_image_new ();
    gtk_image_set_from_stock (GTK_IMAGE (image), "gtk-select-font", GTK_ICON_SIZE_DND);
    label = gtk_label_new (_("\nFonts settings\n\n"));

    gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    frame = gtk_frame_new (_("Incoming messages"));
    gtk_box_pack_start (GTK_BOX (fonts_vbox), frame, FALSE, FALSE, 0);

    tabbox = gtk_table_new (3, 3, FALSE);
    gtk_container_add (GTK_CONTAINER (frame), tabbox);

    label = gtk_label_new (_("Message header"));
    entry = gtk_entry_new ();
    button = gtk_button_new_from_stock ("gtk-select-font");

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), entry, 1, 2, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), button, 2, 3, 0, 1);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (show_fonts_select_dialog), entry);
    g_object_set_data (G_OBJECT (fonts_vbox), "msg_header_font", entry);

    if (config_var_get (gui_handler, "msg_header_font"))
	gtk_entry_set_text (GTK_ENTRY (entry), (gchar *) config_var_get (gui_handler, "msg_header_font"));

    label = gtk_label_new (_("Message body"));
    entry = gtk_entry_new ();
    button = gtk_button_new_from_stock ("gtk-select-font");

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), entry, 1, 2, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), button, 2, 3, 1, 2);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (show_fonts_select_dialog), entry);
    g_object_set_data (G_OBJECT (fonts_vbox), "msg_body_font", entry);

    if (config_var_get (gui_handler, "msg_body_font"))
	gtk_entry_set_text (GTK_ENTRY (entry), (gchar *) config_var_get (gui_handler, "msg_body_font"));

    frame = gtk_frame_new (_("Outgoing messages"));
    gtk_box_pack_start (GTK_BOX (fonts_vbox), frame, FALSE, FALSE, 0);

    tabbox = gtk_table_new (3, 3, FALSE);
    gtk_container_add (GTK_CONTAINER (frame), tabbox);

    label = gtk_label_new (_("Message header"));
    entry = gtk_entry_new ();
    button = gtk_button_new_from_stock ("gtk-select-font");

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), entry, 1, 2, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), button, 2, 3, 0, 1);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (show_fonts_select_dialog), entry);
    g_object_set_data (G_OBJECT (fonts_vbox), "msg_out_header_font", entry);

    if (config_var_get (gui_handler, "msg_out_header_font"))
	gtk_entry_set_text (GTK_ENTRY (entry), (gchar *) config_var_get (gui_handler, "msg_out_header_font"));

    label = gtk_label_new (_("Message body"));
    entry = gtk_entry_new ();
    button = gtk_button_new_from_stock ("gtk-select-font");

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), entry, 1, 2, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), button, 2, 3, 1, 2);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (show_fonts_select_dialog), entry);
    g_object_set_data (G_OBJECT (fonts_vbox), "msg_out_body_font", entry);

    if (config_var_get (gui_handler, "msg_out_body_font"))
	gtk_entry_set_text (GTK_ENTRY (entry), (gchar *) config_var_get (gui_handler, "msg_out_body_font"));

    frame = gtk_frame_new (_("Contact list"));
    gtk_box_pack_start (GTK_BOX (fonts_vbox), frame, FALSE, FALSE, 0);

    tabbox = gtk_table_new (3, 3, FALSE);
    gtk_container_add (GTK_CONTAINER (frame), tabbox);

    label = gtk_label_new (_("Contact"));
    entry = gtk_entry_new ();
    button = gtk_button_new_from_stock ("gtk-select-font");

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), entry, 1, 2, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), button, 2, 3, 0, 1);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (show_fonts_select_dialog), entry);
    g_object_set_data (G_OBJECT (fonts_vbox), "contact_list_contact_font", entry);

    if (config_var_get (gui_handler, "contact_list_contact_font"))
	gtk_entry_set_text (GTK_ENTRY (entry), (gchar *) config_var_get (gui_handler, "contact_list_contact_font"));

    label = gtk_label_new (_("Protocol (tree view)"));
    entry = gtk_entry_new ();
    button = gtk_button_new_from_stock ("gtk-select-font");

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), entry, 1, 2, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), button, 2, 3, 1, 2);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (show_fonts_select_dialog), entry);
    g_object_set_data (G_OBJECT (fonts_vbox), "contact_list_protocol_font", entry);

    if (config_var_get (gui_handler, "contact_list_protocol_font"))
	gtk_entry_set_text (GTK_ENTRY (entry), (gchar *) config_var_get (gui_handler, "contact_list_protocol_font"));

    return fonts_vbox;
}

static void show_file_select_dialog (GtkWidget * widget, gpointer user_data)
{
    GtkWidget *entry = (GtkWidget *) user_data;
    GtkWidget *file_selector = NULL;
    gchar *filename = NULL;
    gint response;
    const gchar *_filename;

    file_selector = gtk_file_selection_new (_("Select file"));
    _filename = gtk_entry_get_text (GTK_ENTRY (entry));
    if (_filename && *_filename)
	gtk_file_selection_set_filename (GTK_FILE_SELECTION (file_selector), _filename);

    response = gtk_dialog_run (GTK_DIALOG (file_selector));

    if (response == GTK_RESPONSE_OK)
      {
	  filename = (gchar *) gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_selector));
	  gtk_entry_set_text (GTK_ENTRY (entry), filename);
      }

    gtk_widget_destroy (file_selector);
}

static GtkWidget *create_sound_tab ()
{
    GtkWidget *sound_vbox;
    GtkWidget *image;
    GtkWidget *label;
    GtkWidget *hbox;
    GtkWidget *tabbox;
    GtkWidget *entry;
    GtkWidget *button;
    GtkWidget *frame;

    sound_vbox = gtk_vbox_new (FALSE, 2);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (sound_vbox), hbox, FALSE, FALSE, 0);

    image = gtk_image_new ();
    gtk_image_set_from_stock (GTK_IMAGE (image), "gtk-stock-cdrom", GTK_ICON_SIZE_DND);
    label = gtk_label_new (_("\nSound settings\n\n"));

    gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    frame = gtk_frame_new (_("Messages"));
    gtk_box_pack_start (GTK_BOX (sound_vbox), frame, FALSE, FALSE, 0);

    tabbox = gtk_table_new (3, 3, FALSE);
    gtk_container_add (GTK_CONTAINER (frame), tabbox);

    label = gtk_label_new (_("Incoming message:"));
    entry = gtk_entry_new ();
    button = gtk_button_new_from_stock ("gtk-open");

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), entry, 1, 2, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), button, 2, 3, 0, 1);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (show_file_select_dialog), entry);
    g_object_set_data (G_OBJECT (sound_vbox), "sound_msg_in", entry);

    if (config_var_get (gui_handler, "sound_msg_in"))
	gtk_entry_set_text (GTK_ENTRY (entry), (gchar *) config_var_get (gui_handler, "sound_msg_in"));

    label = gtk_label_new (_("Outgoing message:"));
    entry = gtk_entry_new ();
    button = gtk_button_new_from_stock ("gtk-open");

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), entry, 1, 2, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), button, 2, 3, 1, 2);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (show_file_select_dialog), entry);
    g_object_set_data (G_OBJECT (sound_vbox), "sound_msg_out", entry);

    if (config_var_get (gui_handler, "sound_msg_out"))
	gtk_entry_set_text (GTK_ENTRY (entry), (gchar *) config_var_get (gui_handler, "sound_msg_out"));

    return sound_vbox;
}

void gui_preferences (GtkWidget * widget, gpointer data)
{
    GtkWidget *preferences;
    GtkWidget *notebook;
    GtkWidget *image;
    GtkWidget *general_vbox;
    GtkWidget *sound_vbox;
    GtkWidget *colors_vbox;
    GtkWidget *fonts_vbox;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *emotic;
    GtkWidget *show_active;
    GtkWidget *tree;
    GtkWidget *expand;
    GtkWidget *chatstyle;
    GtkWidget *chatwindowshow;
    GtkWidget *chatwindowraise;
    GtkWidget *usexosdfornewmsgs;
    GtkWidget *send_on_enter;
    GtkWidget *hide_on_start;
    GtkWidget *hide_toolbar;
    GtkWidget *blink;
    GtkWidget *blink_interval;
    GtkWidget *auto_away;
    GtkWidget *auto_away_interval;
    GtkWidget *chat_paned_size;
    GtkWidget *tabbox;
    GDir *dir;
    GtkWidget *combo_theme;
    GtkWidget *combo_icons;
    GdkPixbuf *windowicon = NULL;
    gint response = -1;
    gchar *dirname;

    print_debug ("Preferences\n");

    preferences =
	gtk_dialog_new_with_buttons (_("Preferences"), NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				     GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);

    gtk_window_set_resizable (GTK_WINDOW (preferences), FALSE);

    if ((windowicon = create_pixbuf (GGADU_DEFAULT_ICON_FILENAME)) != NULL)
      {
	  gtk_window_set_icon (GTK_WINDOW (preferences), windowicon);
	  gdk_pixbuf_unref (windowicon);
      }

    notebook = gtk_notebook_new ();
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (preferences)->vbox), notebook);

    general_vbox = gtk_vbox_new (FALSE, 4);
    sound_vbox = create_sound_tab ();
    colors_vbox = create_colors_tab ();
    fonts_vbox = create_fonts_tab ();

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), general_vbox, gtk_label_new (_("General")));

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), sound_vbox, gtk_label_new (_("Sound")));

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), colors_vbox, gtk_label_new (_("Colors")));

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), fonts_vbox, gtk_label_new (_("Fonts")));

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), gui_plugins_mgr_tab (), gtk_label_new (_("Plugins manager")));

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (general_vbox), hbox, FALSE, FALSE, 0);

    image = gtk_image_new ();
    gtk_image_set_from_stock (GTK_IMAGE (image), "gtk-preferences", GTK_ICON_SIZE_DND);
    label = gtk_label_new (_("\nGUI plugin preferences\n\n"));

    gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (general_vbox), vbox, FALSE, FALSE, 0);

    emotic = gtk_check_button_new_with_label (_("Enable emoticons"));
    gtk_box_pack_start (GTK_BOX (vbox), emotic, FALSE, FALSE, 0);

    show_active = gtk_check_button_new_with_label (_("Show only active users"));
    gtk_box_pack_start (GTK_BOX (vbox), show_active, FALSE, FALSE, 0);

    tree = gtk_check_button_new_with_label (_("Tree user list"));
    gtk_box_pack_start (GTK_BOX (vbox), tree, FALSE, FALSE, 0);

    expand = gtk_check_button_new_with_label (_("Expand Tree by default"));
    gtk_box_pack_start (GTK_BOX (vbox), expand, FALSE, FALSE, 0);

    g_signal_connect (tree, "toggled", G_CALLBACK (tree_toggled), expand);

    chatstyle = gtk_check_button_new_with_label (_("Tabbed chat window style"));
    gtk_box_pack_start (GTK_BOX (vbox), chatstyle, FALSE, FALSE, 0);

    chatwindowshow = gtk_check_button_new_with_label (_("Automatically show chat window"));
    gtk_box_pack_start (GTK_BOX (vbox), chatwindowshow, FALSE, FALSE, 0);

    chatwindowraise = gtk_check_button_new_with_label (_("Automatically raise chat window"));
    gtk_box_pack_start (GTK_BOX (vbox), chatwindowraise, FALSE, FALSE, 0);

    send_on_enter = gtk_check_button_new_with_label (_("Send messages after 'Enter' button pressed"));
    gtk_box_pack_start (GTK_BOX (vbox), send_on_enter, FALSE, FALSE, 0);

    usexosdfornewmsgs = gtk_check_button_new_with_label (_("Notify about new messages via XOSD"));
    gtk_box_pack_start (GTK_BOX (vbox), usexosdfornewmsgs, FALSE, FALSE, 0);

    hide_on_start = gtk_check_button_new_with_label (_("Auto hide on start"));
    gtk_box_pack_start (GTK_BOX (vbox), hide_on_start, FALSE, FALSE, 0);

    hide_toolbar = gtk_check_button_new_with_label (_("Hide toolbar"));
    gtk_box_pack_start (GTK_BOX (vbox), hide_toolbar, FALSE, FALSE, 0);

    blink = gtk_check_button_new_with_label (_("Blink status"));
    gtk_box_pack_start (GTK_BOX (vbox), blink, FALSE, FALSE, 0);

    auto_away = gtk_check_button_new_with_label (_("Auto away"));
    gtk_box_pack_start (GTK_BOX (vbox), auto_away, FALSE, FALSE, 0);

    tabbox = gtk_table_new (5, 2, FALSE);
    gtk_box_pack_start (GTK_BOX (general_vbox), tabbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Blink interval"));
    blink_interval = gtk_spin_button_new_with_range (0, 2000, 100);

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), blink_interval, 1, 2, 0, 1);
    g_signal_connect (blink, "toggled", G_CALLBACK (tree_toggled), blink_interval);

    /* ZONK - how to name it ? */
    label = gtk_label_new (_("Chat window split size (%)"));
    chat_paned_size = gtk_spin_button_new_with_range (0, 100, 5);

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), chat_paned_size, 1, 2, 1, 2);

    combo_theme = gtk_combo_new ();
    label = gtk_label_new (_("Select Theme"));

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 2, 3);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), combo_theme, 1, 2, 2, 3);

    combo_icons = gtk_combo_new ();
    label = gtk_label_new (_("Select icon set"));

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 3, 4);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), combo_icons, 1, 2, 3, 4);

    label = gtk_label_new (_("Auto away interval (minutes)"));
    auto_away_interval = gtk_spin_button_new_with_range (0, 1440, 1);

    gtk_table_attach_defaults (GTK_TABLE (tabbox), label, 0, 1, 4, 5);
    gtk_table_attach_defaults (GTK_TABLE (tabbox), auto_away_interval, 1, 2, 4, 5);
    g_signal_connect (auto_away, "toggled", G_CALLBACK (tree_toggled), auto_away_interval);

    if (config_var_get (gui_handler, "emot"))
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (emotic), TRUE);

    if (config_var_get (gui_handler, "show_active"))
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_active), TRUE);

    if (config_var_get (gui_handler, "tree"))
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tree), TRUE);
    else
	gtk_widget_set_sensitive (expand, FALSE);


    if (config_var_get (gui_handler, "expand"))
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (expand), TRUE);
	
    if ((gint) config_var_get (gui_handler, "chat_type") == CHAT_TYPE_TABBED)
      {
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chatstyle), TRUE);
      }
    else if ((gint) config_var_get (gui_handler, "chat_type") == CHAT_TYPE_CLASSIC)
      {
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chatstyle), FALSE);
      }

    if (config_var_get (gui_handler, "blink"))
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (blink), TRUE);
    else
	gtk_widget_set_sensitive (blink_interval, FALSE);

    if (config_var_get (gui_handler, "blink_interval"))
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (blink_interval),
				   (gint) config_var_get (gui_handler, "blink_interval"));

    if (config_var_get (gui_handler, "auto_away"))
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (auto_away), TRUE);
    else
	gtk_widget_set_sensitive (auto_away_interval, FALSE);

    if (config_var_get (gui_handler, "auto_away_interval"))
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (auto_away_interval),
				   (gint) config_var_get (gui_handler, "auto_away_interval"));

    if (config_var_get (gui_handler, "chat_window_auto_show"))
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chatwindowshow), TRUE);

    if (config_var_get (gui_handler, "chat_window_auto_raise"))
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chatwindowraise), TRUE);

    if (config_var_get (gui_handler, "chat_paned_size"))
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (chat_paned_size),
				   (gint) config_var_get (gui_handler, "chat_paned_size"));

    if (config_var_get (gui_handler, "send_on_enter"))
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (send_on_enter), TRUE);

    if (config_var_get (gui_handler, "use_xosd_for_new_msgs"))
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (usexosdfornewmsgs), TRUE);
	
    if (config_var_get (gui_handler, "hide_on_start"))
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (hide_on_start), TRUE);

    if (config_var_get (gui_handler, "hide_toolbar"))
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (hide_toolbar), TRUE);

    /* read available themes */
    dirname = g_build_filename (PACKAGE_DATA_DIR, "themes", NULL);
    print_debug ("tryin to read themes directory %s\n", dirname);
    dir = g_dir_open (dirname, 0, NULL);
    g_free (dirname);

    if (dir)
      {
	  GList *list_theme = NULL;
	  gchar *theme_current;
	  gchar *theme_name_file;
	  gchar *theme_name;

	  theme_current = config_var_get (gui_handler, "theme");
	  list_theme = g_list_append (list_theme, theme_current);

	  while ((theme_name_file = (gchar *) g_dir_read_name (dir)) != NULL)
	    {
		print_debug ("%s\n", theme_name_file);

		if (g_str_has_suffix (theme_name_file, ".theme"))
		  {
		      theme_name = g_strndup (theme_name_file, strlen (theme_name_file) - strlen (".theme"));
		      if (!theme_current || ggadu_strcasecmp (theme_name, theme_current))
			  list_theme = g_list_append (list_theme, g_strdup (theme_name));
		      g_free (theme_name);
		  }
	    }

	  g_dir_close (dir);
	  gtk_combo_set_popdown_strings (GTK_COMBO (combo_theme), list_theme);
      }

    dirname = g_build_filename (PACKAGE_DATA_DIR, "pixmaps", "icons", NULL);
    print_debug ("tryin to read icons directory %s\n", dirname);
    dir = g_dir_open (dirname, 0, NULL);
    g_free (dirname);

    if (dir)
      {
	  GList *list_icons = NULL;
	  gchar *icons_current;
	  gchar *icons_dir;

	  icons_current = config_var_get (gui_handler, "icons");
	  list_icons = g_list_append (list_icons, icons_current);

	  while ((icons_dir = (gchar *) g_dir_read_name (dir)) != NULL)
	    {
		gchar *testdirname = g_build_filename (PACKAGE_DATA_DIR, "pixmaps", "icons", icons_dir, NULL);

		if (g_file_test (testdirname, G_FILE_TEST_IS_DIR))
		  {
		      print_debug ("%s\n", icons_dir);
		      if (!icons_current || ggadu_strcasecmp (icons_dir, icons_current))
			  list_icons = g_list_append (list_icons, g_strdup (icons_dir));
		  }
		g_free (testdirname);
	    }

	  g_dir_close (dir);
	  gtk_combo_set_popdown_strings (GTK_COMBO (combo_icons), list_icons);
      }

    gtk_window_set_default_size (GTK_WINDOW (preferences), 250, 200);

    gtk_widget_show_all (preferences);

    gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);
    response = gtk_dialog_run (GTK_DIALOG (preferences));
    if (response == GTK_RESPONSE_ACCEPT)
      {
	  GtkWidget *entry;
	  GIOChannel *ch =
	      g_io_channel_new_file (g_build_filename (config->configdir, "modules.load", NULL), "w", NULL);

	  gtk_tree_model_foreach (GTK_TREE_MODEL (store), save_selected_plugins, ch);

	  g_io_channel_shutdown (ch, TRUE, NULL);

	  config_var_set (gui_handler, "emot", (gpointer) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (emotic)));

	  config_var_set (gui_handler, "show_active",
			  (gpointer) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (show_active)));

	  config_var_set (gui_handler, "tree", (gpointer) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tree)));

	  config_var_set (gui_handler, "expand", (gpointer) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (expand)));

	  config_var_set (gui_handler, "chat_type",
			  (gpointer) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (chatstyle)));

	  config_var_set (gui_handler, "chat_window_auto_show",
			  (gpointer) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (chatwindowshow)));

	  config_var_set (gui_handler, "chat_window_auto_raise",
			  (gpointer) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (chatwindowraise)));

	  config_var_set (gui_handler, "chat_paned_size",
			  (gpointer) gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (chat_paned_size)));

	  config_var_set (gui_handler, "blink", (gpointer) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (blink)));

	  config_var_set (gui_handler, "blink_interval",
			  (gpointer) gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (blink_interval)));

	  config_var_set (gui_handler, "auto_away",
			  (gpointer) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (auto_away)));

	  config_var_set (gui_handler, "auto_away_interval",
			  (gpointer) gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (auto_away_interval)));

	  {
	      GSList *list = protocols;
	      while (list)
		{
		    gui_protocol *gp = (gui_protocol *) list->data;
		    auto_away_start (gp);
		    list = list->next;
		}
	  }

	  config_var_set (gui_handler, "send_on_enter",
			  (gpointer) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (send_on_enter)));

	  config_var_set (gui_handler, "use_xosd_for_new_msgs",
			  (gpointer) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (usexosdfornewmsgs)));

	  config_var_set (gui_handler, "hide_on_start",
			  (gpointer) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (hide_on_start)));

	  config_var_set (gui_handler, "hide_toolbar",
			  (gpointer) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (hide_toolbar)));

	  config_var_set (gui_handler, "theme",
			  (gpointer) g_strdup (gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (combo_theme)->entry))));
	  config_var_set (gui_handler, "icons",
			  (gpointer) g_strdup (gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (combo_icons)->entry))));

	  entry = g_object_get_data (G_OBJECT (colors_vbox), "msg_header_color");
	  g_return_if_fail (entry != NULL);
	  config_var_set (gui_handler, "msg_header_color",
			  (gpointer) g_strdup (gtk_entry_get_text (GTK_ENTRY (entry))));

	  entry = g_object_get_data (G_OBJECT (colors_vbox), "msg_body_color");
	  g_return_if_fail (entry != NULL);
	  config_var_set (gui_handler, "msg_body_color", (gpointer) g_strdup (gtk_entry_get_text (GTK_ENTRY (entry))));

	  entry = g_object_get_data (G_OBJECT (fonts_vbox), "msg_header_font");
	  g_return_if_fail (entry != NULL);
	  config_var_set (gui_handler, "msg_header_font", (gpointer) g_strdup (gtk_entry_get_text (GTK_ENTRY (entry))));

	  entry = g_object_get_data (G_OBJECT (fonts_vbox), "msg_body_font");
	  g_return_if_fail (entry != NULL);
	  config_var_set (gui_handler, "msg_body_font", (gpointer) g_strdup (gtk_entry_get_text (GTK_ENTRY (entry))));

	  entry = g_object_get_data (G_OBJECT (colors_vbox), "msg_out_header_color");
	  g_return_if_fail (entry != NULL);
	  config_var_set (gui_handler, "msg_out_header_color",
			  (gpointer) g_strdup (gtk_entry_get_text (GTK_ENTRY (entry))));

	  entry = g_object_get_data (G_OBJECT (colors_vbox), "msg_out_body_color");
	  g_return_if_fail (entry != NULL);
	  config_var_set (gui_handler, "msg_out_body_color",
			  (gpointer) g_strdup (gtk_entry_get_text (GTK_ENTRY (entry))));

	  entry = g_object_get_data (G_OBJECT (fonts_vbox), "msg_out_header_font");
	  g_return_if_fail (entry != NULL);
	  config_var_set (gui_handler, "msg_out_header_font",
			  (gpointer) g_strdup (gtk_entry_get_text (GTK_ENTRY (entry))));

	  entry = g_object_get_data (G_OBJECT (fonts_vbox), "msg_out_body_font");
	  g_return_if_fail (entry != NULL);
	  config_var_set (gui_handler, "msg_out_body_font",
			  (gpointer) g_strdup (gtk_entry_get_text (GTK_ENTRY (entry))));

	  entry = g_object_get_data (G_OBJECT (sound_vbox), "sound_msg_out");
	  g_return_if_fail (entry != NULL);
	  config_var_set (gui_handler, "sound_msg_out", (gpointer) g_strdup (gtk_entry_get_text (GTK_ENTRY (entry))));

	  entry = g_object_get_data (G_OBJECT (sound_vbox), "sound_msg_in");
	  g_return_if_fail (entry != NULL);
	  config_var_set (gui_handler, "sound_msg_in", (gpointer) g_strdup (gtk_entry_get_text (GTK_ENTRY (entry))));

	  entry = g_object_get_data (G_OBJECT (fonts_vbox), "contact_list_contact_font");
	  g_return_if_fail (entry != NULL);
	  config_var_set (gui_handler, "contact_list_contact_font",
			  (gpointer) g_strdup (gtk_entry_get_text (GTK_ENTRY (entry))));

	  entry = g_object_get_data (G_OBJECT (fonts_vbox), "contact_list_protocol_font");
	  g_return_if_fail (entry != NULL);
	  config_var_set (gui_handler, "contact_list_protocol_font",
			  (gpointer) g_strdup (gtk_entry_get_text (GTK_ENTRY (entry))));

	  config_save (gui_handler);
	  gui_load_theme ();
	  gui_config_emoticons ();
	  gui_reload_images ();
	  gui_user_view_refresh ();
	  gui_chat_update_tags ();
      }

    gtk_widget_destroy (preferences);
}
