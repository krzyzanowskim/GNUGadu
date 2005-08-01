/* $Id: gui_userview.c,v 1.68 2005/08/01 12:01:12 mkobierzycki Exp $ */

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
#include <string.h>
#include <gtk/gtk.h>

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
#include "gtkanimlabel.h"

extern gboolean tree;

extern GGaduPlugin *gui_handler;

extern GSList *protocols;

extern GtkWidget *window;
extern GtkWidget *view_container;
extern GtkWidget *status_hbox;

GtkWidget *notebook;
GtkWidget *tree_scrolled_window;
GtkWidget *treeview = NULL;
GtkTreeStore *users_treestore = NULL;
GtkTooltips *tooltips = NULL;

void status_clicked(GtkWidget * widget, GdkEventButton * ev, gpointer user_data)
{
	gui_protocol *gp = user_data;
	GtkWidget *popupmenu = create_status_menu(gp, gtk_bin_get_child(GTK_BIN(widget)));

	gtk_menu_popup(GTK_MENU(popupmenu), NULL, NULL, NULL, NULL, ev->button, ev->time);
	print_debug("status clicked");
}

static void on_pixbuf_data(GtkTreeViewColumn * column, GtkCellRenderer * renderer, GtkTreeModel * model,
			   GtkTreeIter * iter, gpointer data)
{
	GGaduContact *k = NULL;

	gtk_tree_model_get(model, iter, 2, &k, -1);

	if (!k)
	{
		g_object_set(G_OBJECT(renderer), "visible", FALSE, NULL);
	}
	else
	{
		g_object_set(G_OBJECT(renderer), "visible", TRUE, NULL);
	}

}

static void on_text_data(GtkTreeViewColumn * column, GtkCellRenderer * renderer, GtkTreeModel * model,
			 GtkTreeIter * iter, gpointer data)
{
	GGaduContact *k = NULL;

	gtk_tree_model_get(model, iter, 2, &k, -1);

	if (!k) {
		gchar *font = ggadu_config_var_get(gui_handler, "contact_list_protocol_font");
		g_object_set(G_OBJECT(renderer), "font", (font) ? font : "bold", NULL);
	} else {
		gchar *font = ggadu_config_var_get(gui_handler, "contact_list_contact_font");
		g_object_set(G_OBJECT(renderer), "font", (font) ? font : "normal", NULL);

		if (ggadu_config_var_get(gui_handler, "descr_on_list") && k->status_descr) {
		    gchar *descr = g_strdup(k->status_descr);
		    gchar *tmp0 = NULL;
		    gchar *tmp1 = NULL;
		    
		    if(ggadu_config_var_get(gui_handler, "wrap_descr"))
		    {
		        gint cur_part_width = 0;
		        gint cur_char_width = 0;
		        gint max_width	    = 0;
		        float fix_width	    = 0;
		        gint last_space	    = 0;
		        gint skip           = 0;
		        gint i              = 0;
		    
		        if (GTK_WIDGET_VISIBLE(window))
			    gtk_window_get_size(GTK_WINDOW(window), &max_width, NULL);
		        else 
			    max_width = (gint) ggadu_config_var_get(gui_handler, "width");
			
		        max_width -= 65;

			/* We're working on UTF-8 strings here. */
		        for (i = 0; i < g_utf8_strlen(descr, -1); i++)
		        {
			    cur_part_width = cur_char_width;
			
			    if (cur_char_width == cur_part_width) fix_width += 6;

			    if ((g_utf8_offset_to_pointer(descr, i)[0] == '.') ||
			        (g_utf8_offset_to_pointer(descr, i)[0] == ' ') ||
				(g_utf8_offset_to_pointer(descr, i)[0] == '!') ||
				(g_utf8_offset_to_pointer(descr, i)[0] == ':')) fix_width += 0.4;
			
			    if (g_utf8_offset_to_pointer(descr, i)[0] == ' ')
			        last_space = i;
			    else if (g_utf8_offset_to_pointer(descr, i)[0] == '\n')
			        skip = i;
			    
			    if ((cur_char_width+(gint)fix_width) >= max_width)
			    {
			        if(last_space != 0)
			        {
				    g_utf8_offset_to_pointer(descr, last_space)[0] = '\n';
				    cur_part_width    = 0;
				    i                 = last_space;
				    last_space        = 0;
				    skip              = i;
				    fix_width         = 0;
			        }
			        else
			        {
				    tmp1 = g_strndup(descr, (gint)(g_utf8_offset_to_pointer(descr, i) - descr)/sizeof(gchar));
				    tmp0           = descr;
				    descr          = g_strdup_printf("%s\n%s", tmp1, g_utf8_offset_to_pointer(descr, i));
				    cur_part_width = 0;
				    skip           = i;
				    fix_width      = 0;
				    g_free(tmp0);
				    g_free(tmp1);
				    tmp0 = NULL;
				    tmp1 = NULL;
			        }
			    }
		        }
		    } 

		    gchar *markup_descr = ggadu_config_var_get(gui_handler, "wrap_descr") ?
		                          g_markup_escape_text(descr, strlen(descr)) :
			                  g_markup_escape_text(k->status_descr, strlen(k->status_descr));
		    gchar *markup = g_strdup_printf("%s\n<small>%s</small>", k->nick, markup_descr);
		    g_object_set(G_OBJECT(renderer), "text", NULL, "markup", markup, NULL);
		    g_free(markup_descr);
		    g_free(markup);
		    if(ggadu_config_var_get(gui_handler, "wrap_descr")) g_free(descr);
		}
	}
}

void add_columns(GtkTreeView * tv)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	column = gtk_tree_view_column_new();

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "pixbuf", 0, NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer, on_pixbuf_data, NULL, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", 1, NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer, on_text_data, NULL, NULL);

	gtk_tree_view_column_set_sizing(GTK_TREE_VIEW_COLUMN(column), GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), FALSE);
}

gint gui_get_status_pos(guint status, gpointer user_data)
{
	gint pos = 0;
	gui_protocol *gp = user_data;
	GSList *tmp = gp->p->statuslist;

	while (tmp)
	{
		GGaduStatusPrototype *sp = tmp->data;

		if ((sp) && (sp->status == status))
			return pos;

		pos++;
		tmp = tmp->next;
	}
	return pos;
}

gint sort_func(GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer user_data)
{
	GGaduContact *k1, *k2;
	gchar *d1, *d2;
	gint p1, p2, s1, s2, ret;
	gui_protocol *gp = user_data;

	gtk_tree_model_get(GTK_TREE_MODEL(model), a, 1, &d1, 2, &k1, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(model), b, 1, &d2, 2, &k2, -1);

	if ((!k1 || !k2))
		return 0;

	if (!gp)
		gtk_tree_model_get(GTK_TREE_MODEL(model), a, 3, &gp, -1);

	s1 = k1->status;
	s2 = k2->status;

	/* print_debug("SORTUJE\n"); */
	/* maly trik, jesli statusy sa w tej samej grupie to trakuj je jako te same statusy przy sortowaniu */
	if ((ggadu_is_in_status(s1,gp->p->online_status) && ggadu_is_in_status(s2,gp->p->online_status)) ||
	   (ggadu_is_in_status(s1,gp->p->away_status) && ggadu_is_in_status(s2,gp->p->away_status)) ||
	   (ggadu_is_in_status(s1,gp->p->offline_status) && ggadu_is_in_status(s2,gp->p->offline_status)))
	    s1=s2;
	    
	/* je¶li status jest taki sam posortuj alfabetycznie wg. tego co wy¶wietla */
	if ((s1 == s2))
	{
		/*print_debug("%s %d %s %d  %d",d1,s1,d2,s2,ggadu_strcasecmp(d1, d2));*/
		return ggadu_strcasecmp(d1, d2);
	}
	
	/* w innym przypadku sprawd¼ który status jest wy¿ej na li¶cie */
	p1 = gui_get_status_pos(k1->status, gp);
	p2 = gui_get_status_pos(k2->status, gp);


	if (p1 > p2)
		ret = 1;
	else
		ret = -1;

	return ret;
}

static void create_protocol_icon(gui_protocol *gp, GGaduStatusPrototype *sp)
{
	if (!gp->tooltips) {
	    gp->tooltips = gtk_tooltips_new();
	    gtk_tooltips_enable(gp->tooltips);
	}
	gp->statuslist_eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(gp->statuslist_eventbox), create_image(sp->image));
	gtk_tooltips_set_tip(gp->tooltips, gp->statuslist_eventbox, sp->description, NULL);
	g_signal_connect(G_OBJECT(gp->statuslist_eventbox), "button-press-event", G_CALLBACK(status_clicked), gp);
	gtk_box_pack_start(GTK_BOX(status_hbox), gp->statuslist_eventbox, FALSE, FALSE, 2);
	gtk_widget_show_all(gp->statuslist_eventbox);
}

void gui_list_add(gui_protocol * gp)
{
	GGaduStatusPrototype *sp = NULL;
	GtkListStore *users_liststore = NULL;
	GtkTreeModel *model = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *treeview = NULL;
	GtkWidget *scrolled_window = NULL;
	GtkWidget *label = NULL;
	GtkWidget *eventbox = NULL;
	GtkWidget *add_info_label_desc = NULL;
	GtkTreeSelection *selection = NULL;
	gchar *markup = NULL;

	if (!gp) return;

	if (!notebook)
	{
		print_debug("no notebook, creating new one");
		notebook = gtk_notebook_new();
		gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_RIGHT);
		gtk_box_pack_start(GTK_BOX(view_container), notebook, TRUE, TRUE, 0);
		gtk_widget_show(notebook);
	}

	users_liststore = gtk_list_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);

	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(users_liststore), 2, (GtkTreeIterCompareFunc) sort_func, gp,
					NULL);
	model = GTK_TREE_MODEL(users_liststore);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), 2, GTK_SORT_ASCENDING);
	
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
	
	g_object_set_data(G_OBJECT(users_liststore),"treeview",treeview);
	g_signal_connect(G_OBJECT(model),"row-changed",G_CALLBACK(nick_list_row_changed2),users_liststore);
	g_object_unref(model);

	/* podwojne klikniecie na liscie na kolesia */
	g_signal_connect(G_OBJECT(treeview), "button-press-event", G_CALLBACK(nick_list_clicked), users_liststore);
	g_signal_connect(G_OBJECT(treeview), "row-activated", G_CALLBACK(nick_list_row_activated), users_liststore);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	gtk_tree_selection_set_select_function(selection, nick_list_row_changed, users_liststore, NULL);
	
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), FALSE);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_MULTIPLE);

	add_columns(GTK_TREE_VIEW(treeview));

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	if (gp->p->img_filename)
	{
		label = create_image(gp->p->img_filename);
	}
	else
	{
		label = gtk_label_new(NULL);
		gtk_label_set_selectable(GTK_LABEL(label), FALSE);
		markup = g_strdup_printf("<small>%s</small>", gp->p->display_name);
		gtk_label_set_markup(GTK_LABEL(label), markup);
		g_free(markup);
	}

	/* cos co jest oznaczone jakos p->offine_status przez protocol */
	print_debug("gui_list_add");
	// !ZONK should be freed ??
	sp = signal_emit("main-gui", "get current status", NULL, gp->plugin_name);

	if (!sp)
	{
		if (gp->p->offline_status)
		{
			// !ZONK should be freed
			sp = ggadu_find_status_prototype(gp->p, *(gint *) &gp->p->offline_status->data);
		}
		else if (gp->p->statuslist)
			sp = gp->p->statuslist->data; /* last resord, get dirst status from statuslist */
	}

	if (sp && !sp->receive_only)
	    create_protocol_icon(gp, sp);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

	gp->add_info_label = gtk_label_new(NULL);

	gtk_label_set_selectable(GTK_LABEL(gp->add_info_label), TRUE);
	gtk_widget_set_size_request(GTK_WIDGET(gp->add_info_label), 0, -1);
	gtk_misc_set_alignment(GTK_MISC(gp->add_info_label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(gp->add_info_label), 3, 0);

	add_info_label_desc = gtk_anim_label_new();
	gtk_anim_label_set_delay(GTK_ANIM_LABEL(add_info_label_desc), 2);

	eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eventbox), add_info_label_desc);

	gtk_box_pack_start(GTK_BOX(vbox), gp->add_info_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), eventbox, FALSE, FALSE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
	
	if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) == 1)
	    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook),FALSE);
	else
	    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook),TRUE);

	gtk_widget_show_all(vbox);
	gtk_widget_hide(gp->add_info_label);
	gtk_widget_hide(add_info_label_desc);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);

	g_object_set_data(G_OBJECT(vbox), "add_info_label", gp->add_info_label);
	g_object_set_data(G_OBJECT(gp->add_info_label), "add_info_label_desc", add_info_label_desc);

	gp->users_liststore = users_liststore;
	
	//forcing realize treeview before we render contacts
	gtk_widget_realize(treeview); 
	
	// !ZONK take a look 
	//GGaduStatusPrototype_free(sp);
}

void gui_tree_add(gui_protocol * gp)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GGaduStatusPrototype *sp = NULL;

	g_return_if_fail(gp != NULL);

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

	gtk_tree_store_append(GTK_TREE_STORE(users_treestore), &iter, NULL);
	gtk_tree_store_set(GTK_TREE_STORE(users_treestore), &iter, 0, NULL, 1,
			   g_strdup_printf("%s (0/0)", gp->p->display_name), 3, gp, -1);
	gp->tree_path = g_strdup(gtk_tree_model_get_string_from_iter(model, &iter));
	print_debug("gui_tree_add");
	
	// !ZONK should be freed ??
	sp = signal_emit("main-gui", "get current status", NULL, gp->plugin_name);

	if (!sp)
	{
		if (gp->p->offline_status)
			sp = ggadu_find_status_prototype(gp->p, *(int *) &gp->p->offline_status->data);
		else if (gp->p->statuslist)
			sp = gp->p->statuslist->data; /* last resord, get dirst status from statuslist */
	}

	if (sp && !sp->receive_only)
	    create_protocol_icon(gp, sp);

	gp->add_info_label = g_object_get_data(G_OBJECT(treeview), "add_info_label");

	if (ggadu_config_var_get(gui_handler, "expand"))
		gtk_tree_view_expand_all(GTK_TREE_VIEW(treeview));
		
//	!ZONK take a look	
//	GGaduStatusPrototype_free(sp);
}

void gui_create_tree()
{
	GtkWidget *vbox;
	GtkWidget *scrolled_window;
	GtkWidget *add_info_label;
	GtkWidget *add_info_label_desc;
	GtkWidget *eventbox;
	GtkWidget *frame;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(view_container), vbox);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

	users_treestore = gtk_tree_store_new(4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(users_treestore), 3, (GtkTreeIterCompareFunc) sort_func, NULL,
					NULL);

	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(users_treestore));
	gtk_widget_set_name(GTK_WIDGET(treeview),"GGaduTreeView");
	model = GTK_TREE_MODEL(users_treestore);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), 3, GTK_SORT_ASCENDING);
	
	g_object_set_data(G_OBJECT(users_treestore),"treeview",treeview);
	g_signal_connect(G_OBJECT(model),"row-changed",G_CALLBACK(nick_list_row_changed2),users_treestore);
	
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), FALSE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_MULTIPLE);

	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(treeview));

	/* podwojne klikniecie na liscie na kolesia */
	g_signal_connect(G_OBJECT(treeview), "button-press-event", G_CALLBACK(nick_list_clicked), users_treestore);
	g_signal_connect(G_OBJECT(treeview), "row-activated", G_CALLBACK(nick_list_row_activated), users_treestore);
	
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	gtk_tree_selection_set_select_function(selection, nick_list_row_changed, users_treestore, NULL);
	
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	add_columns(GTK_TREE_VIEW(treeview));
	gtk_container_add(GTK_CONTAINER(frame), scrolled_window);

	add_info_label = gtk_label_new(NULL);
	gtk_label_set_selectable(GTK_LABEL(add_info_label), TRUE);
	gtk_widget_set_size_request(GTK_WIDGET(add_info_label), 0, -1);
	gtk_misc_set_alignment(GTK_MISC(add_info_label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(add_info_label), 3, 0);

	add_info_label_desc = gtk_anim_label_new();
	gtk_anim_label_set_delay(GTK_ANIM_LABEL(add_info_label_desc), 2);

	eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eventbox), add_info_label_desc);

	gtk_box_pack_start(GTK_BOX(vbox), add_info_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), eventbox, FALSE, FALSE, 0);

	gtk_widget_show_all(view_container);

	g_object_set_data(G_OBJECT(treeview), "add_info_label", add_info_label);
	g_object_set_data(G_OBJECT(add_info_label), "add_info_label_desc", add_info_label_desc);

	gtk_widget_hide(add_info_label);
	gtk_widget_hide(add_info_label_desc);
}


void gui_user_view_clear(gui_protocol * gp)
{
	GtkTreeIter users_iter;
	gboolean valid = FALSE;
	
	g_return_if_fail(gp != NULL);
	
	if (tree)
	{
		gchar *path = g_strdup_printf("%s:0", gp->tree_path);
		valid = gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(users_treestore), &users_iter, path);
		g_free(path);
	} else {
		valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gp->users_liststore),&users_iter);
	}

	while (valid)
	{
		GdkPixbuf *image = NULL;
		gchar *id = NULL;
		/*GGaduContact *k = NULL;*/
		
		gtk_tree_model_get((tree) ? GTK_TREE_MODEL(users_treestore) : GTK_TREE_MODEL(gp->users_liststore), &users_iter, 0, &image, -1);
		gtk_tree_model_get((tree) ? GTK_TREE_MODEL(users_treestore) : GTK_TREE_MODEL(gp->users_liststore), &users_iter, 1, &id, -1);
		/*gtk_tree_model_get((tree) ? GTK_TREE_MODEL(users_treestore) : GTK_TREE_MODEL(gp->users_liststore), &users_iter, 2, &k, -1);*/
		gdk_pixbuf_unref(image);
		g_free(id);
		/*GGaduContact_free(k);*/
		
		if (tree)
			valid = gtk_tree_store_remove(GTK_TREE_STORE(users_treestore), &users_iter);
		else
			valid = gtk_list_store_remove(GTK_LIST_STORE(gp->users_liststore), &users_iter);
/*
		if (!tree)
		{
			g_free(g_object_get_data(G_OBJECT(gp->users_liststore), "plugin_name"));
			gtk_list_store_clear(gp->users_liststore);
		}
*/
	}
}

static gint gui_get_active_users_count(gui_protocol * gp)
{
	GSList *tmplist = gp->userlist;
	gint active_count = 0;

	while (tmplist)
	{
		GGaduContact *k = tmplist->data;
		if (!ggadu_is_in_status(k->status, gp->p->offline_status))
			active_count++;
		tmplist = tmplist->next;
	}

	return active_count;
}

void gui_user_view_notify(gui_protocol * gp, GGaduNotify * n)
{
	GGaduStatusPrototype *sp = NULL;
	GtkTreeIter users_iter;
	GtkTreeIter parent_iter;
	GtkTreeModel *model;
	gboolean valid;
	gboolean found = FALSE;
	
	g_return_if_fail(gp != NULL);
	g_return_if_fail(n != NULL);

	sp = ggadu_find_status_prototype(gp->p, n->status);

	g_return_if_fail(sp != NULL);

	model = (tree) ? GTK_TREE_MODEL(users_treestore) : GTK_TREE_MODEL(gp->users_liststore);

	if (!tree)
	{
		valid = gtk_tree_model_get_iter_first(model, &users_iter);
	}
	else
	{
		gchar *path = g_strdup_printf("%s:0", gp->tree_path);
		valid = gtk_tree_model_get_iter_from_string(model, &users_iter, path);
		g_free(path);
	}

	while (valid)
	{
		GGaduContact *k = NULL;
		GdkPixbuf *oldpixbuf = NULL;
		gtk_tree_model_get(model, &users_iter, 0, &oldpixbuf, 2, &k, -1);

		if (k && !ggadu_strcasecmp(n->id, k->id))
		{
			GdkPixbuf *image = NULL;
			gchar *descr = NULL;
			gui_chat_session *session = gui_session_find(gp, k->id);
			gchar *st = NULL;

			found = TRUE;

			if (k->status_descr)
				st = g_strdup_printf("- %s (%s)", k->status_descr, (sp ? sp->description : ""));
			else
				st = g_strdup_printf("- %s", (sp ? sp->description : ""));

			if (session)
			{
				gint chat_type = (gint) ggadu_config_var_get(gui_handler, "chat_type");
				GtkWidget *window =
					(GtkWidget *) g_object_get_data(G_OBJECT(session->chat), "top_window");

				if (ggadu_config_var_get(gui_handler, "notify_status_changes"))
				{
					GGaduMsg *msg = g_new0(GGaduMsg, 1);
					msg->id = g_strdup(session->id);
					if (k->status_descr)
						msg->message = g_strdup_printf("%s (%s)", sp ? sp->description : "", k->status_descr);
					else
						msg->message = g_strdup_printf("%s", sp ? sp->description : "");
					msg->class = (g_slist_length(session->recipients) > 1) ? GGADU_CLASS_CONFERENCE : GGADU_CLASS_CHAT;
					msg->recipients = g_slist_copy(session->recipients);
					gui_chat_append(session->chat, msg, FALSE, TRUE);
				}

				if (chat_type == CHAT_TYPE_CLASSIC)
				{
					const gchar *tmp = NULL;

					if (!ggadu_strcasecmp(k->nick, k->id))
						tmp = g_strdup_printf("%s %s", k->id, (sp ? st : ""));
					else
						tmp = g_strdup_printf("%s (%s) %s", k->nick, k->id, (sp ? st : ""));

					gtk_window_set_title(GTK_WINDOW(window), tmp);
					/* g_free(tmp);  tmp shoud be const char */
				} else if (chat_type == CHAT_TYPE_TABBED)
				{
					GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(window), "chat_notebook");
					guint curr = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
					guint nr = gtk_notebook_page_num(GTK_NOTEBOOK(chat_notebook), session->chat);
					const gchar *tmp = g_strdup_printf("%s %s",
								     ggadu_strcasecmp(k->nick, k->id) ? k->nick : k->id,
								     (sp ? st : ""));

					g_object_set_data_full(G_OBJECT(session->chat), "tab_window_title_char",
							       g_strdup(tmp), g_free);

					/* update top_window title if current notebook page == this session's page */
					if (nr == curr)
						gtk_window_set_title(GTK_WINDOW(window), tmp);

					/* g_free(tmp); const gchar */
				}
			}
			if (ggadu_config_var_get(gui_handler, "show_active") &&
			    ggadu_is_in_status(n->status, gp->p->offline_status))
			{
				if (tree)
					gtk_tree_store_remove(GTK_TREE_STORE(users_treestore), &users_iter);
				else
					gtk_list_store_remove(gp->users_liststore, &users_iter);

				gdk_pixbuf_unref(oldpixbuf);

				valid = gtk_tree_model_iter_next(model, &users_iter);

				continue;
			}

			image = create_pixbuf(sp->image);

			if (image == NULL)
				print_debug("main-gui : Nie mogê za³adowaæ pixmapy");

			if (!tree)
			{
				gtk_list_store_set(gp->users_liststore, &users_iter, 0, image, -1);
				gtk_list_store_set(gp->users_liststore, &users_iter, 1, k->nick, -1);

			}
			else
			{
				gtk_tree_store_set(users_treestore, &users_iter, 0, image, -1);
				gtk_tree_store_set(users_treestore, &users_iter, 1, k->nick, -1);
			}

			if (k->status_descr)
				descr = g_strdup_printf("(%s)", k->status_descr);

			gtk_tree_sortable_sort_column_changed(GTK_TREE_SORTABLE(model));
			
			if (find_plugin_by_name("xosd") && (gint) ggadu_config_var_get(gui_handler, "use_xosd_for_status_change") == TRUE)
			{
				signal_emit("main-gui", "xosd show message",
					    g_strdup_printf("%s - %s %s", k->nick, sp->description,
							    (descr) ? descr : ""), "xosd");
			}
			g_free(descr);
			g_free(st);
		}

		valid = gtk_tree_model_iter_next(model, &users_iter);
	}

	if (ggadu_config_var_get(gui_handler, "show_active") && !found)
	{
		GSList *tmplist = gp->userlist;
		GGaduContact *k;

		if (tree)
			gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(users_treestore), &parent_iter,
							    gp->tree_path);

		while (tmplist)
		{
			k = tmplist->data;

			if (k && !ggadu_is_in_status(n->status, gp->p->offline_status) && !ggadu_strcasecmp(n->id, k->id))
			{
				GdkPixbuf *image = create_pixbuf(sp->image);

				if (image == NULL)
					print_debug("%s : Nie mogê za³adowaæ pixmapy %s\n", "main-gui", sp->image);

				if (!tree)
				{
					gtk_list_store_append(gp->users_liststore, &users_iter);
					gtk_list_store_set(gp->users_liststore, &users_iter, 0, image, 1, k->nick, 2,
							   (gpointer) k, -1);
				}
				else
				{
					gtk_tree_store_append(users_treestore, &users_iter, &parent_iter);
					gtk_tree_store_set(users_treestore, &users_iter, 0, image, 1, k->nick, 2,
							   (gpointer) k, 3, gp, -1);
				}
				break;
			}
			tmplist = tmplist->next;
		}
	}

	if (tree)
	{
		gchar *tmp;
		if (ggadu_config_var_get(gui_handler, "expand"))
		{
			GtkTreePath *path = gtk_tree_path_new_from_string(gp->tree_path);
			gtk_tree_view_expand_row(GTK_TREE_VIEW(treeview), path, TRUE);
			gtk_tree_path_free(path);
		}
		gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(users_treestore), &parent_iter, gp->tree_path);
		gtk_tree_model_get(GTK_TREE_MODEL(users_treestore), &parent_iter, 1, &tmp, -1);
		gtk_tree_store_set(users_treestore, &parent_iter, 1,
				   g_strdup_printf("%s (%d/%d)", gp->p->display_name, gui_get_active_users_count(gp),
						   g_slist_length(gp->userlist)), -1);
		g_free(tmp);
	}
	GGaduStatusPrototype_free(sp);
}

void gui_user_view_add_userlist(gui_protocol * gp)
{
	GtkTreeIter parent_iter;
	GtkTreeIter users_iter;
	GSList *tmplist = NULL;
	gboolean row_expanded = FALSE;
	GtkTreePath *path = NULL;
	GtkWidget *add_info_label_desc = NULL;
	GtkTooltipsData *tool_data = NULL;
	
	g_return_if_fail(gp != NULL);


	if (tree)
	{
		gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(users_treestore), &parent_iter, gp->tree_path);
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(users_treestore), &parent_iter);
		row_expanded = gtk_tree_view_row_expanded(GTK_TREE_VIEW(treeview), path);
	}

	gui_user_view_clear(gp);

	tmplist = gp->userlist;

	add_info_label_desc = g_object_get_data(G_OBJECT(gp->add_info_label), "add_info_label_desc");

	if(!tmplist && GTK_WIDGET_VISIBLE(gp->add_info_label))
	    gtk_widget_hide(GTK_WIDGET(gp->add_info_label));

	if(!tmplist && GTK_WIDGET_VISIBLE(add_info_label_desc))
	{
	    tool_data = gtk_tooltips_data_get(gtk_widget_get_ancestor(add_info_label_desc, GTK_TYPE_EVENT_BOX));
	    gtk_tooltips_disable(tool_data->tooltips);
	    gtk_widget_hide(GTK_WIDGET(add_info_label_desc));
	}
	
	while (tmplist)
	{
		GGaduContact *k = tmplist->data;
		GGaduStatusPrototype *sp = ggadu_find_status_prototype(gp->p, k->status);

		print_debug("Adding %s %s", k->id, k->nick);

		if (ggadu_config_var_get(gui_handler, "show_active") && ggadu_is_in_status(k->status, gp->p->offline_status))
		{
			tmplist = tmplist->next;
			continue;
		}

		if (!k->nick)
		{
			if (k->id)
				k->nick = g_strdup(k->id);
			else
				k->nick = g_strdup(_("(None)"));
		}

		if ((sp != NULL) && (sp->image != NULL))
		{
			GdkPixbuf *image = create_pixbuf(sp->image);

			if (image == NULL)
				print_debug("%s : Nie mogê za³adowaæ pixmapy %s\n", "main-gui", sp->image);

			if (!tree)
			{
				gtk_list_store_append(gp->users_liststore, &users_iter);
				gtk_list_store_set(gp->users_liststore, &users_iter, 0, image, 1, k->nick, 2,
						   (gpointer) k, -1);
			}
			else
			{
				gtk_tree_store_append(users_treestore, &users_iter, &parent_iter);
				gtk_tree_store_set(users_treestore, &users_iter, 0, image, 1, k->nick, 2, (gpointer) k,
						   3, gp, -1);
			}
		}
		GGaduStatusPrototype_free(sp);
		tmplist = tmplist->next;
	}

	if (tree)
	{
		gchar *tmp = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(users_treestore), &parent_iter, 1, &tmp, -1);
		gtk_tree_store_set(users_treestore, &parent_iter, 1,
				   g_strdup_printf("%s (%d/%d)", gp->p->display_name, gui_get_active_users_count(gp),
						   g_slist_length(gp->userlist)), -1);
		g_free(tmp);
		
		if (row_expanded)
		    gtk_tree_view_expand_row(GTK_TREE_VIEW(treeview), path, TRUE);

		gtk_tree_path_free(path);
	}
	else
	{
		g_object_set_data(G_OBJECT(gp->users_liststore), "plugin_name", g_strdup(gp->plugin_name));
		gtk_tree_sortable_sort_column_changed(GTK_TREE_SORTABLE(gp->users_liststore));
	}
}

static void gui_user_view_rescan_paths(gui_protocol * gp)
{
	gboolean valid;
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(users_treestore);

	valid = gtk_tree_model_get_iter_first(model, &iter);
	while (valid)
	{
		gui_protocol *_gp;

		gtk_tree_model_get(model, &iter, 3, &_gp, -1);

		if (_gp == gp)
			continue;

		g_free(_gp->tree_path);
		_gp->tree_path = g_strdup(gtk_tree_model_get_string_from_iter(model, &iter));
		valid = gtk_tree_model_iter_next(model, &iter);
	}
}

void gui_user_view_unregister(gui_protocol * gp)
{
	GtkTreeIter users_iter;

	g_return_if_fail(gp != NULL);

	gui_user_view_clear(gp);

	if (tree)
	{
		gchar *path = gp->tree_path;
		gchar *txt = NULL;
		gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(users_treestore), &users_iter, path);
		gtk_tree_model_get(GTK_TREE_MODEL(users_treestore), &users_iter, 1, &txt, -1);
		g_free(txt);
		gtk_tree_store_remove(GTK_TREE_STORE(users_treestore), &users_iter);
	}

	if (gp->statuslist_eventbox)
		gtk_widget_destroy(gp->statuslist_eventbox);

	gui_user_view_rescan_paths(gp);
}

void gui_user_view_register(gui_protocol * gp)
{
	g_return_if_fail(gp != NULL);

	if (!tree) {
		gui_list_add(gp);
	} else {
		gui_tree_add(gp);
	}	
}

/* ZONK */
static void destroy_view_widgets()
{
	GList *children, *tmplist;

	children = tmplist = gtk_container_get_children(GTK_CONTAINER(view_container));

	while (tmplist)
	{
		gtk_widget_destroy((GtkWidget *) tmplist->data);
		tmplist = tmplist->next;
	}

	g_list_free(children);

}

/*
static gboolean get_iter_from_path(GtkTreeModel *model, GtkTreeIter *iter, gchar *path, ...)
{
    gboolean valid;
    valid = gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(users_treestore), &users_iter, path);
    return valid;
}
*/

void gui_user_view_refresh()
{
	GSList *tmplist = NULL;
	
	tree = (gboolean) ggadu_config_var_get(gui_handler, "tree");

	print_debug("refreshing user view\n");

	destroy_view_widgets();	/* ZONK crash */

	notebook = NULL;

	if (tree)
		gui_create_tree();

	tmplist = protocols;

	while (tmplist)
	{
		gui_protocol *gp = (gui_protocol *) tmplist->data;
		if (gp->statuslist_eventbox)
		{
			gtk_widget_destroy(gp->statuslist_eventbox);
			gp->statuslist_eventbox = NULL;
		}
		gui_user_view_register(gp);
		gui_user_view_add_userlist(gp);
		tmplist = tmplist->next;
	}

	if (tree && ggadu_config_var_get(gui_handler, "expand"))
		gtk_tree_view_expand_all(GTK_TREE_VIEW(treeview));
	
	if (tree)	
		gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview),TRUE);
}

void gui_user_view_switch()
{
	gboolean current_tree_var = (gboolean) ggadu_config_var_get(gui_handler, "tree");
	print_debug("switching user view\n");
	if (tree == current_tree_var)
		return;

	tree = current_tree_var;

	gui_user_view_refresh();

}
