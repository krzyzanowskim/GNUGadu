/* $Id: gui_handlers.c,v 1.2 2003/03/23 11:51:31 zapal Exp $ */

#include <gtk/gtk.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "support.h"
#include "menu.h"
#include "GUI_plugin.h"
#include "gui_chat.h"
#include "gui_support.h"
#include "gui_main.h"
#include "gui_search.h"
#include "gui_dialogs.h"
#include "gui_userview.h"
#include "gui_handlers.h"

GtkTreeIter users_iter;
GtkItemFactory	*item_factory = NULL;

extern GtkTreeStore *users_treestore;
extern GGaduConfig *config;
extern GGaduPlugin *gui_handler;
extern GSList *protocols;
extern GSList *emoticons;
extern GtkWidget *window;
extern gboolean tree;
extern GSList *invisible_chats;

void handle_add_user_window(GGaduSignal *signal) 
{
	gui_user_data_window(signal,FALSE);
}

void handle_show_dialog(GGaduSignal *signal) 
{
	gui_show_dialog(signal,FALSE);
}

void handle_show_window_with_text(GGaduSignal *signal) 
{
        gui_show_window_with_text(signal);
}

void handle_change_user_window(GGaduSignal *signal)
{
	gui_user_data_window(signal,TRUE);
}

void handle_msg_receive(GGaduSignal *signal)
{
	gui_msg_receive(signal);
}

void handle_show_invisible_chats(GGaduSignal *signal)
{
    GSList *tmp = invisible_chats;
    
    if (!invisible_chats) 
    {
	gui_show_hide_window();
	gtk_window_move(GTK_WINDOW(window), 
		(gint)config_var_get(gui_handler, "left"), 
		(gint)config_var_get(gui_handler, "top"));
//	gdk_threads_leave();
	
	return;
    }
	    
    while (tmp) 
    {
	GtkWidget *chat = tmp->data;
	gtk_widget_show_all(chat);
	tmp = tmp->next;
    }
	    
    g_slist_free(invisible_chats);
    invisible_chats = NULL;
}

void handle_register_protocol(GGaduSignal *signal)
{
    GGaduProtocol *p = signal->data;
    gui_protocol *gp = g_new0(gui_protocol, 1);
	    
    print_debug("%s: %s protocol registered %s\n", "main-gui", p->display_name,signal->source_plugin_name);
    gp->plugin_name = g_strdup(signal->source_plugin_name);
    gp->p = p;
	    
    gui_user_view_register(gp);

    protocols = g_slist_append(protocols, gp);
}

/* przyjmuje menu ktore moge dodac*/
void handle_register_menu(GGaduSignal *signal)
{
    GGaduMenu *root = signal->data;

    gui_produce_menu_for_factory(root,item_factory,"/Menu",NULL);
}

void handle_unregister_menu(GGaduSignal *signal)
{
    GGaduMenu *root = signal->data;

    if (G_NODE_IS_ROOT(root))
      root = g_node_first_child(root);
    else
      root = g_node_first_sibling(root);

    if (root)
    {
      GGaduMenuItem *it = root->data;
      gchar *path;

      path = g_strdup_printf("/Menu/%s", it->label);
      
      gtk_item_factory_delete_item (item_factory, path);
      g_free (path);
    }
}

void handle_register_userlist_menu(GGaduSignal *signal)
{
    gui_protocol *gp = gui_find_protocol(signal->source_plugin_name,protocols);
	    
    if (gp->userlist_menu == NULL)
		gp->userlist_menu = signal->data;
}
	
void handle_send_userlist(GGaduSignal *signal)
{
    gui_protocol *gp 		= gui_find_protocol(signal->source_plugin_name,protocols);
		
    if (gp && (gp->users_liststore || users_treestore)) 
    {
	gp->userlist = (GSList *)signal->data;
	gui_user_view_add_userlist(gp);
    }
}

void handle_auth_request(GGaduSignal *signal)
{
    GGaduContact *k = signal->data;
    print_debug("%s chce nas dodac do swojej ksiazki adresowej!", k->id);
}

void handle_auth_request_accepted(GGaduSignal *signal)
{
    GGaduContact *k = signal->data;
    print_debug("%s zgodzil sie, zebysmy go mieli w swojej ksiazce adresowej!", k->id);
}

void handle_unauth_request(GGaduSignal *signal)
{
    GGaduContact *k = signal->data;
    print_debug("%s chce nas usunac ze swojej ksiazki adresowej!", k->id);
}

void handle_unauth_request_accepted(GGaduSignal *signal)
{
    GGaduContact *k = signal->data;
    print_debug("%s zgodzil sie, zebysmy go usuneli ze swojej ksiazki!!", k->id);
}


void handle_show_warning(GGaduSignal *signal)
{
    gui_show_message_box(GTK_MESSAGE_WARNING, signal);
}

void handle_show_message(GGaduSignal *signal)
{
    gui_show_message_box(GTK_MESSAGE_INFO, signal);
}
	
void handle_notify(GGaduSignal *signal)
{
    gui_protocol	 *gp = NULL;
    GGaduNotify      	 *n  = (GGaduNotify *)signal->data;
    
    gp = gui_find_protocol(signal->source_plugin_name,protocols);
    
    g_return_if_fail(gp != NULL);
    g_return_if_fail(n != NULL);
    
    gui_user_view_notify(gp, n);
}
	
void handle_disconnected(GGaduSignal *signal)
{
    GGaduStatusPrototype *sp = NULL;
    gui_protocol	 *gp = NULL;
    GdkPixbuf		 *image = NULL;
    GtkWidget		 *status_image;
    gboolean 		 valid;
    GtkTreeModel 	*model;
    gp = gui_find_protocol(signal->source_plugin_name,protocols);
		
    g_return_if_fail(gp != NULL);
		
    sp = gui_find_status_prototype(gp->p, gp->p->offline_status);

    g_return_if_fail(sp != NULL);	    
    	    
    image = create_pixbuf(sp->image);
    model = (tree) ? GTK_TREE_MODEL(users_treestore) : GTK_TREE_MODEL(gp->users_liststore);
    
    if (image == NULL) print_debug("%s : Nie mogê za³adowaæ pixmapy\n", "main-gui");
	if (!tree) {
		valid = gtk_tree_model_get_iter_first(model, &users_iter);
	} else {
		gchar *path = g_strdup_printf("%s:0", gp->tree_path);
		valid = gtk_tree_model_get_iter_from_string(model, &users_iter, path);
		g_free(path);
	}
    
    if (!config_var_get(gui_handler, "show_active")) {			
	while (valid) {
	    GGaduContact	*k;
	    GdkPixbuf	*oldpixbuf;
		
	    gtk_tree_model_get(GTK_TREE_MODEL(model), &users_iter, 0, &oldpixbuf, 2, &k, -1);
	
	    if (k->status != gp->p->offline_status) {
		if (!tree) {
    	    	    gtk_list_store_set(gp->users_liststore, &users_iter, 0, image, -1);
		} else {
	        gtk_tree_store_set(users_treestore, &users_iter, 0, image, -1);
		}
		gdk_pixbuf_unref(oldpixbuf);
	    }
	valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &users_iter);
	}
    } else {
	gui_user_view_clear(gp);
    }
	    
    /* zmiana obrazka na offline */
    status_image = gtk_bin_get_child(GTK_BIN(gp->statuslist_eventbox));
	    
    gtk_image_set_from_pixbuf(GTK_IMAGE(status_image), image);

    gtk_tree_sortable_sort_column_changed(GTK_TREE_SORTABLE(model));
}
	
void handle_change_icon(GGaduSignal *signal)
{
    gui_protocol	 *gp = NULL;
    GdkPixbuf		 *image = NULL;
    GtkWidget		 *status_image;
    GGaduStatusPrototype *sp = signal->data;

    gp = gui_find_protocol(signal->source_plugin_name,protocols);
		
    g_return_if_fail(gp != NULL);
    	    
    image = create_pixbuf(sp->image);

    /* zmiana obrazka na porzadany */
    status_image = gtk_bin_get_child(GTK_BIN(gp->statuslist_eventbox));
	    
    gtk_image_set_from_pixbuf(GTK_IMAGE(status_image), image);
}


void handle_show_search_results(GGaduSignal *signal)
{
    GSList *list = signal->data;
    gui_show_search_results(list, signal->source_plugin_name);
}

void handle_status_changed(GGaduSignal *signal)
{
    gui_protocol	 *gp = NULL;
    GdkPixbuf		 *image = NULL;
    GtkWidget		 *status_image;
    GGaduStatusPrototype *sp = NULL;
    gint		 status;

    gp = gui_find_protocol(signal->source_plugin_name,protocols);
    g_return_if_fail(gp != NULL);

    status = (gint) signal_emit("main-gui", "get current status", NULL, gp->plugin_name);
    sp = gui_find_status_prototype(gp->p, (status) ? status : gp->p->offline_status);
    g_return_if_fail(sp != NULL);
    	    
    image = create_pixbuf(sp->image);
    status_image = gtk_bin_get_child(GTK_BIN(gp->statuslist_eventbox));
    gtk_image_set_from_pixbuf(GTK_IMAGE(status_image), image);
}
