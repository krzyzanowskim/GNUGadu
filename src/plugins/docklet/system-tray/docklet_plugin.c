/* $Id: docklet_plugin.c,v 1.4 2003/11/26 22:00:36 thrulliq Exp $ */

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
#include "menu.h"
#include "support.h"

#include "docklet_plugin.h"
#include "eggtrayicon.h"

GGaduPlugin *handler;

//GtkWidget *status_docklet = NULL;
GtkWidget *pixmap = NULL;
GdkPixbuf *logopix = NULL;
GtkTooltips *tooltips = NULL;
gchar *tooltipstr = NULL;

EggTrayIcon *docklet = NULL;

GGadu_PLUGIN_INIT(DOCKLET_PLUGIN_NAME,GGADU_PLUGIN_TYPE_MISC);

static void create_docklet();

GtkWidget *docklet_create_image(const gchar * directory, const gchar * filename) {
	GtkWidget 	*image		= NULL;
	gchar 		*found_filename = NULL;
	GSList		*dir		= NULL;
	gchar 		*iconsdir	= NULL;

	if (!filename)
	    return NULL;
		
	/* We first try any pixmaps directories set by the application. */
	dir = g_slist_prepend(dir,PACKAGE_DATA_DIR "/pixmaps");
	dir = g_slist_prepend(dir,PACKAGE_DATA_DIR "/pixmaps/emoticons");
#ifdef GGADU_DEBUG
	dir = g_slist_prepend(dir,PACKAGE_SOURCE_DIR "/pixmaps");
	dir = g_slist_prepend(dir,PACKAGE_SOURCE_DIR "/pixmaps/emoticons");
#endif
	if (directory) {
	    iconsdir = g_build_filename(PACKAGE_DATA_DIR, "pixmaps", "icons", directory, NULL);
	    dir = g_slist_prepend(dir, iconsdir);
	}
	
	while (dir) 
	{
		found_filename = check_file_exists((gchar *) dir->data, filename);
		
		if (found_filename)
			break;
			
		dir = dir->next;
	}

	/* If we haven't found the pixmap, try the source directory. */
	if (!found_filename) 
		found_filename = check_file_exists("../pixmaps", filename);

	if (!found_filename) 
	{
		g_warning(_("Couldn't find pixmap file: %s"), filename);
		return NULL;
	}

	image = gtk_image_new_from_file(found_filename);

	g_slist_free(dir);
	g_free(iconsdir);
	
	return image;
}

GdkPixbuf *docklet_create_pixbuf(const gchar * directory, const gchar * filename) {
	gchar 		*found_filename = NULL;
	GdkPixbuf 	*pixbuf		= NULL;
	GSList		*dir		= NULL;
	gchar 		*iconsdir	= NULL;

	print_debug("%s %s\n",directory,filename);

	if (!filename || !filename[0])
		return NULL;

	/* We first try any pixmaps directories set by the application. */
	dir = g_slist_prepend(dir,PACKAGE_DATA_DIR "/pixmaps");
	dir = g_slist_prepend(dir,PACKAGE_DATA_DIR "/pixmaps/emoticons");
#ifdef GGADU_DEBUG
	dir = g_slist_prepend(dir,PACKAGE_SOURCE_DIR "/pixmaps");
	dir = g_slist_prepend(dir,PACKAGE_SOURCE_DIR "/pixmaps/emoticons");
#endif
	if (directory) {
	    iconsdir = g_build_filename(PACKAGE_DATA_DIR, "pixmaps", "icons", directory, NULL);
	    dir = g_slist_prepend(dir, iconsdir);
	}
	
	while (dir) 
	{
		found_filename = check_file_exists((gchar *) dir->data, filename);
		
		if (found_filename)
			break;
			
		dir = dir->next;
	}

	/* If we haven't found the pixmap, try the source directory. */
	if (!found_filename) 
		found_filename = check_file_exists("../pixmaps", filename);

	if (!found_filename) 
	{
		g_warning(_("Couldn't find pixmap file: %s"), filename);
		return NULL;
	}

	pixbuf = gdk_pixbuf_new_from_file(found_filename, NULL);
	
	g_slist_free(dir);
	g_free(iconsdir);	
	
	return pixbuf;
}


GtkWidget *ggadu_new_item_from_stock(GtkWidget *menu, const char *str, const char *icon, GtkSignalFunc sf, gpointer data, guint accel_key, guint accel_mods, char *mod)
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

        if (icon != NULL) {
            image = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_MENU);
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
        }
	
        gtk_widget_show_all(menuitem);

        return menuitem;
}

void docklet_about(GtkWidget *widget, gpointer user_data) {
    signal_emit(DOCKLET_PLUGIN_NAME, "gui show about", NULL, "main-gui");
}

void docklet_quit(GtkWidget *widget, gpointer user_data) {
    signal_emit(DOCKLET_PLUGIN_NAME, "exit", NULL, NULL);
    g_main_loop_quit(config->main_loop);
}

void docklet_menu(GdkEventButton *event) {
    static GtkWidget *menu = NULL;

    menu = gtk_menu_new();
    ggadu_new_item_from_stock(menu, _("About"), GTK_STOCK_DIALOG_INFO, G_CALLBACK(docklet_about), NULL, 0, 0, 0);
    ggadu_new_item_from_stock(menu, _("Quit"), GTK_STOCK_QUIT, G_CALLBACK(docklet_quit), NULL, 0, 0, 0);
    gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
}


void docklet_clicked_cb(GtkWidget * widget, GdkEventButton * ev, gpointer data) 
{
    switch (ev->button) {
      case 1:
	gtk_image_set_from_pixbuf(GTK_IMAGE(pixmap),logopix);
	gtk_widget_show(pixmap);
	gtk_tooltips_set_tip(tooltips, GTK_WIDGET(docklet), tooltipstr, NULL);
	
	signal_emit(GGadu_PLUGIN_NAME, "gui show invisible chats", NULL, "main-gui");
	print_debug("%s : mouse clicked\n",DOCKLET_PLUGIN_NAME);
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


static gboolean
docklet_create_cb(gpointer data)
{
    print_debug("re-create docklet\n");

    create_docklet();
    
    gtk_image_set_from_pixbuf(GTK_IMAGE(pixmap), logopix);

    return FALSE;
}

static void
docklet_destroyed_cb(GtkWidget *widget, gpointer data)
{
	print_debug("tray icon destroyed\n");

	g_object_unref(G_OBJECT(data));
	
	docklet = NULL;
	
	g_idle_add((GSourceFunc)docklet_create_cb, NULL);
}

static void
docklet_embedded_cb(GtkWidget *widget, gpointer data)
{
	print_debug("tray icon embedded\n");
}

static
void create_docklet()
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
	
	g_signal_connect(G_OBJECT(docklet),"embedded",G_CALLBACK(docklet_embedded_cb),docklet);
	g_signal_connect(G_OBJECT(docklet),"destroy",G_CALLBACK(docklet_destroyed_cb),docklet);
	g_signal_connect(G_OBJECT(docklet),"button_press_event",G_CALLBACK(docklet_clicked_cb),pixmap);
	
	g_object_ref(G_OBJECT(docklet));
}

void my_signal_receive(gpointer name, gpointer signal_ptr) {
	GGaduSignal *signal = (GGaduSignal *)signal_ptr;
	
        print_debug("%s : receive signal %d\n",GGadu_PLUGIN_NAME,signal->name);

	if (signal->name == g_quark_from_static_string("docklet set icon")) 
	{
	    GtkImage *image = NULL;
	    GSList   *sigdata = (GSList *)signal->data;
	    gchar    *directory = g_strdup(g_slist_nth_data(sigdata,0));
	    gchar    *filename = g_strdup(g_slist_nth_data(sigdata,1));
	    gchar    *tooltip = g_strdup(g_slist_nth_data(sigdata,2));

	    if (!filename) return;
	    

	    image = (GtkImage *) docklet_create_image(directory, filename);

	    gtk_widget_ref(GTK_WIDGET(image));

	    gtk_image_set_from_pixbuf(GTK_IMAGE(pixmap),gtk_image_get_pixbuf(GTK_IMAGE(image)));
	    
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
	    GSList   *sigdata = signal->data;
	    gchar    *directory = g_strdup(g_slist_nth_data(sigdata,0));
	    gchar    *filename = g_strdup(g_slist_nth_data(sigdata,1));
	    gchar    *tooltip = g_strdup(g_slist_nth_data(sigdata,2));
	    logopix = (GdkPixbuf *)docklet_create_pixbuf(directory, filename);
	    
	    gtk_image_set_from_pixbuf(GTK_IMAGE(pixmap),logopix);
	    
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

    register_signal(handler,"docklet set icon");
    register_signal(handler,"docklet set default icon");
}


GGaduPlugin *initialize_plugin(gpointer conf_ptr) {
    print_debug("%s : initialize\n",GGadu_PLUGIN_NAME);

    /* It have to be initialized */
    gtk_init(NULL, NULL);
    
    GGadu_PLUGIN_ACTIVATE(conf_ptr); /* wazne zeby to bylo tutaj */
    
    handler = (GGaduPlugin *)register_plugin(GGadu_PLUGIN_NAME,_("Dockable icon"));
    
    register_signal_receiver((GGaduPlugin *)handler, (signal_func_ptr)my_signal_receive);

    return handler;
}



void destroy_plugin() {
    print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
    gtk_widget_destroy (GTK_WIDGET(docklet));
}
