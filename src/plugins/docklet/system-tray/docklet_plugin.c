/* $Id: docklet_plugin.c,v 1.16 2004/03/13 14:27:06 krzyzak Exp $ */

/* 
 * Docklet plugin for GNU Gadu 2 
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
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "ggadu_menu.h"
#include "ggadu_support.h"
#include "ggadu_repo.h"

#include "docklet_plugin.h"
#include "eggtrayicon.h"

GGaduPlugin *handler;

/*
GtkWidget *status_docklet = NULL;
*/
static GtkWidget *pixmap = NULL;
static GdkPixbuf *logopix = NULL;
static GtkTooltips *tooltips = NULL;
static gchar *tooltipstr = NULL;
static gchar *icons_dir = NULL;
static EggTrayIcon *docklet = NULL;
static gboolean plugin_destroyed = FALSE;

GGadu_PLUGIN_INIT(DOCKLET_PLUGIN_NAME, GGADU_PLUGIN_TYPE_MISC);

static void create_docklet();

GtkWidget *docklet_create_image(const gchar * directory, const gchar * filename)
{
	GtkWidget *image = NULL;
	gchar *found_filename = NULL;

	if ((found_filename = ggadu_get_image_path(directory, filename)))
	{
		image = gtk_image_new_from_file(found_filename);
		g_free(found_filename);
	}

	return image ? image : NULL;
}

GdkPixbuf *docklet_create_pixbuf(const gchar * directory, const gchar * filename)
{
	gchar *found_filename = NULL;
	GdkPixbuf *pixbuf = NULL;

	print_debug("%s %s\n", directory, filename);

	if (!filename || !filename[0])
		return NULL;

	if ((found_filename = ggadu_get_image_path(directory, filename)))
	{
		pixbuf = gdk_pixbuf_new_from_file(found_filename, NULL);
		g_free(found_filename);
	}

	return pixbuf ? pixbuf : NULL;
}


GtkWidget *ggadu_new_item_from_stock(GtkWidget * menu, const char *str, const char *icon, GtkSignalFunc sf,
				     gpointer data, guint accel_key, guint accel_mods, char *mod)
{
	GtkWidget *menuitem;
	GtkWidget *image;

	if (icon == NULL)
		menuitem = gtk_menu_item_new_with_mnemonic(str);
	else
		menuitem = gtk_image_menu_item_new_with_mnemonic(str);

	if (menu)
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	if (sf)
		g_signal_connect(GTK_OBJECT(menuitem), "activate", sf, data);

	if (icon != NULL)
	{
		image = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	}

	gtk_widget_show_all(menuitem);

	return menuitem;
}

GtkWidget *ggadu_new_item_from_image(GtkWidget * menu, const char *str, const char *icon, GtkSignalFunc sf,
				     gpointer data, guint accel_key, guint accel_mods, char *mod)
{
	GtkWidget *menuitem;
	GtkWidget *image;

	if (icon == NULL)
		menuitem = gtk_menu_item_new_with_mnemonic(str);
	else
		menuitem = gtk_image_menu_item_new_with_mnemonic(str);

	if (menu)
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	if (sf)
		g_signal_connect(GTK_OBJECT(menuitem), "activate", sf, data);

	if (icon != NULL)
	{
		image = docklet_create_image(icons_dir, icon);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	}

	gtk_widget_show_all(menuitem);

	return menuitem;
}

void docklet_status_activate(GtkWidget * widget, gpointer user_data)
{
	GGaduStatusPrototype *sp = user_data;
	GGaduProtocol *p = g_object_get_data(G_OBJECT(widget), "protocol");

	/* FIXME: this is a dirty hack, because we don't really know target plugin name */

	if (p && sp)
		signal_emit(DOCKLET_PLUGIN_NAME, "change status", sp, p->display_name);
}


void docklet_about(GtkWidget * widget, gpointer user_data)
{
	signal_emit(DOCKLET_PLUGIN_NAME, "gui show about", NULL, "main-gui");
}

void docklet_quit(GtkWidget * widget, gpointer user_data)
{
	signal_emit(DOCKLET_PLUGIN_NAME, "exit", NULL, NULL);
	g_main_loop_quit(config->main_loop);
}

static
void go_status(gint type)
{
	GGaduProtocol *p = NULL;
	gpointer key, index;

	if (!ggadu_repo_exists("_protocols_"))
	    return;
	
	index = ggadu_repo_value_first("_protocols_", REPO_VALUE_PROTOCOL, &key);

	while (index) {
	    GGaduStatusPrototype *sp;
	    p = ggadu_repo_find_value("_protocols_", key);
		
	    if (p) {
	        GSList *status_ = NULL;
		GSList *tmp;
	        gint status;
			    
		switch (type) {
		    case STATUS_ONLINE:
		        status_ = p->online_status;
		        break;
		    case STATUS_AWAY:
		        status_ = p->away_status;
		        break;
		    case STATUS_OFFLINE:
		        status_ = p->offline_status;
		        break;
		}
		
		if (status_) {
		    tmp = p->statuslist;
    		    status = (gint) status_->data;
		
		    while (tmp) {
			sp = tmp->data;
			if (sp->status == status) {
				signal_emit(DOCKLET_PLUGIN_NAME, "change status", sp, p->display_name);
				break;
			}
			tmp = tmp->next;
		    }
		}
	    }
	    index = ggadu_repo_value_next("_protocols_", REPO_VALUE_PROTOCOL, &key, index);
	}
}


void go_online(GtkWidget * widget, gpointer user_data)
{
    go_status(STATUS_ONLINE);
}

void go_offline(GtkWidget * widget, gpointer user_data)
{
    go_status(STATUS_OFFLINE);
}

void go_away(GtkWidget * widget, gpointer user_data)
{
    go_status(STATUS_AWAY);
}

void docklet_menu(GdkEventButton * event)
{
	static GtkWidget *menu = NULL;
	gpointer key, index;

	menu = gtk_menu_new();

	if (ggadu_repo_exists("_protocols_"))
	{
		GtkWidget *menuitem;
		GGaduProtocol *p = NULL;
		index = ggadu_repo_value_first("_protocols_", REPO_VALUE_PROTOCOL, &key);

		menuitem = ggadu_new_item_from_image(NULL, _("Go Online (all)"), "online.png", NULL, NULL, 0, 0, 0);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		g_signal_connect(GTK_OBJECT(menuitem), "activate", G_CALLBACK(go_online), NULL);

		menuitem = ggadu_new_item_from_image(NULL, _("Go Away (all)"), "away.png", NULL, NULL, 0, 0, 0);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		g_signal_connect(GTK_OBJECT(menuitem), "activate", G_CALLBACK(go_away), NULL);


		menuitem = ggadu_new_item_from_image(NULL, _("Go Offline (all)"), "offline.png", NULL, NULL, 0, 0, 0);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		g_signal_connect(GTK_OBJECT(menuitem), "activate", G_CALLBACK(go_offline), NULL);

		/* separator */
		menuitem = gtk_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		while (index)
		{
			GGaduStatusPrototype *sp = NULL;
			GSList *tmp = NULL;
			gboolean empty = TRUE;
			p = ggadu_repo_find_value("_protocols_", key);
			
			if (!p || !p->statuslist) 
			{
				index = ggadu_repo_value_next("_protocols_", REPO_VALUE_PROTOCOL, &key, index);
				continue;
			}
			
			/* search if a submenu isn't empty, if so, then do not list here */
			tmp = p->statuslist;
			while (tmp && empty)
			{
				sp = tmp->data;
				
				if (!sp->receive_only)
					empty = FALSE;
				
				tmp = tmp->next;
			}
			
			if (empty)
			{
				index = ggadu_repo_value_next("_protocols_", REPO_VALUE_PROTOCOL, &key, index);
				continue;
			}
			
			sp = p->statuslist->data;
			menuitem = ggadu_new_item_from_image(menu, p->display_name, sp->image, NULL, NULL, 0, 0, 0);

			if (p->statuslist)
			{
				GtkWidget *submenu = gtk_menu_new();
				GtkWidget *subitem;
				
				tmp = p->statuslist;
				while (tmp)
				{
					sp = tmp->data;
					if (!sp->receive_only)
					{
						subitem =
							ggadu_new_item_from_image(submenu, sp->description, sp->image,
										  G_CALLBACK(docklet_status_activate),
										  sp, 0, 0, 0);
						g_object_set_data(G_OBJECT(subitem), "protocol", p);
					}
					tmp = tmp->next;
				}
				gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), submenu);
			}
			index = ggadu_repo_value_next("_protocols_", REPO_VALUE_PROTOCOL, &key, index);
		}

		/* separator */
		menuitem = gtk_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}

	ggadu_new_item_from_stock(menu, _("About"), GTK_STOCK_DIALOG_INFO, G_CALLBACK(docklet_about), NULL, 0, 0, 0);
	ggadu_new_item_from_stock(menu, _("Quit"), GTK_STOCK_QUIT, G_CALLBACK(docklet_quit), NULL, 0, 0, 0);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
}


void docklet_clicked_cb(GtkWidget * widget, GdkEventButton * ev, gpointer data)
{
	switch (ev->button)
	{
	case 1:
		gtk_image_set_from_pixbuf(GTK_IMAGE(pixmap), logopix);
		gtk_widget_show(pixmap);
		gtk_tooltips_set_tip(tooltips, GTK_WIDGET(docklet), tooltipstr, NULL);

		signal_emit(GGadu_PLUGIN_NAME, "gui show invisible chats", NULL, "main-gui");
		print_debug("%s : mouse clicked\n", DOCKLET_PLUGIN_NAME);
		print_debug("%s : left button clicked\n", DOCKLET_PLUGIN_NAME);
		break;
	case 2:
		print_debug("%s : middle button clicked\n", DOCKLET_PLUGIN_NAME);
		break;
	case 3:
		print_debug("%s : right button clicked\n", DOCKLET_PLUGIN_NAME);
		docklet_menu(ev);
		break;
	}
}


static gboolean docklet_create_cb(gpointer data)
{
	print_debug("re-create docklet\n");

	create_docklet();

	gtk_image_set_from_pixbuf(GTK_IMAGE(pixmap), logopix);

	return FALSE;
}

static void docklet_destroyed_cb(GtkWidget * widget, gpointer data)
{
	print_debug("tray icon destroyed\n");

	g_object_unref(G_OBJECT(data));

	docklet = NULL;

	if (!plugin_destroyed)
	    g_idle_add((GSourceFunc) docklet_create_cb, NULL);
}

static void docklet_embedded_cb(GtkWidget * widget, gpointer data)
{
	print_debug("tray icon embedded\n");
}

static void create_docklet()
{
	GtkWidget *eventbox;

	docklet = egg_tray_icon_new("GNU Gadu 2");

	tooltips = gtk_tooltips_new();
	tooltipstr = g_strdup("GNU Gadu 2");
	gtk_tooltips_enable(tooltips);
	gtk_tooltips_set_tip(tooltips, GTK_WIDGET(docklet), tooltipstr, NULL);

	pixmap = gtk_image_new();

	eventbox = gtk_event_box_new();

	gtk_container_add(GTK_CONTAINER(eventbox), pixmap);
	gtk_container_add(GTK_CONTAINER(docklet), eventbox);
	gtk_widget_show_all(GTK_WIDGET(docklet));

	g_signal_connect(G_OBJECT(docklet), "embedded", G_CALLBACK(docklet_embedded_cb), docklet);
	g_signal_connect(G_OBJECT(docklet), "destroy", G_CALLBACK(docklet_destroyed_cb), docklet);
	g_signal_connect(G_OBJECT(docklet), "button_press_event", G_CALLBACK(docklet_clicked_cb), pixmap);

	g_object_ref(G_OBJECT(docklet));
}

void my_signal_receive(gpointer name, gpointer signal_ptr)
{
	GGaduSignal *signal = (GGaduSignal *) signal_ptr;

	print_debug("%s : receive signal %d\n", GGadu_PLUGIN_NAME, signal->name);

	if (signal->name == g_quark_from_static_string("docklet set icon"))
	{
		GtkImage *image = NULL;
		GSList *sigdata = (GSList *) signal->data;
		gchar *directory = g_strdup(g_slist_nth_data(sigdata, 0));
		gchar *filename = g_strdup(g_slist_nth_data(sigdata, 1));
		gchar *tooltip = g_strdup(g_slist_nth_data(sigdata, 2));

		/* save current icons pack dir for other purposes */
		if (icons_dir)
			g_free(icons_dir);

		icons_dir = g_strdup(directory);

		if (!filename)
			return;

		image = (GtkImage *) docklet_create_image(directory, filename);

		gtk_widget_ref(GTK_WIDGET(image));

		gtk_image_set_from_pixbuf(GTK_IMAGE(pixmap), gtk_image_get_pixbuf(GTK_IMAGE(image)));

		gtk_widget_unref(GTK_WIDGET(image));

		gtk_widget_show(pixmap);

		gtk_tooltips_set_tip(tooltips, GTK_WIDGET(docklet), g_strdup(tooltip ? tooltip : "GNU Gadu 2"), NULL);

		signal->data_return = pixmap;

		g_free(directory);
		g_free(filename);
		g_free(tooltip);

		return;
	}


	if (signal->name == g_quark_from_static_string("docklet set default icon"))
	{
		GSList *sigdata = signal->data;
		gchar *directory = g_strdup(g_slist_nth_data(sigdata, 0));
		gchar *filename = g_strdup(g_slist_nth_data(sigdata, 1));
		gchar *tooltip = g_strdup(g_slist_nth_data(sigdata, 2));
		logopix = (GdkPixbuf *) docklet_create_pixbuf(directory, filename);

		/* save current icons pack dir for other purposes */
		if (icons_dir)
			g_free(icons_dir);

		icons_dir = g_strdup(directory);

		gtk_image_set_from_pixbuf(GTK_IMAGE(pixmap), logopix);

		signal->data_return = logopix;

		g_free(tooltipstr);

		tooltipstr = (tooltip ? g_strdup(tooltip) : "GNU Gadu 2");
		gtk_tooltips_set_tip(tooltips, GTK_WIDGET(docklet), tooltipstr, NULL);

		gtk_widget_show(pixmap);

		g_free(directory);
		g_free(filename);
		g_free(tooltip);
		return;
	}

	return;
}

void start_plugin()
{
	create_docklet();

	register_signal(handler, "docklet set icon");
	register_signal(handler, "docklet set default icon");
}


GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
	print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);

	/* It have to be initialized */
	gtk_init(NULL, NULL);

	GGadu_PLUGIN_ACTIVATE(conf_ptr);	/* wazne zeby to bylo tutaj */

	handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("Dockable icon"));

	register_signal_receiver((GGaduPlugin *) handler, (signal_func_ptr) my_signal_receive);

	return handler;
}



void destroy_plugin()
{
	print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
	plugin_destroyed = TRUE;
	gtk_widget_destroy(GTK_WIDGET(docklet));
	docklet = NULL;
	g_free(icons_dir);
	icons_dir = NULL;
}
