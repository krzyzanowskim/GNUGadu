/* $Id: gui_search.c,v 1.10 2005/01/02 00:13:15 krzyzak Exp $ */

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

#include <gtk/gtk.h>
#include "ggadu_types.h"
#include "ggadu_support.h"
#include "ggadu_menu.h"
#include "signals.h"
#include "plugins.h"

#include "gui_main.h"
#include "gui_support.h"
#include "GUI_plugin.h"


extern GSList *protocols;

static void add_columns(GtkTreeView * tv)
{
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;

    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new_with_attributes(_("Status"), renderer, "pixbuf", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Id"), renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("User"), renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("City"), renderer, "text", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Age"), renderer, "text", 5, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), column);

}

void on_destroy_search(GtkWidget * search, gpointer user_data)
{
    GSList *tmplist = user_data;

    print_debug("freeing search data\n");

    while (tmplist)
    {
	GGaduContact *k = tmplist->data;
	GGaduContact_free(k);
	tmplist = tmplist->next;
    }
    g_slist_free(user_data);
    gtk_widget_destroy(search);
}

gpointer search_user_add(gpointer user_data)
{
    GtkWidget *widget = user_data;
    GGaduContact *tmp, *k = g_object_get_data(G_OBJECT(widget), "contact");

    tmp = g_new0(GGaduContact, 1);
    tmp->id = g_strdup(k->id);
    tmp->nick = g_strdup(k->nick);
    tmp->last_name = g_strdup(k->last_name);
    tmp->first_name = g_strdup(k->first_name);
    tmp->mobile = g_strdup(k->mobile);
    tmp->status = k->status;
    tmp->city = g_strdup(k->city);

    signal_emit("main-gui", "add user search", tmp, g_object_get_data(G_OBJECT(widget), "plugin_name"));
    return NULL;
}

gboolean search_list_clicked(GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
    GtkTreePath *treepath = NULL;
    GtkTreeViewColumn *treevc = NULL;
    GtkTreeIter iter;
    GtkTreeModel *model;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));

    gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)), &model, &iter);

    if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3))
    {
	GtkItemFactory *ifactory = NULL;
	GGaduMenu *umenu;

	if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), event->x, event->y, &treepath, &treevc, NULL, NULL))
	    return FALSE;

	if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)), &model, &iter))
	{
	    GGaduContact *k;
	    GtkWidget *w;
	    gtk_tree_model_get(model, &iter, 2, &k, -1);

	    umenu = ggadu_menu_create();
	    ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("Add"), search_user_add, NULL));
	    ggadu_menu_print(umenu, NULL);

	    ifactory = gtk_item_factory_new(GTK_TYPE_MENU, "<name>", NULL);
	    w = gtk_item_factory_get_widget(ifactory, "<name>");
	    if (w)
	    {
		g_object_set_data(G_OBJECT(w), "plugin_name", g_object_get_data(G_OBJECT(widget), "plugin_name"));
		g_object_set_data(G_OBJECT(w), "contact", k);
	    }
	    gui_produce_menu_for_factory(umenu, ifactory, NULL, w);
	    gtk_item_factory_popup(ifactory, event->x_root, event->y_root, event->button, event->time);
	}

	gtk_tree_path_free(treepath);
	return TRUE;
    }
    return FALSE;
}

void gui_show_search_results(GSList * list, gchar * plugin_name)
{
    GtkWidget *search;
    GtkWidget *vbox;
    GtkListStore *search_liststore;
    GtkWidget *tree_view;
    GtkWidget *scrolled_window;
    GtkWidget *hbox;
    GtkWidget *button_close;
    GtkWidget *frame;
    GSList *tmplist = list;
    GtkTreeIter search_iter;
    gui_protocol *gp;

    search = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name(search, "GGSearchResults");


    gtk_window_set_title(GTK_WINDOW(search), _("Search results"));
    gtk_window_set_default_size(GTK_WINDOW(search), 350, 300);

    search_liststore = gtk_list_store_new(6, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(search_liststore));
/*
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view)), GTK_SELECTION_MULTIPLE);    
*/

    add_columns(GTK_TREE_VIEW(tree_view));

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_add(GTK_CONTAINER(frame), scrolled_window);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(search), vbox);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    button_close = gtk_button_new_from_stock("gtk-close");
    gtk_box_pack_end(GTK_BOX(hbox), button_close, FALSE, FALSE, 0);

    g_signal_connect(search, "destroy", G_CALLBACK(on_destroy_search), list);
    g_signal_connect_swapped(button_close, "clicked", G_CALLBACK(gtk_widget_destroy), search);
    g_signal_connect(G_OBJECT(tree_view), "button-press-event", G_CALLBACK(search_list_clicked), search_liststore);
    gtk_widget_show_all(search);

    gp = gui_find_protocol(plugin_name, protocols);

    if (gp)
    {
	g_object_set_data(G_OBJECT(tree_view), "plugin_name", gp->plugin_name);

	while (tmplist)
	{
	    GGaduContact *k = tmplist->data;
	    GGaduStatusPrototype *sp = ggadu_find_status_prototype(gp->p, k->status);
	    print_debug("adding kontakt to results list: %s\n", k->id);
	    if (sp && sp->image != NULL)
	    {
		gchar *display;
		gchar *dispcity;
		gchar *dispage;
		GdkPixbuf *image = create_pixbuf(sp->image);
		display = g_strdup_printf("%s %s%s%s", (k->first_name) ? k->first_name : "", (k->nick) ? "(" : "", (k->nick) ? k->nick : "", (k->nick) ? ")" : "");
		dispcity = g_strdup_printf("%s", (k->city) ? k->city : "");
		dispage = g_strdup_printf("%s", (k->age) ? k->age : "");
		gtk_list_store_append(search_liststore, &search_iter);
		gtk_list_store_set(search_liststore, &search_iter, 0, image, 1, k->id, 2, (gpointer) k, 3, display, 4, dispcity, 5, dispage, -1);
		gdk_pixbuf_unref(image);
	    }
	    tmplist = tmplist->next;
	}
    }
}
