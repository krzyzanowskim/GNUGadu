/* $Id: gui_userview.c,v 1.11 2003/06/11 17:49:16 zapal Exp $ */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "support.h"
#include "menu.h"
#include "gui_main.h"
#include "GUI_plugin.h"
#include "gui_chat.h"
#include "gui_support.h"
#include "gui_dialogs.h"

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

void status_clicked(GtkWidget *widget, GdkEventButton *ev, gpointer user_data) 
{
    gui_protocol *gp = user_data;
//    GGaduStatusPrototype *sp;
//    gint status;

    GtkWidget *popupmenu = create_status_menu(gp, gtk_bin_get_child(GTK_BIN(widget)));
    
    gtk_menu_popup(GTK_MENU(popupmenu), NULL, NULL, NULL, NULL, ev->button, ev->time);
    print_debug("status clicked\n");

//  status = (gint) signal_emit("main-gui", "get current status", NULL, gp->plugin_name);
//  sp = gui_find_status_prototype(gp->p, (status) ? status : gp->p->offline_status);
}

static
void on_pixbuf_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer, 
		    GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    GGaduContact *k = NULL;
    
    gtk_tree_model_get(model, iter, 2, &k, -1);
    
    if (!k) {
	g_object_set(G_OBJECT(renderer), "visible", FALSE, NULL);
    } else {
	g_object_set(G_OBJECT(renderer), "visible", TRUE, NULL);
    }

}

static
void on_text_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer, 
		    GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    GGaduContact *k = NULL;
    
    gtk_tree_model_get(model, iter, 2, &k, -1);
    
    if (!k) {
	gchar *font = config_var_get(gui_handler, "contact_list_protocol_font");
	g_object_set(G_OBJECT(renderer), "font", (font) ? font : "bold", NULL);
    } else {
	gchar *font = config_var_get(gui_handler, "contact_list_contact_font");
	g_object_set(G_OBJECT(renderer), "font", (font) ? font : "normal", NULL);
    }
}

void add_columns(GtkTreeView *tv) 
{
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;

    renderer = gtk_cell_renderer_pixbuf_new();
    
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "pixbuf", 0, NULL);
    gtk_tree_view_column_set_cell_data_func(column, renderer, on_pixbuf_data, NULL, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "text", 1, NULL);
    gtk_tree_view_column_set_cell_data_func(column, renderer, on_text_data, NULL, NULL);
    
    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
				   GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    gtk_tree_view_append_column (GTK_TREE_VIEW(tv), column); 
    gtk_tree_view_set_headers_visible( GTK_TREE_VIEW(tv), FALSE);
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

gint sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data)
{
    GGaduContact *k1, *k2;
    gchar *d1, *d2;
    gint p1, p2;
    gui_protocol *gp = user_data;
    
    gtk_tree_model_get(GTK_TREE_MODEL(model), a, 1, &d1, 2, &k1, -1);
    gtk_tree_model_get(GTK_TREE_MODEL(model), b, 1, &d2, 2, &k2, -1);
    
    if ((!k1 || !k2))
    	return 0;
	
    // je¶li status jest taki sam posortuj alfabetycznie wg. tego co wy¶wietla
    if ((k1->status == k2->status))
	return strcasecmp(d1, d2);
    
    if (!gp)
        gtk_tree_model_get(GTK_TREE_MODEL(model), a, 3, &gp, -1);
    
    // w innym przypadku sprawd¼ który status jest wy¿ej na li¶cie
    p1 = gui_get_status_pos(k1->status, gp);
    p2 = gui_get_status_pos(k2->status, gp);

    if (p1 > p2)
	return 1;
    else 
	return -1;
	
    return 0;
}

void gui_list_add(gui_protocol *gp) 
{
    GGaduStatusPrototype *sp;
    GtkListStore *users_liststore;
    GtkTreeModel *model;
    GtkWidget *vbox;
    GtkWidget *treeview;
    GtkWidget *scrolled_window;
    GtkWidget *label;
    GtkWidget *eventbox;
    gchar *markup;
    gint status;
    
    g_return_if_fail(gp != NULL);
    
    if (!notebook) {
	print_debug("no notebook, creating new one\n");
    	notebook = gtk_notebook_new();
    	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_RIGHT);
    	gtk_box_pack_start(GTK_BOX(view_container), notebook, TRUE, TRUE, 0);
	gtk_widget_show(notebook);
    }

    users_liststore = gtk_list_store_new (3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
    
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(users_liststore), 2, (GtkTreeIterCompareFunc) sort_func, gp, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(users_liststore), 2, GTK_SORT_ASCENDING);
    
    model = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(users_liststore));
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
    g_object_unref(model);

    /* podwojne klikniecie na liscie na kolesia */
    g_signal_connect(G_OBJECT(treeview), "button-release-event", G_CALLBACK(nick_list_clicked), users_liststore);
    g_signal_connect(G_OBJECT(treeview), "button-press-event", G_CALLBACK(nick_list_clicked), users_liststore);

    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), FALSE);

    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW (treeview)), GTK_SELECTION_MULTIPLE);

    add_columns(GTK_TREE_VIEW(treeview));
    
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    if (gp->p->img_filename) {
	label = create_image(gp->p->img_filename);
    } else {
	label = gtk_label_new(NULL);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	markup = g_strdup_printf("<small>%s</small>", gp->p->display_name);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);
    }
    
    /* cos co jest oznaczone jakos p->offine_status przez protocol */

    status = (gint) signal_emit("main-gui", "get current status", NULL, gp->plugin_name);
    sp = gui_find_status_prototype(gp->p, (status) ? status : *(int*)&gp->p->offline_status->data);

    if (sp) {
	gp->statuslist_eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(gp->statuslist_eventbox), create_image(sp->image));
	g_signal_connect(G_OBJECT(gp->statuslist_eventbox), "button-press-event", G_CALLBACK(status_clicked), gp);
	gtk_box_pack_start(GTK_BOX(status_hbox), gp->statuslist_eventbox, FALSE, FALSE, 2);
	gtk_widget_show_all(gp->statuslist_eventbox);
    }
    
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    
    gp->add_info_label = gtk_label_new(NULL);
    gtk_label_set_selectable(GTK_LABEL(gp->add_info_label), TRUE);
    gtk_widget_set_size_request(GTK_WIDGET(gp->add_info_label), 0, -1);
    gtk_misc_set_alignment(GTK_MISC(gp->add_info_label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(gp->add_info_label), 3, 0);

    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox),gp->add_info_label);

    gtk_box_pack_start(GTK_BOX(vbox), eventbox, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
    
    gtk_widget_show_all(vbox);
    gtk_widget_hide(gp->add_info_label);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);

    gp->users_liststore = users_liststore;
}

void gui_tree_add(gui_protocol *gp) 
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    GGaduStatusPrototype *sp;
    gint status = 0;
    
    g_return_if_fail(gp != NULL);
	
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    gtk_tree_store_append(GTK_TREE_STORE(users_treestore), &iter, NULL);
    gtk_tree_store_set(GTK_TREE_STORE(users_treestore), &iter, 0, NULL, 1, g_strdup_printf("%s (0/0)", gp->p->display_name), 3, gp,-1);
    gp->tree_path = g_strdup(gtk_tree_model_get_string_from_iter(model, &iter));
    
    status = (gint) signal_emit("main-gui", "get current status", NULL, gp->plugin_name);
    
    sp = gui_find_status_prototype(gp->p, (status) ? status : *(int*)&gp->p->offline_status->data);
    
    if (sp) {
    	gp->statuslist_eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(gp->statuslist_eventbox), create_image(sp->image));
        g_signal_connect(G_OBJECT(gp->statuslist_eventbox), "button-press-event", G_CALLBACK(status_clicked), gp);
	gtk_box_pack_start(GTK_BOX(status_hbox), gp->statuslist_eventbox, FALSE, FALSE, 2);
	gtk_widget_show_all(gp->statuslist_eventbox);
    }
    
    gp->add_info_label = g_object_get_data(G_OBJECT(treeview), "add_info_label");

    if (config_var_get(gui_handler, "expand"))
	gtk_tree_view_expand_all(GTK_TREE_VIEW(treeview));
}

void gui_create_tree()
{
    GtkWidget *vbox;
    GtkWidget *scrolled_window;
    GtkWidget *add_info_label;
    GtkWidget *eventbox;
    GtkWidget *frame;
    
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(view_container), vbox);
    
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

    users_treestore = gtk_tree_store_new (4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER);
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(users_treestore), 3, (GtkTreeIterCompareFunc) sort_func, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(users_treestore), 3, GTK_SORT_ASCENDING);
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(users_treestore));
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), FALSE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW (treeview)), GTK_SELECTION_MULTIPLE);

    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(treeview));

    /* podwojne klikniecie na liscie na kolesia */
    g_signal_connect(G_OBJECT(treeview), "button-release-event", G_CALLBACK(nick_list_clicked), users_treestore);
    g_signal_connect(G_OBJECT(treeview), "button-press-event", G_CALLBACK(nick_list_clicked), users_treestore);

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);	
    add_columns(GTK_TREE_VIEW(treeview));
    gtk_container_add(GTK_CONTAINER(frame), scrolled_window);

    add_info_label = gtk_label_new(NULL);
    gtk_label_set_selectable(GTK_LABEL(add_info_label), TRUE);
    gtk_widget_set_size_request(GTK_WIDGET(add_info_label), 0, -1);
    gtk_misc_set_alignment(GTK_MISC(add_info_label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(add_info_label), 3, 0);
	
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox),add_info_label);

    gtk_box_pack_start(GTK_BOX(vbox), eventbox, FALSE, FALSE, 0);
    
    gtk_widget_show_all(window);
    
    g_object_set_data(G_OBJECT(treeview), "add_info_label", add_info_label);
    gtk_widget_hide(add_info_label);
}


void gui_user_view_clear(gui_protocol *gp)
{
    GtkTreeIter users_iter;
    
    g_return_if_fail(gp != NULL);
    
    if (!tree) {
	gtk_list_store_clear(gp->users_liststore);
	g_free(g_object_get_data(G_OBJECT(gp->users_liststore),"plugin_name"));
    } else {
    	gboolean valid;
    	gchar *path = g_strdup_printf("%s:0", gp->tree_path);
    	valid = gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(users_treestore), &users_iter, path);
    	g_free(path);
    	while (valid) {
    		GdkPixbuf	*image = NULL;
    		gtk_tree_model_get(GTK_TREE_MODEL(users_treestore), &users_iter, 0, &image, -1);
    		valid = gtk_tree_store_remove(GTK_TREE_STORE(users_treestore), &users_iter);
    		gdk_pixbuf_unref(image);
    	}
    }
}

static
gint gui_get_active_users_count(gui_protocol *gp)
{
    GSList *tmplist = gp->userlist;
    gint active_count = 0;
    
    while (tmplist) {
	GGaduContact *k = tmplist->data;
	if (!is_in_status (k->status, gp->p->offline_status))
	    active_count++;
	tmplist = tmplist->next;
    }
    
    return active_count;
}

void gui_user_view_notify(gui_protocol *gp, GGaduNotify *n)
{
    GGaduStatusPrototype *sp = NULL;
    GtkTreeIter users_iter;
    GtkTreeIter parent_iter;
    GtkTreeModel *model;
    gboolean valid;
    gboolean found = FALSE;
    
    g_return_if_fail(gp != NULL);
    g_return_if_fail(n != NULL);
    
    sp = gui_find_status_prototype(gp->p, n->status);

    g_return_if_fail(sp != NULL);

    model = (tree) ? GTK_TREE_MODEL(users_treestore) : GTK_TREE_MODEL(gp->users_liststore);
    
    if (!tree) {
	valid = gtk_tree_model_get_iter_first(model, &users_iter);
    } else {
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
    	    GdkPixbuf	     *image = NULL;
	    gchar 	     *descr = NULL;
	    gui_chat_session *session = gui_session_find(gp, k->id);
	    gchar 	     *st = NULL;

	    found = TRUE;
	    if (k->status_descr)
		st = g_strdup_printf("- %s (%s)", (sp ? sp->description : ""), k->status_descr);
	    else
		st = g_strdup_printf("- %s", (sp ? sp->description : ""));

	    if (session) {
		gint chat_type = (gint)config_var_get(gui_handler,"chat_type");
		GtkWidget *window = (GtkWidget *)g_object_get_data(G_OBJECT(session->chat),"top_window");
		
		if (chat_type == CHAT_TYPE_CLASSIC) {
		    gchar *tmp = NULL;

		    if (!g_strcasecmp(k->nick,k->id))
			tmp = g_strdup_printf(_("Talking to %s %s"),k->id, (sp ? st : ""));
		    else
			tmp = g_strdup_printf(_("Talking to %s (%s) %s"),k->nick,k->id, (sp ? st : ""));
    		    
		    gtk_window_set_title(GTK_WINDOW(window), tmp);
		    g_free(tmp);
		}

		if (chat_type == CHAT_TYPE_TABBED) {
		    GtkWidget *chat_notebook = g_object_get_data(G_OBJECT(window),"chat_notebook");
		    guint     curr = gtk_notebook_get_current_page(GTK_NOTEBOOK(chat_notebook));
		    guint     nr = gtk_notebook_page_num(GTK_NOTEBOOK(chat_notebook), session->chat);
		    gchar     *tmp = g_strdup_printf("%s %s", g_strcasecmp(k->nick, k->id) ? k->nick : k->id, (sp ? st : ""));

		    g_object_set_data_full(G_OBJECT(session->chat),"tab_window_title_char",g_strdup(tmp),g_free);

		    /* update top_window title if current notebook page == this session's page */
		    if (nr == curr)
			gtk_window_set_title(GTK_WINDOW(window), tmp);

		    g_free(tmp);
		}
	    }
	    if (config_var_get(gui_handler, "show_active") && is_in_status (n->status, gp->p->offline_status)) {
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
		print_debug("%s : Nie mogê za³adowaæ pixmapy\n", "main-gui");

	    if (!tree) {
	    	gtk_list_store_set(gp->users_liststore, &users_iter, 0, image, -1);
	    } else {
		gtk_tree_store_set(users_treestore, &users_iter, 0, image, -1);
	    }
	    
	    if (k->status_descr)
		descr = g_strdup_printf("(%s)", k->status_descr);
		
	    gtk_tree_sortable_sort_column_changed(GTK_TREE_SORTABLE(model));
   	    signal_emit("main-gui", "xosd show message", g_strdup_printf("%s - %s %s", k->nick, 
									sp->description,(descr) ? descr : ""),"xosd");
	    g_free(descr);
	    g_free(st);
	}

	valid = gtk_tree_model_iter_next(model, &users_iter);
    }
    
    if (config_var_get(gui_handler, "show_active") && !found) {
	GSList *tmplist = gp->userlist;
	GGaduContact *k;

	if (tree)
	    gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL(users_treestore), &parent_iter, gp->tree_path);
		
	while (tmplist) {
	    k = tmplist->data;
	    
	    if (k && !is_in_status (n->status, gp->p->offline_status) && !ggadu_strcasecmp(n->id, k->id)) {
		GdkPixbuf	 *image		= create_pixbuf(sp->image);
		    
		if (image == NULL)
	    	    print_debug("%s : Nie mogê za³adowaæ pixmapy %s\n", "main-gui", sp->image);
	
		if (!tree) {
	    	    gtk_list_store_append(gp->users_liststore, &users_iter);
	    	    gtk_list_store_set(gp->users_liststore, &users_iter, 0, image, 1, k->nick, 2, (gpointer) k, -1);
		} else {
	    	    gtk_tree_store_append(users_treestore ,&users_iter, &parent_iter);
	    	    gtk_tree_store_set(users_treestore, &users_iter, 0, image, 1, k->nick, 2, (gpointer) k, 3, gp, -1);
		}
		break;
	    }
	    tmplist = tmplist->next;
	}
    }
    
    if (tree) {
	gchar *tmp;
	if(config_var_get(gui_handler, "expand")) {
	    GtkTreePath *path = gtk_tree_path_new_from_string(gp->tree_path);
	    gtk_tree_view_expand_row(GTK_TREE_VIEW(treeview), path, TRUE);
	    gtk_tree_path_free(path);
	}
	gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL(users_treestore), &parent_iter, gp->tree_path);
	gtk_tree_model_get(GTK_TREE_MODEL(users_treestore), &parent_iter, 1, &tmp, -1);
	gtk_tree_store_set(users_treestore, &parent_iter, 1, 
	g_strdup_printf("%s (%d/%d)", gp->p->display_name, gui_get_active_users_count(gp), g_slist_length(gp->userlist)),-1);
	g_free(tmp);
    }
}

void gui_user_view_add_userlist(gui_protocol *gp)
{
    GtkTreeIter parent_iter;
    GtkTreeIter users_iter;
    GSList *tmplist;

    g_return_if_fail(gp != NULL);

    tmplist = gp->userlist;
	
    gui_user_view_clear(gp);
    
    if (tree)
        gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL(users_treestore), &parent_iter, gp->tree_path);
    
    while (tmplist) {
	GGaduContact *k = tmplist->data;
	GGaduStatusPrototype *sp = gui_find_status_prototype(gp->p,k->status);
	
	print_debug("Adding %s %s\n",k->id, k->nick);
	
        if (config_var_get(gui_handler, "show_active") && is_in_status (k->status, gp->p->offline_status)) {
	    tmplist = tmplist->next;
	    continue;
	} 
	
	if (!k->nick) {
	    if (k->id)
		k->nick = g_strdup(k->id);
	    else
		k->nick = g_strdup(_("(None)"));
	}
			    
	if ((sp != NULL) && (sp->image != NULL)) {
	    GdkPixbuf	 *image		= create_pixbuf(sp->image);
		    
	    if (image == NULL)
	        print_debug("%s : Nie mogê za³adowaæ pixmapy %s\n", "main-gui", sp->image);

	    if (!tree) {
	    	gtk_list_store_append(gp->users_liststore, &users_iter);
	    	gtk_list_store_set(gp->users_liststore, &users_iter, 0, image, 1, k->nick, 2, (gpointer) k, -1);
	    } else {
	    	gtk_tree_store_append(users_treestore ,&users_iter, &parent_iter);
	    	gtk_tree_store_set(users_treestore, &users_iter, 0, image, 1, k->nick, 2, (gpointer) k, 3, gp, -1);
	    }
	}
	tmplist = tmplist->next;
    }

    if (tree) {
	gchar *tmp;
	gtk_tree_model_get(GTK_TREE_MODEL(users_treestore), &parent_iter, 1, &tmp, -1);
	gtk_tree_store_set(users_treestore, &parent_iter, 1, 
	g_strdup_printf("%s (%d/%d)", gp->p->display_name, gui_get_active_users_count(gp), g_slist_length(gp->userlist)),-1);
	g_free(tmp);
    } else {
    	g_object_set_data(G_OBJECT(gp->users_liststore),"plugin_name",g_strdup(gp->plugin_name));
	gtk_tree_sortable_sort_column_changed(GTK_TREE_SORTABLE(gp->users_liststore));
    }
}

static void gui_user_view_rescan_paths (gui_protocol *gp)
{
    gboolean valid;
    GtkTreeIter iter;
    GtkTreeModel *model = GTK_TREE_MODEL(users_treestore);

    valid = gtk_tree_model_get_iter_first (model, &iter);
    while (valid) {
	gui_protocol *_gp;

	gtk_tree_model_get (model, &iter, 3, &_gp, -1);

	if (_gp == gp)
    	    continue;
	
        g_free (_gp->tree_path);
	_gp->tree_path = g_strdup(gtk_tree_model_get_string_from_iter(model, &iter));
	valid = gtk_tree_model_iter_next (model, &iter);
    }
}

void gui_user_view_unregister(gui_protocol *gp)
{
    GtkTreeIter users_iter;
    
    g_return_if_fail(gp != NULL);
    
    gui_user_view_clear(gp);
    
    if (!tree) {
    } else {
    	gchar *path = gp->tree_path;
	gchar *txt = NULL;
    	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(users_treestore), &users_iter, path);
    	gtk_tree_model_get(GTK_TREE_MODEL(users_treestore), &users_iter, 1, &txt, -1);
	g_free(txt);
    	gtk_tree_store_remove(GTK_TREE_STORE(users_treestore), &users_iter);
    }

    if (gp->statuslist_eventbox)
        gtk_widget_destroy(gp->statuslist_eventbox);

    gui_user_view_rescan_paths (gp);
}

void gui_user_view_register(gui_protocol *gp)
{
    g_return_if_fail(gp != NULL);

    if (!tree)
	gui_list_add(gp);
    else 
	gui_tree_add(gp);
}

static void destroy_view_widgets() 
{
    GList *children, *tmplist;

    children = tmplist = gtk_container_get_children(GTK_CONTAINER(view_container));
    
    while (tmplist) {
	gtk_widget_destroy((GtkWidget *)tmplist->data);
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
    GSList *tmplist;
    
    tree = (gboolean) config_var_get(gui_handler, "tree");
    
    print_debug("refreshing user view\n");

    destroy_view_widgets();
    
    notebook = NULL;

    if (tree)
	gui_create_tree();
    
    tmplist = protocols;

    while (tmplist) {
	gui_protocol *gp = (gui_protocol *)tmplist->data;
	if (gp->statuslist_eventbox) {
	    gtk_widget_destroy(gp->statuslist_eventbox);
	    gp->statuslist_eventbox = NULL;
	}
	gui_user_view_register(gp);
	gui_user_view_add_userlist(gp);
	tmplist = tmplist->next;
    }
    
    if (tree && config_var_get(gui_handler, "expand"))
	gtk_tree_view_expand_all(GTK_TREE_VIEW(treeview));
}

void gui_user_view_switch()
{
    gboolean current_tree_var = (gboolean) config_var_get(gui_handler, "tree");
    print_debug("switching user view\n");
    if (tree == current_tree_var) 
	return;

    tree = current_tree_var;
    
    gui_user_view_refresh();

}

