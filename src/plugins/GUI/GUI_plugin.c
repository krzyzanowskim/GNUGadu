/* $Id: GUI_plugin.c,v 1.69 2004/04/09 21:19:03 thrulliq Exp $ */

/*
 * GUI (gtk+) plugin for GNU Gadu 2
 *
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>

#include "gg-types.h"
#include "unified-types.h"
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
GtkWidget *main_toolbar = NULL;
GtkWidget *toolbar_handle_box;
GtkWidget *status_hbox = NULL;
GtkWidget *view_container = NULL;

/* used with tree user view */
GtkAccelGroup *accel_group = NULL;

void set_selected_users_list(GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, gpointer data)
{
	GSList **users = data;
	GGaduContact *k = NULL;

	gtk_tree_model_get(model, iter, 2, &k, -1);

	*users = g_slist_append(*users, k);

}

gboolean nick_list_row_changed(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean cur_sel, gpointer user_data)
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

	add_info_label_desc = g_object_get_data(G_OBJECT(gp->add_info_label), "add_info_label_desc");

	tooltip = gtk_tooltips_new();

	if (k)
	{
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

			markup_id =
				g_strdup_printf("<span size=\"small\">Id: <b>%s</b> %s</span>", k->id, ip ? ip : "");
			markup_desc =
				(k->status_descr) ? g_strdup_printf("<span size=\"small\">%s</span>", desc_text) : NULL;

			gtk_tooltips_set_tip(tooltip, gtk_widget_get_ancestor(add_info_label_desc, GTK_TYPE_EVENT_BOX),
					     k->status_descr, "caption");
		}
		else
		{
			GGaduStatusPrototype *sp;
			gint status;

			status = (gint) signal_emit("main-gui", "get current status", NULL, gp->plugin_name);
			sp = gui_find_status_prototype(gp->p, (gint) status);

			if (sp)
			{
				markup_id =
					g_strdup_printf("<span size=\"small\"><b>%s</b></span>", gp->p->display_name);
				markup_desc =
					(sp) ? g_strdup_printf("<span size=\"small\"><b>%s</b></span>",
							       sp->description) : _("(None)");
				is_desc = TRUE;
				gtk_tooltips_set_tip(tooltip,
						     gtk_widget_get_ancestor(add_info_label_desc, GTK_TYPE_EVENT_BOX),
						     NULL, "caption");
			}
		}


	gtk_tooltips_enable(tooltip);

	gtk_label_set_markup(GTK_LABEL(gp->add_info_label), markup_id);

	gtk_anim_label_set_text(GTK_ANIM_LABEL(add_info_label_desc), markup_desc);
	gtk_anim_label_animate(GTK_ANIM_LABEL(add_info_label_desc), TRUE);

	if (!GTK_WIDGET_VISIBLE(gp->add_info_label))
	{
		gtk_widget_show(gp->add_info_label);
	}

	if (is_desc)
	{
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

gboolean nick_list_row_activated(GtkWidget * widget, GtkTreePath *arg1, GtkTreeViewColumn *arg2, gpointer user_data)
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

	if (!tree)
	{
		plugin_name = g_object_get_data(G_OBJECT(user_data), "plugin_name");
		gp = gui_find_protocol(plugin_name, protocols);
	}
	else
	{
		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 3, &gp, -1);
	}

	if (gp != NULL)
	{
		gui_chat_session *session = gui_session_find(gp, k->id);
		if (!session)
		{
			session = g_new0(gui_chat_session, 1);
			session->id = g_strdup(k->id);
			gp->chat_sessions = g_slist_append(gp->chat_sessions, session);
		}

		if (!session->chat)
			session->chat = create_chat(session, gp->plugin_name, k->id, TRUE);
		else {
		    GtkWidget *window = gtk_widget_get_ancestor(session->chat, GTK_TYPE_WINDOW);
		    if (!GTK_WIDGET_VISIBLE(window))
			gtk_widget_show(window);
		}

		gui_chat_append(session->chat, NULL, TRUE);
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


	/* count how many rows is selected and set list "selectedusers" */
	gtk_tree_selection_selected_foreach(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)), set_selected_users_list,
					    &selectedusers);

	if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
	{
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
		GGaduContact *k = NULL;
		GtkTreeIter iter;

		if (!gtk_tree_view_get_path_at_pos
		    (GTK_TREE_VIEW(widget), event->x, event->y, &treepath, &treevc, NULL, NULL))
			return FALSE;

		print_debug("GDK_2BUTTON_PRESS");
		gtk_tree_model_get_iter(model, &iter, treepath);
		gtk_tree_model_get(model, &iter, 2, &k, -1);

		g_return_val_if_fail(k != NULL, FALSE);

		if (!tree)
		{
			plugin_name = g_object_get_data(G_OBJECT(user_data), "plugin_name");
			gp = gui_find_protocol(plugin_name, protocols);
		}
		else
		{
			gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 3, &gp, -1);
		}

		if (gp != NULL)
		{
			gui_chat_session *session = gui_session_find(gp, k->id);

			if (!session)
			{
				session = g_new0(gui_chat_session, 1);
				session->id = g_strdup(k->id);
				gp->chat_sessions = g_slist_append(gp->chat_sessions, session);
			}

			if (!session->chat)
				session->chat = create_chat(session, gp->plugin_name, k->id, TRUE);
			else {
			    GtkWidget *window = gtk_widget_get_ancestor(session->chat, GTK_TYPE_WINDOW);
			    if (!GTK_WIDGET_VISIBLE(window))
				gtk_widget_show(window);
			}

			gui_chat_append(session->chat, NULL, TRUE);
		}

		print_debug("gui-main : clicked : %s : %s\n", k->id, plugin_name);

		gtk_tree_path_free(treepath);
	}
	
	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3))
	{
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
		GtkItemFactory *ifactory = NULL;
		GGaduMenu *umenu = NULL;
		GtkTreeIter iter;

		print_debug("main-gui : wcisnieto prawy klawisz ? %s\n",
			    g_object_get_data(G_OBJECT(user_data), "plugin_name"));

		print_debug("GDK_BUTTON_PRESS 3\n");

		if (!gtk_tree_view_get_path_at_pos
		    (GTK_TREE_VIEW(widget), event->x, event->y, &treepath, &treevc, NULL, NULL))
			return FALSE;


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

		if (gp && gp->plugin_name && selectedusers)
			umenu = signal_emit("main-gui", "get user menu", selectedusers, gp->plugin_name);

		if (!umenu)
			return FALSE;

		ifactory = gtk_item_factory_new(GTK_TYPE_MENU, "<name>", NULL);

		if (selectedusers)
		{
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


/*
 * Application window
 */
void gui_main_window_create(gboolean visible)
{
	GtkWidget *main_vbox = NULL;
	GdkPixbuf *image;
	gint width, height;
	gint top, left;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_name(window, "ggadu_window");
	gtk_window_set_wmclass(GTK_WINDOW(window), "GM_NAME", "GNUGadu");
	gtk_window_set_title(GTK_WINDOW(window), "GNU Gadu 2");
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

	gtk_box_pack_start(GTK_BOX(main_vbox), main_menu_bar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(main_vbox), toolbar_handle_box, FALSE, FALSE, 0);

/*
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gui_main_window_destroy), NULL);
*/
	g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(gui_main_window_delete), NULL);

	gtk_container_add(GTK_CONTAINER(window), main_vbox);

	view_container = gtk_vbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(main_vbox), view_container, TRUE, TRUE, 0);

	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

	status_hbox = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(main_vbox), status_hbox, FALSE, TRUE, 2);

	gtk_widget_show_all(GTK_WIDGET(main_vbox));

	if (visible)
	{
		gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
		gtk_window_set_auto_startup_notification(TRUE);
		gtk_widget_show_all(GTK_WIDGET(window));
	}

	if (!ggadu_config_var_get(gui_handler, "show_toolbar"))
		gtk_widget_hide(toolbar_handle_box);

	if (tree)
		gui_create_tree();
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
	gchar *plugin_source = g_ptr_array_index(ptra, 1);
	gui_protocol *gp = NULL;
	GGaduStatusPrototype *sp1 = NULL;

	gint status = 0;
	/*    
	 * GtkWidget *status_image = g_ptr_array_index(ptra, 2);
	 * GtkWidget *image = create_image(sp->image);
	 * 
	 * gtk_image_set_from_pixbuf(GTK_IMAGE(status_image), gtk_image_get_pixbuf(GTK_IMAGE(image)));
	 * gtk_widget_destroy(image);
	 */

	gp = gui_find_protocol(plugin_source, protocols);
	if (gp && !is_in_status(sp->status, gp->p->offline_status) && ggadu_config_var_get(gui_handler, "blink"))
	{
		gint last_resort_status;
		gp->aaway_timer = -1;

		if (gp->blinker > 0)
			g_source_remove(gp->blinker);
		
		gp->blinker = -1;

		status = (gint) signal_emit("main-gui", "get current status", NULL, gp->plugin_name);
		
		if (gp->p->offline_status)
			last_resort_status = (gint)gp->p->offline_status->data;
		else {
			GGaduStatusPrototype *stmp = (GGaduStatusPrototype *)gp->p->statuslist->data;
			last_resort_status = stmp->status;
		}
		
		print_debug("requested status ID : %d, last_resort_status : %d\n", status, last_resort_status);
		
		sp1 = gui_find_status_prototype(gp->p, status != 0 ? status : last_resort_status);
		/* sp1 = gui_find_status_prototype(gp->p, status != 0 ? status : *(int *) &last_resort_status); */
		if (sp1 != NULL && is_in_status(status, gp->p->offline_status))
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
	}
	else if (is_in_status(sp->status, gp->p->offline_status) && gp->blinker > 0)
	{
		g_source_remove(gp->blinker);
		gp->blinker = -1;
	}

	signal_emit("main-gui", "change status", sp, plugin_source);
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
	toolbar_handle_box = gtk_handle_box_new();
	main_toolbar = gtk_toolbar_new();

	gtk_container_add(GTK_CONTAINER(toolbar_handle_box), main_toolbar);

	gtk_toolbar_set_style(GTK_TOOLBAR(main_toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(main_toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);

	gtk_toolbar_insert_stock(GTK_TOOLBAR(main_toolbar), GTK_STOCK_PREFERENCES, _("Open preferences window"), NULL,
				 (GtkSignalFunc) gui_preferences, NULL, 0);
	gtk_toolbar_insert_stock(GTK_TOOLBAR(main_toolbar), GTK_STOCK_QUIT, _("Exit program"), NULL,
				 (GtkSignalFunc) gui_quit, NULL, 0);

	gtk_toolbar_append_element(GTK_TOOLBAR(main_toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL, NULL,
				   _("Show/hide inactive users"), NULL, create_image("show-hide-inactive.png"),
				   (GtkSignalFunc) show_hide_inactive, NULL);

	gtk_toolbar_append_element(GTK_TOOLBAR(main_toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL, NULL,
				   _("Show/hide descriptions on userlist"), NULL, create_image("show-hide-descriptions.png"),
				   (GtkSignalFunc) show_hide_descriptions, NULL);
}

/*
 * SIGNAL CALLBACK FUNCTIONS
 */
void gui_msg_receive(GGaduSignal * signal)
{
	GGaduMsg *msg = signal->data;
	gui_protocol *gp = NULL;
	gchar *soundfile = NULL;

	if (!signal || !msg || !msg)
	{
		print_debug("main-gui : gui_msg_receive : ((msg == NULL) || (msg->id == NULL) || (signal == NULL) - return !!!!");
		return;
	}

	gp = gui_find_protocol(signal->source_plugin_name, protocols);

	print_debug("%s : %s -> %s | %s", "gui-main", msg->id, msg->message, signal->source_plugin_name);

	if (msg->message && (soundfile = ggadu_config_var_get(gui_handler, "sound_msg_in")))
		signal_emit_full("main-gui", "sound play file", soundfile, "sound*", NULL);

	if (gp != NULL)
	{
		gui_chat_session *session = NULL;
		gboolean showwindow = ggadu_config_var_get(gui_handler, "chat_window_auto_show") ? TRUE : FALSE;
		GSList *sigdata = NULL;
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
		sigdata =
			g_slist_append(sigdata,
					       g_strdup_printf(_("New message from %s"), (k ? k->nick : msg->id)));
		if (!session->chat)
		{
			if (showwindow == FALSE)
			{
			    if (find_plugin_by_pattern("docklet-*"))
				signal_emit_full("main-gui", "docklet set icon", sigdata, NULL, (gpointer) g_slist_free);
			/*    	    showwindow = FALSE;
			    else
				    showwindow = TRUE;
			*/
			} else {
				g_slist_free(sigdata);
			}

			session->recipients = g_slist_copy(msg->recipients);
			session->chat = create_chat(session, gp->plugin_name, msg->id, showwindow);
		} else {
			GtkWidget *window = gtk_widget_get_ancestor(session->chat, GTK_TYPE_WINDOW);
		        if (!GTK_WIDGET_VISIBLE(window)) {
				if (showwindow) {
				    GtkWidget *input = g_object_get_data(G_OBJECT(session->chat), "input");
				    //if (ggadu_config_var_get(gui_handler, "chat_window_auto_raise"))
				    //	gtk_widget_grab_focus(input);
				    gtk_widget_show_all(window);
				} else if (msg->message && find_plugin_by_pattern("docklet-*")) {
    				    invisible_chats = g_slist_append(invisible_chats, session->chat);
				    signal_emit_full("main-gui", "docklet set icon", sigdata, NULL, (gpointer) g_slist_free);
				}
			} else {
				g_slist_free(sigdata);
			}
		}

		if ((gint) ggadu_config_var_get(gui_handler, "use_xosd_for_new_msgs") == TRUE &&
		    find_plugin_by_name("xosd") && msg->message)
		{
			signal_emit("main-gui", "xosd show message",
				    g_strdup_printf(_("New message from %s"), (k ? k->nick : msg->id)), "xosd");
		}

		gui_chat_append(session->chat, msg, FALSE);
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
			path = g_build_filename(PACKAGE_DATA_DIR, "pixmaps", G_DIR_SEPARATOR_S, "emoticons",
						"emoticons.def", NULL);
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
	gtk_rc_parse(themepath);
	gtk_widget_reset_rc_styles(GTK_WIDGET(window));
	gtk_rc_reparse_all();
	g_free(themepath);
	g_free(themefilename);
}

void gui_reload_images()
{
	GSList *sigdata = NULL;

	/* Shouldn't be here but anyway... */
	if (!ggadu_config_var_get(gui_handler, "show_toolbar")) {
		gtk_widget_hide(toolbar_handle_box);
	} else {
		gtk_widget_show(toolbar_handle_box);
	}
/*
    gui_user_view_refresh();
*/

	sigdata = g_slist_append(sigdata, (gchar *) ggadu_config_var_get(gui_handler, "icons"));
	sigdata = g_slist_append(sigdata, GGADU_DEFAULT_ICON_FILENAME);
	sigdata = g_slist_append(sigdata, "GNU Gadu 2");

	signal_emit_full("main-gui", "docklet set default icon", sigdata, NULL, (gpointer) g_slist_free);
}
