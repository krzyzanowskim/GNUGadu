/* $Id: gui_handlers.c,v 1.73 2006/08/07 20:02:13 krzyzak Exp $ */

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

#ifdef PERL_EMBED
#include <EXTERN.h>
#include <perl.h>
#endif

#include "ggadu_types.h"
#include "plugins.h"
#include "ggadu_conf.h"
#include "signals.h"
#include "ggadu_support.h"
#include "ggadu_menu.h"
#include "ggadu_repo.h"
#include "GUI_plugin.h"
#include "gui_chat.h"
#include "gui_support.h"
#include "gui_main.h"
#include "gui_search.h"
#include "gui_dialogs.h"
#include "gui_userview.h"
#include "gui_handlers.h"

GtkTreeIter users_iter;
GtkItemFactory *item_factory = NULL;
extern GtkTreeStore *users_treestore;
extern GGaduPlugin *gui_handler;
extern GSList *protocols;
extern GSList *emoticons;
extern GtkWidget *window;
extern gboolean tree;
extern GSList *invisible_chats;

void handle_show_dialog(GGaduSignal * signal)
{
	gui_show_dialog(signal, FALSE);
}

void handle_show_window_with_text(GGaduSignal * signal)
{
	gui_show_window_with_text(signal);
}

void handle_show_about(GGaduSignal * signal)
{
	gui_show_about(signal);
}

#ifdef PERL_EMBED
void perl_gui_msg_receive(GGaduSignal * signal, gchar * perl_func, void *pperl)
{
	int count, junk;
	GGaduMsg *msg = (GGaduMsg *) signal->data;
	SV *sv_name;
	SV *sv_src;
	SV *sv_dst;
	SV *sv_data_id;
	SV *sv_data_message;
	SV *sv_data_class;
	SV *sv_data_time;
	PerlInterpreter *my_perl = (PerlInterpreter *) pperl;

	dSP;

	/* empty message, leave */
	if (!msg->message)
		return;

	ENTER;
	SAVETMPS;

	sv_name = sv_2mortal(newSVpv(g_quark_to_string(signal->name), 0));
	sv_src = sv_2mortal(newSVpv(signal->source_plugin_name, 0));
	if (signal->destination_plugin_name)
		sv_dst = sv_2mortal(newSVpv(signal->destination_plugin_name, 0));
	else
		sv_dst = sv_2mortal(newSVpv("", 0));
	sv_data_id = sv_2mortal(newSVpv(msg->id, 0));
	sv_data_message = sv_2mortal(newSVpv(msg->message, 0));
	sv_data_class = sv_2mortal(newSViv(msg->class));
	sv_data_time = sv_2mortal(newSViv(msg->time));

	PUSHMARK(SP);
	XPUSHs(sv_name);
	XPUSHs(sv_src);
	XPUSHs(sv_dst);
	XPUSHs(sv_data_id);
	XPUSHs(sv_data_message);
	XPUSHs(sv_data_class);
	XPUSHs(sv_data_time);
	PUTBACK;

	count = call_pv(perl_func, G_DISCARD);

	if (count == 0)
	{
		gchar *dst;
		signal->name = g_quark_from_string(SvPV(sv_name, junk));
		signal->source_plugin_name = g_strdup(SvPV(sv_src, junk));
		dst = SvPV(sv_dst, junk);
		if (dst[0] != '\0')
			signal->destination_plugin_name = g_strdup(dst);
		msg->id = g_strdup(SvPV(sv_data_id, junk));
		msg->message = g_strdup(SvPV(sv_data_message, junk));
		msg->class = SvIV(sv_data_class);
		msg->time = SvIV(sv_data_time);
	}

	FREETMPS;
	LEAVE;
}
#endif

void handle_msg_receive(GGaduSignal * signal)
{
	gui_msg_receive(signal);
}

void handle_show_invisible_chats(GGaduSignal * signal)
{
	GSList *tmp = invisible_chats;

	if (!invisible_chats)
	{
		gui_show_hide_window();
		gtk_window_move(GTK_WINDOW(window), (gint) ggadu_config_var_get(gui_handler, "left"),
				(gint) ggadu_config_var_get(gui_handler, "top"));

		return;
	}

	while (tmp)
	{
		if (tmp->data && G_IS_OBJECT(tmp->data))
		{
			GtkWidget *chat = GTK_WIDGET(tmp->data);
			GtkWidget *win = g_object_get_data(G_OBJECT(chat), "top_window");
			GtkWidget *input = g_object_get_data(G_OBJECT(chat), "input");

			if (win)
				gtk_widget_show_all(win);

			if (input)
				gtk_widget_grab_focus(GTK_WIDGET(input));
		}
		tmp = tmp->next;
	}

	g_slist_free(invisible_chats);
	invisible_chats = NULL;
}

void handle_register_protocol(GGaduSignal * signal)
{
	GGaduProtocol *p = signal->data;
	gui_protocol *gp = g_new0(gui_protocol, 1);

	print_debug("%s: %s protocol registered %s\n", "main-gui", p->display_name, signal->source_plugin_name);
	gp->plugin_name = g_strdup(signal->source_plugin_name);
	gp->p = p;
	gp->blinker = -1;

	gui_user_view_register(gp);

	protocols = g_slist_append(protocols, gp);
}

void handle_unregister_protocol(GGaduSignal * signal)
{
	GGaduProtocol *p = signal->data;
	gui_protocol *gp;
	GSList *tmp;

	print_debug("%s: %s protocol unregistered %s\n", "main-gui", p->display_name, signal->source_plugin_name);
	tmp = protocols;
	while (tmp)
	{
		gp = (gui_protocol *) tmp->data;
		if (!ggadu_strcasecmp(gp->plugin_name, (char *) signal->source_plugin_name))
		{
			gui_user_view_unregister(gp);
			protocols = g_slist_remove(protocols, gp);
			return;
		}
		tmp = tmp->next;
	}
}

/* przyjmuje menu ktore moge dodac*/
void handle_register_menu(GGaduSignal * signal)
{
	GGaduMenu *root = signal->data;

	gui_produce_menu_for_factory(root, item_factory, "/Menu", NULL);
}

void handle_unregister_menu(GGaduSignal * signal)
{
	GGaduMenu *root = signal->data;

	if (G_NODE_IS_ROOT(root))
		root = g_node_first_child(root);
	else
		root = g_node_first_sibling(root);

	if (root)
	{
		GGaduMenuItem *it = root->data;
		int len = strlen(it->label);
		int i, c;
		gchar *label = g_new0(char, len + 1);
		gchar *path;

		for (i = 0, c = 0; i < len; i++)
		{
			if (it->label[i] != '_')
			{
				label[c] = it->label[i];
				c++;
			}
		}

		path = g_strdup_printf("/Menu/%s", label);
		g_free(label);

		gtk_item_factory_delete_item(item_factory, path);
		g_free(path);
		ggadu_menu_free(root);
	}
}

/* remember about repo, repo is more important than userlist send as a signal argument */
void handle_send_userlist(GGaduSignal * signal)
{
	gui_protocol *gp = gui_find_protocol(signal->source_plugin_name, protocols);
	/* repo support hmmm */

	if (gp && (gp->users_liststore || users_treestore))
	{
		g_slist_free(gp->userlist);
		gp->userlist = ggadu_repo_get_as_slist(signal->source_plugin_name, REPO_VALUE_CONTACT);
		gui_user_view_add_userlist(gp);
	}
}

void handle_show_warning(GGaduSignal * signal)
{
	gui_show_message_box(GTK_MESSAGE_WARNING, signal);
}

void handle_show_message(GGaduSignal * signal)
{
	gui_show_message_box(GTK_MESSAGE_INFO, signal);
}

void handle_notify(GGaduSignal * signal)
{
	gui_protocol *gp = NULL;
	GGaduNotify *n = (GGaduNotify *) signal->data;

	g_return_if_fail(n != NULL);

	gp = gui_find_protocol(signal->source_plugin_name, protocols);

	g_return_if_fail(gp != NULL);

	gui_user_view_notify(gp, n);
}

void handle_disconnected(GGaduSignal * signal)
{
	GGaduStatusPrototype *sp = NULL;
	gui_protocol *gp = NULL;
	GdkPixbuf *image = NULL;
	GtkWidget *status_image;
	gboolean valid = FALSE;
	GtkTreeModel *model;
	GtkTreeIter parent_iter;

	gp = gui_find_protocol(signal->source_plugin_name, protocols);

	g_return_if_fail(gp != NULL);
	g_return_if_fail(gp->p->offline_status != NULL);

	sp = ggadu_find_status_prototype(gp->p, *(int *) &gp->p->offline_status->data);

	g_return_if_fail(sp != NULL);

	if (gp->blinker > 0)
		g_source_remove(gp->blinker);

	gp->blinker = -1;

	image = create_pixbuf(sp->image);
	model = (tree) ? GTK_TREE_MODEL(users_treestore) : GTK_TREE_MODEL(gp->users_liststore);

	if (image == NULL)
		print_debug("%s : Nie mogê za³adowaæ pixmapy\n", "main-gui");

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

	if (!ggadu_config_var_get(gui_handler, "show_active"))
	{
		while (valid)
		{
			GGaduContact *k;
			GdkPixbuf *oldpixbuf;

			gtk_tree_model_get(GTK_TREE_MODEL(model), &users_iter, 0, &oldpixbuf, 2, &k, -1);

			/* This shouldn'be be like this */
			/* I expect that first entry in offline_status is offline without description */
			/* only in such situation I can figure out which one is pure offline status */
			/* if (!ggadu_is_in_status (k->status, gp->p->offline_status)) */
/*
            print_debug("%d %d\n",k->status,*(gint *)&gp->p->offline_status->data);
*/
			if (k->status != *(gint *) & gp->p->offline_status->data)
			{
				if (!tree)
				{
					gtk_list_store_set(gp->users_liststore, &users_iter, 0, image, -1);
				}
				else
				{
					gtk_tree_store_set(users_treestore, &users_iter, 0, image, -1);
				}
				gdk_pixbuf_unref(oldpixbuf);
			}
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &users_iter);
		}
	}
	else
	{
		gui_user_view_clear(gp);
	}

	if (tree)
	{
		gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(model), &parent_iter, gp->tree_path);
		gtk_tree_store_set(users_treestore, &parent_iter, 1,
				   g_strdup_printf("%s (%d/%d)", gp->p->display_name, 0, g_slist_length(gp->userlist)),
				   -1);
	}

	/* zmiana obrazka na offline */
	status_image = gtk_bin_get_child(GTK_BIN(gp->statuslist_eventbox));

	gtk_image_set_from_pixbuf(GTK_IMAGE(status_image), image);

	gtk_tree_sortable_sort_column_changed(GTK_TREE_SORTABLE(model));

	if (gp->blinker_image1)
		gdk_pixbuf_unref(gp->blinker_image1);

	if (gp->blinker_image2)
		gdk_pixbuf_unref(gp->blinker_image2);

	gp->blinker_image1 = NULL;
	gp->blinker_image2 = NULL;
	
	gtk_tooltips_set_tip(gp->tooltips, gp->statuslist_eventbox, sp->description, NULL);
	GGaduStatusPrototype_free(sp);
}

void handle_show_search_results(GGaduSignal * signal)
{
	GSList *list = signal->data;
	gui_show_search_results(list, signal->source_plugin_name);
}

void handle_null(GGaduSignal * signal)
{
	print_debug("Doing NULL for %s\n", signal ? signal->source_plugin_name : "[error]");
	return;
}

void handle_status_changed(GGaduSignal * signal)
{
	static GMutex *thread_status_changed_mutex = NULL;
	gui_protocol *gp = NULL;
	GdkPixbuf *image = NULL;
	GtkWidget *status_image = NULL;
	GGaduStatusPrototype *sp = signal->data;
	gchar *tipdesc = NULL;
	print_debug("handle_status_changed start");

	if (!thread_status_changed_mutex)
		thread_status_changed_mutex = g_mutex_new();

	g_mutex_lock(thread_status_changed_mutex);

	if (!sp) goto out;

	gp = gui_find_protocol(signal->source_plugin_name, protocols);
	if (!gp) goto out;

	if (gp->blinker > 0)
		g_source_remove(gp->blinker);
		
	gp->blinker = -1;

	print_debug("requested pixbuf:");
	print_debug("is:%s",sp->image);
	image = create_pixbuf(sp->image);
	status_image = gtk_bin_get_child(GTK_BIN(gp->statuslist_eventbox));
	gtk_image_set_from_pixbuf(GTK_IMAGE(status_image), image);

	if (gp->blinker_image1)
		gdk_pixbuf_unref(gp->blinker_image1);
	if (gp->blinker_image2)
		gdk_pixbuf_unref(gp->blinker_image2);
		
	gp->blinker_image1 = NULL;
	gp->blinker_image2 = NULL;

	if (sp->status_description && (strlen(sp->status_description) > 0))
	    tipdesc = g_strconcat(sp->description," - ",sp->status_description,NULL);
	else
	    tipdesc = g_strconcat(sp->description,sp->status_description,NULL);
	    
	gtk_tooltips_set_tip(gp->tooltips, gp->statuslist_eventbox, tipdesc, NULL);
	g_free(tipdesc);
	/* GGaduStatusPrototype_free(sp); */
out:
	print_debug("handle_status_changed end");
	g_mutex_unlock(thread_status_changed_mutex);
}

void notify_callback(gchar * repo_name, gpointer key, gint actions)
{
	gui_protocol *gp = NULL;
	GGaduContact *k = NULL;
	GGaduNotify *n = NULL;

	gp = gui_find_protocol(repo_name, protocols);
	k = ggadu_repo_find_value(repo_name, key);

	if (!gp)
		return;
		
	g_return_if_fail(k != NULL);

	n = g_new0(GGaduNotify, 1);
	n->id = k->id;
	n->status = k->status;

	gui_user_view_notify(gp, n);
	g_free(n);
}


