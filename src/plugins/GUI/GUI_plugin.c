/* $Id: GUI_plugin.c,v 1.107 2005/01/18 14:53:15 krzyzak Exp $ */

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

#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>

#include "ggadu_types.h"
#include "plugins.h"
#include "ggadu_conf.h"
#include "signals.h"
#include "ggadu_support.h"
#include "ggadu_menu.h"
#include "gui_main.h"
#include "GUI_plugin.h"
#include "gui_chat.h"
#include "gui_support.h"
#include "gui_dialogs.h"
#include "gui_userview.h"
#include "gui_preferences.h"
#include "gtkanimlabel.h"

extern GGaduPlugin *gui_handler;

extern GSList *protocols;
extern GSList *emoticons;

extern GtkWidget *window;
extern gboolean tree;

extern GtkItemFactory *item_factory;

extern GSList *invisible_chats;

GtkWidget *main_menu_bar = NULL;
GtkWidget *toolbar_handle_box = NULL;
GtkWidget *status_hbox = NULL;
GtkWidget *view_container = NULL;

/* used with tree user view */
GtkAccelGroup *accel_group = NULL;

typedef struct _GUISkinData {
    gchar *bkg;
    gint w1;
    gint w2; 
    gint w3;
    gint w4;
    gint mx;
    gint my;
    gint cx;
    gint cy;
} GUISkinData;

/* skin stuff */


void set_selected_users_list(GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, gpointer data)
{
	GSList **users = data;
	GGaduContact *k = NULL;

	gtk_tree_model_get(model, iter, 2, &k, -1);

	*users = g_slist_append(*users, k);

}

void nick_list_row_changed2(GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, gpointer user_data)
{
	GtkTreeView *treeview = g_object_get_data(G_OBJECT(user_data), "treeview");

	if (treeview)
	{
		GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
		
		if ((gtk_tree_selection_count_selected_rows(selection) == 1) && 
		    gtk_tree_selection_path_is_selected(selection, path))
//		    gtk_tree_selection_iter_is_selected(selection, iter))
		{
			print_debug("nick_list_row_changed2 selected and changed %s",gtk_tree_path_to_string(path));
			nick_list_row_changed(NULL, model, path, FALSE, user_data);
		}
	}
}

gboolean nick_list_row_changed(GtkTreeSelection * selection, GtkTreeModel * model, GtkTreePath * path, gboolean cur_sel, gpointer user_data)
{
	GtkTreeIter iter;
	gchar *markup_id = NULL;
	gchar *markup_desc = NULL;
	gboolean is_desc = FALSE;
	gchar *desc_text = NULL;
	gchar *ip = NULL;
	GtkTooltips *tooltip = NULL;
	GtkWidget *add_info_label_desc = NULL;
	gui_protocol *gp = NULL;
	gchar *plugin_name = NULL;
	GGaduContact *k = NULL;

	print_debug("nick_list_row_changed");

	gtk_tree_model_get_iter(model, &iter, path);

	if (!tree)
	{
		plugin_name = g_object_get_data(G_OBJECT(user_data), "plugin_name");
		gp = gui_find_protocol(plugin_name, protocols);
	}
	else
	{
		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 3, &gp, -1);
	}

	gtk_tree_model_get(model, &iter, 2, &k, -1);

	if (!gp || !k)
		return FALSE;

	/*print_debug("oiuytrew %s", k->id);*/

	add_info_label_desc = g_object_get_data(G_OBJECT(gp->add_info_label), "add_info_label_desc");

	tooltip = gtk_tooltips_new();

	if (k)
	{
/*		print_debug("k->id = %s %s",prev_id, k->id);
		if (!ggadu_strcasecmp(prev_id,k->id) && (selection != NULL))
		{
		    return TRUE;
		}
		 else
		{
		    g_free(prev_id);
		    prev_id = g_strdup(k->id);
		}
		print_debug("duit");
*/
		if (k->ip)
		{
			gchar **strtab = g_strsplit(k->ip, ":", 2);

			if (!strtab)
				return TRUE;

			switch (atoi(strtab[1]))
			{
			case 1:
				ip = g_strdup_printf("\n[NAT %s]", strtab[0]);
				break;
			case 2:
				ip = g_strdup_printf(_("\n[not in userlist]"));
				break;
			default:
				ip = g_strdup_printf("\n[%s]", strtab[0]);
			}
			g_strfreev(strtab);
		}


		if (k->status_descr)
		{
			gchar *desc_esc = g_markup_escape_text(k->status_descr, strlen(k->status_descr));
			desc_text = g_strdup_printf("%s", desc_esc);
			is_desc = TRUE;
			g_free(desc_esc);
		}

		markup_id = g_strdup_printf("<span size=\"small\">Id: <b>%s</b> %s</span>", k->id, ip ? ip : "");
		markup_desc = (k->status_descr) ? g_strdup_printf("<span size=\"small\">%s</span>", desc_text) : NULL;

		gtk_tooltips_set_tip(tooltip, gtk_widget_get_ancestor(add_info_label_desc, GTK_TYPE_EVENT_BOX), k->status_descr, "caption");
	}
	else
	{
		GGaduStatusPrototype *sp = signal_emit("main-gui", "get current status", NULL, gp->plugin_name);

		print_debug("inside nick_list_row_changed");

		if (sp)
		{
			markup_id = g_strdup_printf("<span size=\"small\"><b>%s</b></span>", gp->p->display_name);
			markup_desc = (sp) ? g_strdup_printf("<span size=\"small\"><b>%s</b></span>", sp->description) : _("(None)");
			is_desc = TRUE;
			gtk_tooltips_set_tip(tooltip, gtk_widget_get_ancestor(add_info_label_desc, GTK_TYPE_EVENT_BOX), NULL, "caption");
		}
		GGaduStatusPrototype_free(sp);
	}


	gtk_tooltips_enable(tooltip);

	gtk_label_set_markup(GTK_LABEL(gp->add_info_label), markup_id);

	/*print_debug("%s", markup_desc);*/

	if (!GTK_WIDGET_VISIBLE(gp->add_info_label))
	{
		gtk_widget_show(gp->add_info_label);
	}


	if (is_desc)
	{
		gtk_anim_label_set_text(GTK_ANIM_LABEL(add_info_label_desc), markup_desc);
		gtk_anim_label_animate(GTK_ANIM_LABEL(add_info_label_desc), TRUE);
		gtk_widget_show(add_info_label_desc);
	}
	else
	{
		gtk_anim_label_animate(GTK_ANIM_LABEL(add_info_label_desc), FALSE);
		gtk_widget_hide(add_info_label_desc);
	}

	g_free(markup_id);
	g_free(markup_desc);
	g_free(desc_text);
	g_free(ip);

	return TRUE;
}

gboolean nick_list_row_activated(GtkWidget * widget, GtkTreePath * arg1, GtkTreeViewColumn * arg2, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	gui_protocol *gp = NULL;
	gchar *plugin_name = NULL;
	GGaduContact *k = NULL;

	print_debug("nick list select albercik");

	gtk_tree_model_get_iter(model, &iter, arg1);
	gtk_tree_model_get(model, &iter, 2, &k, -1);

	g_return_val_if_fail(k != NULL, FALSE);
	if (!k)
		return FALSE;

	if (!tree)
	{
		plugin_name = g_object_get_data(G_OBJECT(user_data), "plugin_name");
		gp = gui_find_protocol(plugin_name, protocols);
	}
	else
	{
		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 3, &gp, -1);
	}

	if (gp)
	{
		GGaduMsg *msg = g_new0(GGaduMsg, 1);
		msg->class = GGADU_CLASS_CHAT;
		msg->id = g_strdup(k->id);
		msg->message = NULL;
		signal_emit_full(gp->plugin_name, "gui msg receive", msg, "main-gui", GGaduMsg_free);
	}

	print_debug("gui-main : clicked : %s : %s\n", k->id, plugin_name);

	return FALSE;
}

gboolean nick_list_pressed(GtkWidget * widget, GdkEventKey * event, gpointer user_data)
{
	return FALSE;
}

gboolean nick_list_clicked(GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
	gui_protocol *gp = NULL;
	gchar *plugin_name = NULL;
	GtkTreeViewColumn *treevc = NULL;
	GtkTreePath *treepath = NULL;
	GSList *selectedusers = NULL;

	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3))
	{
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
		GGaduMenu *umenu = NULL;
		GtkTreeIter iter;
		GtkTreeSelection *selection;

		print_debug("main-gui : wcisnieto prawy klawisz ? %s\n", g_object_get_data(G_OBJECT(user_data), "plugin_name"));

		print_debug("GDK_BUTTON_PRESS 3\n");

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));

		if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), event->x, event->y, &treepath, &treevc, NULL, NULL))
			return FALSE;

		if (!(event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)))
			gtk_tree_selection_unselect_all(selection);

		gtk_tree_selection_select_path(selection, treepath);

		if (!tree)
		{
			plugin_name = g_object_get_data(G_OBJECT(user_data), "plugin_name");
			gp = gui_find_protocol(plugin_name, protocols);
		}
		else
		{
			GGaduContact *k = NULL;

			gtk_tree_model_get_iter(model, &iter, treepath);
			gtk_tree_model_get(model, &iter, 2, &k, -1);

			if (k)
				gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 3, &gp, -1);
		}

		/* count how many rows is selected and set list "selectedusers" */
		gtk_tree_selection_selected_foreach(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)), set_selected_users_list, &selectedusers);

		if (gp && gp->plugin_name && selectedusers)
			umenu = signal_emit("main-gui", "get user menu", selectedusers, gp->plugin_name);


		if (!umenu)
			return FALSE;


		if (selectedusers)
		{
			GtkItemFactory *ifactory = gtk_item_factory_new(GTK_TYPE_MENU, "<name>", NULL);
			gui_produce_menu_for_factory(umenu, ifactory, NULL, selectedusers);
			gtk_item_factory_popup(ifactory, event->x_root, event->y_root, event->button, event->time);
		}

		gtk_tree_path_free(treepath);
		ggadu_menu_free(umenu);

		return TRUE;
	}
	return FALSE;
}

void gui_show_hide_window()
{

	if (!GTK_WIDGET_VISIBLE(window))
		gtk_widget_show(window);
	else
	{
		gint top, left;
		gtk_window_get_position(GTK_WINDOW(window), &left, &top);
		ggadu_config_var_set(gui_handler, "top", (gpointer) top);
		ggadu_config_var_set(gui_handler, "left", (gpointer) left);
		gtk_widget_hide(window);
	}
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 						PRIVATE STUFF
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * Exit, with menu
 */
void gui_quit(GtkWidget * widget, gpointer user_data)
{
	gint width, height;
	gint top, left;

	gtk_window_get_size(GTK_WINDOW(window), &width, &height);

	ggadu_config_var_set(gui_handler, "height", (gpointer) height);
	ggadu_config_var_set(gui_handler, "width", (gpointer) width);

	gtk_window_get_position(GTK_WINDOW(window), &left, &top);

	ggadu_config_var_set(gui_handler, "top", (gpointer) top);
	ggadu_config_var_set(gui_handler, "left", (gpointer) left);

	ggadu_config_save(gui_handler);

	signal_emit("main-gui", "exit", NULL, NULL);
	g_main_loop_quit(config->main_loop);
}

/*
 * Delete main window signal
 */

gboolean gui_main_window_delete(GtkWidget * window, GdkEvent * event, gpointer user_data)
{
	GGaduPlugin *p = (GGaduPlugin *) find_plugin_by_pattern("docklet*");
	print_debug("delete event\n");
	if (!p)
	{
		if (window)
			gui_quit(window, NULL);
	}
	else
	{
		gui_show_hide_window();
	}
	return TRUE;
}


gboolean gui_main_fixed_btn_press(GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data)
{
	/* magic of gtk does it all ;-) */
	gtk_window_begin_move_drag      (GTK_WINDOW(window), event->button,
                                             event->x_root,
                                             event->y_root,
                                             event->time);
	return TRUE;
}

/*
 * Read skin data
 */
 
gboolean gui_read_skin_data(GUISkinData *skin) 
{
	gchar *skindir = NULL;
	gchar *path = NULL;
	FILE *f;
	gchar line[128];
	
	if (!ggadu_config_var_get(gui_handler, "skin"))
	    return FALSE;
	
	if (g_getenv("HOME_ETC"))
		skindir = g_build_filename(g_getenv("HOME_ETC"), "gg2", "skins", ggadu_config_var_get(gui_handler, "skin"), NULL);
	else
		skindir = g_build_filename(g_get_home_dir(), ".gg2", "skins", ggadu_config_var_get(gui_handler, "skin"),NULL);
	
	path = g_build_filename(skindir, "main.txt", NULL);

	print_debug("ridink %s\n", path);
	f = fopen(path, "r");
	
	if (!f)
	{
		print_debug("cannot open main skin file!\n");
		return FALSE;
	}
	
	while (fgets(line, 127, f))
	{
		if (!g_ascii_strncasecmp("BKG", line, 3)) {
			gchar **fields = g_strsplit(line, ",", 7);
			
			print_debug("skin main.txt BKG is: %s\n", fields[1]);
			
			skin->bkg = g_build_filename(skindir, fields[1], NULL);
			
			if (fields[4]) 
			    skin->cx = atoi(fields[4]);
			if (fields[5])
			    skin->cy = atoi(fields[5]);
			g_strfreev(fields);
		} else
		if (!g_ascii_strncasecmp("W", line, 1)) {
			gchar **fields = g_strsplit(line, ",", 5);
			if (fields[1])
			    skin->w1 = atoi(fields[1]);
			if (fields[2])
			    skin->w2 = atoi(fields[2]);
			if (fields[3])
			    skin->w3 = atoi(fields[3]);
			if (fields[4])
			    skin->w4 = atoi(fields[4]);
			g_strfreev(fields);
		} else 
		if (!g_ascii_strncasecmp("B", line, 1)) {
			gchar **fields = g_strsplit(line, ",", 4);
			if (!g_ascii_strncasecmp("MAINMENU", fields[1], 8)) {
			    if (fields[2])
			        skin->mx = atoi(fields[2]);
			    if (fields[3])
	    		        skin->my = atoi(fields[3]);
			}
			g_strfreev(fields);
		}
	}
	
	fclose(f);
	
	g_free(skindir);
	g_free(path);
	
        return TRUE;
}

/*
 * Application window
 */
void gui_main_window_create(gboolean visible)
{
	GtkWidget *main_vbox = NULL;
	GtkWidget *fixed;
	GdkPixbuf *image;
	gint width, height;
	gint top, left;
	GUISkinData *skin = NULL;
	gboolean use_theme;
	GdkPixbuf *skin_bkg_im = NULL;
	
	    	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_name(window, "ggadu_window");
	gtk_window_set_wmclass(GTK_WINDOW(window), "GM_NAME", "GNUGadu");
	gtk_window_set_title(GTK_WINDOW(window), "GNU Gadu");
	gtk_window_set_modal(GTK_WINDOW(window), FALSE);
	/* gtk_window_set_has_frame(GTK_WINDOW(window), FALSE); */
	gtk_window_set_role(GTK_WINDOW(window), "GNUGadu");

	width = (gint) ggadu_config_var_get(gui_handler, "width");
	height = (gint) ggadu_config_var_get(gui_handler, "height");


	if (width <= 0 || width > 3000)
		width = DEFAULT_WIDTH;

	if (height <= 0 || height > 3000)
		height = DEFAULT_HEIGHT;

	gtk_window_set_default_size(GTK_WINDOW(window), width, height);

	top = (gint) ggadu_config_var_get(gui_handler, "top");
	left = (gint) ggadu_config_var_get(gui_handler, "left");
	
	if (top < 0 || top > 3000)
		top = 0;

	if (left < 0 || left > 3000)
		left = 0;

	/* HINT: some window managers may ignore this with default configuration */
	gtk_window_move(GTK_WINDOW(window), left, top);

	image = create_pixbuf(GGADU_DEFAULT_ICON_FILENAME);
	gtk_window_set_icon(GTK_WINDOW(window), image);
	gdk_pixbuf_unref(image);

	main_vbox = gtk_vbox_new(FALSE, 0);
/*
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gui_main_window_destroy), NULL);
*/
	g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(gui_main_window_delete), NULL);

	skin = g_new0(GUISkinData, 1);

	if (ggadu_config_var_get(gui_handler, "skin"))
	    use_theme = gui_read_skin_data(skin);
	
	if (use_theme && skin->bkg)
	    skin_bkg_im = create_pixbuf(skin->bkg);
	
	if (skin_bkg_im) {
		GdkPixbuf *background;
		GdkPixmap *pixmap;
		GdkBitmap *mask;
		GtkWidget *img;
		GtkWidget *evbox;
		GtkWidget *close_button;
		
		evbox = gtk_event_box_new();
		
		close_button = gtk_button_new_with_label("x");
		g_signal_connect_swapped(G_OBJECT(close_button), "clicked", G_CALLBACK(gui_main_window_delete), window);
		gtk_widget_set_size_request(close_button, 10, 10);
		
		background = gdk_pixbuf_add_alpha(skin_bkg_im, TRUE, 255, 0, 255);
		gdk_pixbuf_unref(skin_bkg_im);
		
		width = gdk_pixbuf_get_width(background);
		height = gdk_pixbuf_get_height(background);

		fixed = gtk_fixed_new ();
		gtk_widget_set_size_request (fixed, width, height);
		gtk_container_add (GTK_CONTAINER (window), evbox);
		gtk_container_add (GTK_CONTAINER (evbox), fixed);
	
		gdk_pixbuf_render_pixmap_and_mask(background, &pixmap, &mask, 127 );
	
		img = gtk_image_new_from_pixbuf (background);
		gtk_widget_show (img);
	
		g_signal_connect(G_OBJECT(evbox), "button-press-event", G_CALLBACK(gui_main_fixed_btn_press), NULL);
		
		gtk_fixed_put (GTK_FIXED (fixed), img, 0, 0);
		gtk_fixed_put (GTK_FIXED (fixed), main_menu_bar, skin->mx, skin->my);
		gtk_fixed_put (GTK_FIXED (fixed), close_button, skin->cx, skin->cy);

		gtk_widget_set_size_request (main_vbox, width - skin->w1 + skin->w3, height - skin->w2 + skin->w4);
		gtk_fixed_put (GTK_FIXED (fixed), main_vbox, skin->w1, skin->w2);
		gtk_widget_show(fixed);
		
		gtk_window_set_default_size(GTK_WINDOW(window), width, height);
		gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	
		gtk_widget_shape_combine_mask(window, mask, 0, 0);
	} else {
		gtk_box_pack_start(GTK_BOX(main_vbox), main_menu_bar, FALSE, FALSE, 0);
		gtk_container_add(GTK_CONTAINER(window), main_vbox);
	}

	gtk_box_pack_start(GTK_BOX(main_vbox), toolbar_handle_box, FALSE, FALSE, 0);
	
	view_container = gtk_vbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(main_vbox), view_container, TRUE, TRUE, 0);

	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

	status_hbox = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(main_vbox), status_hbox, FALSE, TRUE, 2);

	gtk_widget_show_all(GTK_WIDGET(main_vbox));

	if (visible)
	{
		if (!use_theme)
		    gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
		gtk_window_set_auto_startup_notification(TRUE);
		gtk_widget_show_all(GTK_WIDGET(window));
	}

	if (!ggadu_config_var_get(gui_handler, "show_toolbar"))
		gtk_widget_hide(toolbar_handle_box);

	if (tree)
		gui_create_tree();
	
	g_free(skin->bkg);
	g_free(skin);
}

gboolean status_blinker(gpointer data)
{
	gui_protocol *gp = (gui_protocol *) data;
	GdkPixbuf *tmp;
	GdkPixbuf *image = NULL;
	GtkWidget *status_image;

	/*print_debug ("status_blinker %p\n", data); */

	if (!data)
	{
		return FALSE;	/* stop blinking */
	}

	/* a little weird algorythm ;) */
	tmp = gp->blinker_image1;
	gp->blinker_image1 = gp->blinker_image2;
	gp->blinker_image2 = tmp;

	image = gp->blinker_image1;
	status_image = gtk_bin_get_child(GTK_BIN(gp->statuslist_eventbox));
	gtk_image_set_from_pixbuf(GTK_IMAGE(status_image), image);

	return TRUE;
}

void change_status(GPtrArray * ptra)
{
	GGaduStatusPrototype *sp = g_ptr_array_index(ptra, 0);
	GGaduStatusPrototype *sp1 = NULL;
	GGaduStatusPrototype *sp2 = NULL;
	gchar *plugin_source = g_ptr_array_index(ptra, 1);
	gui_protocol *gp = NULL;

//	gint status = 0;
	/*    
	 * GtkWidget *status_image = g_ptr_array_index(ptra, 2);
	 * GtkWidget *image = create_image(sp->image);
	 * 
	 * gtk_image_set_from_pixbuf(GTK_IMAGE(status_image), gtk_image_get_pixbuf(GTK_IMAGE(image)));
	 * gtk_widget_destroy(image);
	 */

	gp = gui_find_protocol(plugin_source, protocols);
	if (gp && !ggadu_is_in_status(sp->status, gp->p->offline_status) && ggadu_config_var_get(gui_handler, "blink"))
	{
		gint last_resort_status;

		if (gp->blinker > 0)
			g_source_remove(gp->blinker);

		gp->blinker = -1;

		sp2 = signal_emit("main-gui", "get current status", NULL, gp->plugin_name);

		if (gp->p->offline_status)
			last_resort_status = (gint) gp->p->offline_status->data;
		else
		{
			GGaduStatusPrototype *stmp = (GGaduStatusPrototype *) gp->p->statuslist->data;
			last_resort_status = stmp->status;
		}


		GGaduStatusPrototype_free(sp2);
		sp1 = ggadu_find_status_prototype(gp->p, sp2 ? sp2->status : last_resort_status);

		if (sp1 && sp2 && ggadu_is_in_status(sp2->status, gp->p->offline_status))
		{
			/* *INDENT-OFF* */
			gp->blinker_image1 = create_pixbuf(sp1->image);
			gp->blinker_image2 = create_pixbuf(sp->image);
			gp->blinker = g_timeout_add(ggadu_config_var_get(gui_handler, "blink_interval") ? (gint)
					      ggadu_config_var_get(gui_handler, "blink_interval") : 500, status_blinker,
					      gp);
			print_debug("gui: blinking %s and %s\n", sp1->image, sp->image);
			/* *INDENT-ON* */
		}
		GGaduStatusPrototype_free(sp1);
	}
	else if (ggadu_is_in_status(sp->status, gp->p->offline_status) && gp->blinker > 0)
	{
		g_source_remove(gp->blinker);
		gp->blinker = -1;
	}

	if (sp)
	{
	    g_free(sp->status_description);
	    sp->status_description = NULL;
	    signal_emit("main-gui", "change status", sp, plugin_source);
	}
	/* ZONK */
	/* te tablice "ptra" kiedys tam mozna zwolic, ale nie mozna tutaj */
}


GtkWidget *create_status_menu(gui_protocol * gp, GtkWidget * status_image)
{
	GSList *statuslist = gp->p->statuslist;
	GtkWidget *menu = gtk_menu_new();
	GtkWidget *item = NULL;

	while (statuslist)
	{
		GPtrArray *ptra = NULL;
		GGaduStatusPrototype *sp = statuslist->data;

		if (!sp->receive_only)
		{
			item = gtk_image_menu_item_new_with_label(sp->description);
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), create_image(sp->image));
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

			ptra = g_ptr_array_new();

			g_ptr_array_add(ptra, sp);
			g_ptr_array_add(ptra, gp->plugin_name);
			g_ptr_array_add(ptra, status_image);

			g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(change_status), ptra);

			gtk_widget_show_all(item);
		}

		statuslist = statuslist->next;
	}

	return menu;
}

/*
 * Buduje standardowe menu
 */
void gui_build_default_menu()
{
	GtkItemFactoryEntry menu_items[] = {
		{"/_GNU Gadu", NULL, NULL, 0, "<Branch>", NULL},
		{_("/GNU Gadu/_Preferences"), NULL, gui_preferences, 0, "<StockItem>", GTK_STOCK_PREFERENCES},
		{_("/GNU Gadu/"), NULL, NULL, 0, "<Separator>", NULL},
		{_("/GNU Gadu/_About"), "<CTRL>a", gui_about, 0, "<StockItem>", GTK_STOCK_DIALOG_INFO},
		{_("/GNU Gadu/_Quit"), "<CTRL>q", gui_quit, 0, "<StockItem>", GTK_STOCK_QUIT},
		{"/_Menu", NULL, NULL, 0, "<Branch>", NULL},
	};
	gint Nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

	accel_group = gtk_accel_group_new();

	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);

	gtk_item_factory_create_items(item_factory, Nmenu_items, menu_items, NULL);

	main_menu_bar = gtk_item_factory_get_widget(item_factory, "<main>");
}

gpointer show_hide_inactive(GtkWidget * widget, gpointer user_data)
{
	gtk_widget_set_sensitive(toolbar_handle_box, FALSE);

	if (ggadu_config_var_get(gui_handler, "show_active"))
	{
		ggadu_config_var_set(gui_handler, "show_active", (gpointer) FALSE);
	}
	else
	{
		ggadu_config_var_set(gui_handler, "show_active", (gpointer) TRUE);
	}

	ggadu_config_save(gui_handler);

	gui_user_view_refresh();

	gtk_widget_set_sensitive(toolbar_handle_box, TRUE);

	return NULL;
}

gpointer show_hide_descriptions(GtkWidget * widget, gpointer user_data)
{
	gtk_widget_set_sensitive(toolbar_handle_box, FALSE);

	if (ggadu_config_var_get(gui_handler, "descr_on_list"))
	{
		ggadu_config_var_set(gui_handler, "descr_on_list", (gpointer) FALSE);
	}
	else
	{
		ggadu_config_var_set(gui_handler, "descr_on_list", (gpointer) TRUE);
	}

	ggadu_config_save(gui_handler);

	gui_user_view_refresh();

	gtk_widget_set_sensitive(toolbar_handle_box, TRUE);

	return NULL;
}


void gui_build_default_toolbar()
{
	GtkWidget *main_toolbar = gtk_toolbar_new();
	toolbar_handle_box = gtk_handle_box_new();

	gtk_container_add(GTK_CONTAINER(toolbar_handle_box), main_toolbar);

	gtk_toolbar_set_style(GTK_TOOLBAR(main_toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(main_toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);

	gtk_toolbar_insert_stock(GTK_TOOLBAR(main_toolbar), GTK_STOCK_PREFERENCES, _("Open preferences window"), NULL, (GtkSignalFunc) gui_preferences, NULL, 0);
	gtk_toolbar_insert_stock(GTK_TOOLBAR(main_toolbar), GTK_STOCK_QUIT, _("Exit program"), NULL, (GtkSignalFunc) gui_quit, NULL, 0);

	gtk_toolbar_append_element(GTK_TOOLBAR(main_toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL, NULL,
				   _("Show/hide inactive users"), NULL, create_image("show-hide-inactive.png"), (GtkSignalFunc) show_hide_inactive, NULL);

	gtk_toolbar_append_element(GTK_TOOLBAR(main_toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL, NULL,
				   _("Show/hide descriptions on userlist"), NULL, create_image("show-hide-descriptions.png"), (GtkSignalFunc) show_hide_descriptions, NULL);
}

/*
 * SIGNAL CALLBACK FUNCTIONS
 */
void gui_msg_receive(GGaduSignal * signal)
{
	GGaduMsg *msg = signal->data;
	gui_protocol *gp = NULL;
	gchar *soundfile = NULL;

	if (!signal || !msg)
	{
		print_debug("main-gui : gui_msg_receive : ((msg == NULL) || (msg->id == NULL) || (signal == NULL) - return !!!!");
		return;
	}

	gp = gui_find_protocol(signal->source_plugin_name, protocols);

	print_debug("%s : %s -> %s | %s", "gui-main", msg->id, msg->message, signal->source_plugin_name);

	if (gp)
	{
		gui_chat_session *session = NULL;
		GSList *sigdata = NULL;
		gchar  *sigdata_desc = NULL;
		gboolean showwindow = ggadu_config_var_get(gui_handler, "chat_window_auto_show") ? TRUE : FALSE;
		GGaduContact *k = gui_find_user(msg->id, gp);

		if (msg->class == GGADU_CLASS_CONFERENCE)
			session = gui_session_find_confer(gp, msg->recipients);
		else
			session = gui_session_find(gp, msg->id);

		if (!session)
		{
			session = g_new0(gui_chat_session, 1);
			session->id = g_strdup(msg->id);
			gp->chat_sessions = g_slist_append(gp->chat_sessions, session);
		}

		if (msg->message == NULL)
			showwindow = TRUE;

		/* shasta: new "docklet set [default] icon" approach */
		sigdata = g_slist_append(sigdata, (gchar *) ggadu_config_var_get(gui_handler, "icons"));
		sigdata = g_slist_append(sigdata, (gpointer) GGADU_MSG_ICON_FILENAME);
		sigdata_desc = g_strdup_printf(_("New message from %s"), (k ? k->nick : msg->id));
		sigdata = g_slist_append(sigdata, sigdata_desc);

		if (!session->chat)
		{
			if ((showwindow == FALSE) && (find_plugin_by_pattern("docklet-*")))
			{
				signal_emit_full("main-gui", "docklet set icon", sigdata, NULL, (gpointer) g_slist_free);
			}
			else
			{
				g_slist_free(sigdata);
				showwindow = TRUE;
			}

			if (msg->message && (soundfile = ggadu_config_var_get(gui_handler, "sound_msg_in_first")))
				signal_emit_full("main-gui", "sound play file", soundfile, "sound*", NULL);

			session->recipients = g_slist_copy(msg->recipients);
			session->chat = create_chat(session, gp->plugin_name, msg->id, showwindow);
		}
		else
		{
			GtkWidget *window1 = NULL;

			if (msg->message && (soundfile = ggadu_config_var_get(gui_handler, "sound_msg_in")))
				signal_emit_full("main-gui", "sound play file", soundfile, "sound*", NULL);

			window1 = gtk_widget_get_ancestor(session->chat, GTK_TYPE_WINDOW);

			if (!GTK_WIDGET_VISIBLE(window1))
			{
				if (showwindow)
				{
					/* TODO: find this session in invisible_chats and remove */
					invisible_chats = g_slist_remove(invisible_chats, session->chat);
					if (g_slist_length(invisible_chats) < 1)
					{
						GSList *icondatalist = NULL;
						icondatalist = g_slist_append(icondatalist, (gchar *) ggadu_config_var_get(gui_handler, "icons"));
						icondatalist = g_slist_append(icondatalist, (gpointer) GGADU_DEFAULT_ICON_FILENAME);
						icondatalist = g_slist_append(icondatalist, _("GNU Gadu"));
						signal_emit_full("main-gui", "docklet set icon", icondatalist, NULL, (gpointer) g_slist_free);
					}
					gtk_widget_show_all(window1);

					print_debug("showwindow");
				}
				else if (msg->message && find_plugin_by_pattern("docklet-*"))
				{
					invisible_chats = g_slist_append(invisible_chats, session->chat);
					signal_emit_full("main-gui", "docklet set icon", sigdata, NULL, (gpointer) g_slist_free);
				}
				else if (msg->message)
				{
					gtk_widget_show_all(window1);
					print_debug("msg->message");
				}

			}
			else
			{
				g_slist_free(sigdata);
			}

			if(ggadu_config_var_get(gui_handler, "close_on_esc"))
			{
				if((gint) ggadu_config_var_get(gui_handler, "chat_type") == CHAT_TYPE_CLASSIC)
				g_signal_connect(window1, "key-press-event",
					G_CALLBACK(on_key_press_event_chat_window), (gpointer) CHAT_TYPE_CLASSIC);
			} else
			{
				if((gint) ggadu_config_var_get(gui_handler, "chat_type") == CHAT_TYPE_CLASSIC)
				g_signal_handlers_disconnect_by_func(window1, G_CALLBACK(on_key_press_event_chat_window),
					(gpointer) CHAT_TYPE_CLASSIC);
			}
		}

		if ((gint) ggadu_config_var_get(gui_handler, "use_xosd_for_new_msgs") == TRUE && find_plugin_by_name("xosd") && msg->message)
		{
			signal_emit("main-gui", "xosd show message", g_strdup_printf(_("New message from %s"), (k ? k->nick : msg->id)), "xosd");
		}

		g_free(sigdata_desc);
		gui_chat_append(session->chat, msg, FALSE, FALSE);
	}
}

void gui_produce_menu_for_factory(GGaduMenu * menu, GtkItemFactory * item_factory, gchar * root, gpointer user_data)
{
	GGaduMenu *node = menu;
	GGaduMenu *child = NULL;
	gchar *prev_root = NULL;

	if (G_NODE_IS_ROOT(node))
		node = g_node_first_child(node);
	else
		node = g_node_first_sibling(node);

	while (node)
	{
		GtkItemFactoryEntry *e = g_new0(GtkItemFactoryEntry, 1);
		GGaduMenuItem *it = node->data;

		if (g_node_first_child(node) != NULL)
		{
			e->item_type = g_strdup("<Branch>");
			e->callback = NULL;
		}
		else
		{
			e->item_type = g_strdup("<Item>");
			e->callback = it->callback;	/* to sa rozne callbacki ale function_ptr dobrze sie sprawuje jako it->callback */
			e->extra_data = it->data;	/* bezsensu */
		}

		prev_root = root;

		if (root)
			e->path = g_strdup_printf("%s/%s", root, it->label);
		else
			e->path = g_strdup_printf("/%s", it->label);

		print_debug("%s  %s\n", e->item_type, e->path);
		gtk_item_factory_create_item(item_factory, e, user_data, 1);

		if ((child = g_node_first_child(node)) != NULL)
		{
			root = e->path;
			gui_produce_menu_for_factory(child, item_factory, root, user_data);
			root = prev_root;
		}

		node = node->next;

		g_free(e);
	}

}

GSList *gui_read_emoticons(gchar * path)
{
	GIOChannel *ch = NULL;
	GSList *emotlist = NULL;
	GString *line = g_string_new("");
	gchar *stmp = NULL;
	gchar **emot = NULL;

	print_debug("read emoticons from %s", path);
	ch = g_io_channel_new_file(path, "r", NULL);

	if (!ch)
		return NULL;

	g_io_channel_set_encoding(ch, NULL, NULL);

	while (g_io_channel_read_line_string(ch, line, NULL, NULL) == G_IO_STATUS_NORMAL)
	{
		stmp = line->str;
		emot = array_make(stmp, "\t", 2, 1, 1);

		if (emot)
		{
			if (emot[1])
			{
				gui_emoticon *gemo = g_new0(gui_emoticon, 1);
				gemo->emoticon = emot[0];
				gemo->file = g_strstrip(emot[1]);
				emotlist = g_slist_append(emotlist, gemo);
			}
		}

	}

	g_string_free(line, TRUE);
	g_io_channel_shutdown(ch, TRUE, NULL);
	g_io_channel_unref(ch);

	return emotlist;
}

void gui_config_emoticons()
{

	if (ggadu_config_var_get(gui_handler, "emot"))
	{
		gchar *path;

		/* read from ~/.gg2/emoticons.def */
		path = g_build_filename(config->configdir, "emoticons.def", NULL);
		emoticons = gui_read_emoticons(path);
		g_free(path);

		/* if reading from ~/.gg2/emoticons.def failed, try default one */
		if (!emoticons)
		{
			path = g_build_filename(PACKAGE_DATA_DIR, "pixmaps", G_DIR_SEPARATOR_S, "emoticons", "emoticons.def", NULL);
			emoticons = gui_read_emoticons(path);
			g_free(path);
		}
	}
	else
	{
		GSList *tmplist = emoticons;

		while (tmplist)
		{
			gui_emoticon *gemo = tmplist->data;
			g_free(gemo->emoticon);
			g_free(gemo->file);
			g_free(gemo);
			tmplist = tmplist->next;
		}

		g_slist_free(emoticons);
		emoticons = NULL;
	}
}

void gui_load_theme()
{
	gchar *themepath = NULL;
	gchar *themefilename = NULL;

	themefilename = g_strconcat(ggadu_config_var_get(gui_handler, "theme") ? ggadu_config_var_get(gui_handler, "theme") : "", ".theme", NULL);
	themepath = g_build_filename(PACKAGE_DATA_DIR, "themes", themefilename, NULL);

	print_debug("%s : Loading theme from %s\n", "main-gui", themepath);

	if (ggadu_strcasecmp(ggadu_config_var_get(gui_handler, "theme"), "default"))
	{
		gtk_rc_parse(themepath);
		gtk_rc_reset_styles(gtk_settings_get_default());
	}
	else
	{
		gtk_rc_parse(themepath);
		gtk_rc_reset_styles(gtk_settings_get_default());
	}

	g_free(themepath);
	g_free(themefilename);
}

void gui_reload_images()
{
	GSList *sigdata = NULL;

/*
    gui_user_view_refresh();
*/

	sigdata = g_slist_append(sigdata, (gchar *) ggadu_config_var_get(gui_handler, "icons"));
	sigdata = g_slist_append(sigdata, GGADU_DEFAULT_ICON_FILENAME);
	sigdata = g_slist_append(sigdata, _("GNU Gadu"));

	signal_emit_full("main-gui", "docklet set default icon", sigdata, NULL, (gpointer) g_slist_free);
}
