#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <libgadu.h>

#include "gg-types.h"
#include "plugins.h"
#include "signals.h"
#include "support.h"
#include "repo.h"

#include "dockapp_plugin.h"

GGaduPlugin *handler;

GtkWidget *status_dockapp = NULL;	//main dockapp window
GtkWidget *icon_dockapp = NULL;
GdkPixmap *launch_pixmap;		//pixmap with content of window
GdkPixmap *launch_mask;			//mask of dockapp window
GdkGC *dock_gc;				//dockapp graphics context
GdkColormap *cmap;
GdkColor somecol;
GdkPixbuf *icon1_img;
GdkPixbuf *icon2_img;
//GdkPixbuf *icon3_img;
guint blinker_id = 0;			//id of blinker timer
guint blink_no = 0;			//show or hide new message icon (even=show, odd=dont show)

//number of visible nicks in dockapp window
#define NNICK 3
gchar prev_nick[NNICK][10] = {"\0\0\0\0\0\0\0\0\0\0", "\0\0\0\0\0\0\0\0\0\0", "\0\0\0\0\0\0\0\0\0\0"};
guint prev_status[NNICK] = {0, 0, 0};

static GdkColor white	= {0, 65535, 65535, 65535};
static GdkColor black	= {0, 0, 0, 0};
static GdkColor clBg	= {0, 65535, 65000, 60000};	//background color
static GdkColor clOffline = {0, 65535,  3000, 3000};	//offline
static GdkColor clAway	= {0, 65535, 3000, 65535};	//away
static GdkColor clOnline = {0,  3000,  3000, 65535};	//online
static GdkColor clUnk	= {0,  3000, 30000,  3000};	//unknown

//3 icons: position and size
GdkRectangle icon1 = {5, 4, 16, 16};
GdkRectangle icon2 = {23, 4, 16, 16};
GdkRectangle icon3 = {41, 4, 16, 16};

GGadu_PLUGIN_INIT(DOCKLET_PLUGIN_NAME,GGADU_PLUGIN_TYPE_MISC);

/* the same like docklet_create_pixbuf */
GdkPixbuf *dockapp_create_pixbuf(const gchar * directory, const gchar * filename) {
	gchar 		*found_filename = NULL;
	GdkPixbuf 	*pixbuf		= NULL;
	GSList		*dir		= NULL;
	gchar 		*iconsdir	= NULL;

	print_debug("%s: dockapp_create_pixbuf - %s %s\n",GGadu_PLUGIN_NAME, directory, filename);

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
		g_slist_free(dir);
		g_free(iconsdir);	
		return NULL;
	}

	pixbuf = gdk_pixbuf_new_from_file(found_filename, NULL);

	g_free(found_filename);
	g_slist_free(dir);
	g_free(iconsdir);	
	
	return pixbuf;
}

/* Redraw pixmap with content on dockapp window */
void redraw_dockapp()
{
	print_debug("%s : redraw_dockapp\n",GGadu_PLUGIN_NAME);
        gdk_draw_pixmap(icon_dockapp->window, dock_gc, launch_pixmap, 0, 0, 0, 0, 64, 64);
}

/* Draw pixmap */
void draw_pixmap()
{
	GdkFont *font;
	int i;
	print_debug("%s : draw_pixmap\n",GGadu_PLUGIN_NAME);

	/* draw a background */
        gdk_gc_set_foreground(dock_gc, &clBg);
        gdk_draw_rectangle(launch_pixmap, dock_gc, TRUE, 3, 3, 57, 57);

	/* draw icons */
	if (icon1_img != NULL) 
	    gdk_draw_pixbuf(launch_pixmap, dock_gc, icon1_img, 0, 0, icon1.x, icon1.y, icon1.width, icon1.height, GDK_RGB_DITHER_NONE, 0, 0);
	/* draw 'new message' icon */
	if ((icon2_img != NULL) && ((blink_no%2) == 1))
	    gdk_draw_pixbuf(launch_pixmap, dock_gc, icon2_img, 0, 0, icon2.x, icon2.y, icon2.width, icon2.height, GDK_RGB_DITHER_NONE, 0, 0);

	font = gdk_font_load("-misc-fixed-medium-r-normal-*-10-*-*-*-*-*-iso8859-2");

	/* set color and draw notify (last 3 nicks) */
	for (i=0; i<NNICK; i++) {
	    switch (prev_status[i]) {
		case GG_STATUS_NOT_AVAIL:
		case GG_STATUS_NOT_AVAIL_DESCR:
    		    gdk_gc_set_foreground(dock_gc, &clOffline);
		    break;
		case GG_STATUS_BUSY:
		case GG_STATUS_BUSY_DESCR:
    		    gdk_gc_set_foreground(dock_gc, &clAway);
		    break;
		case GG_STATUS_AVAIL:
		case GG_STATUS_AVAIL_DESCR:
    		    gdk_gc_set_foreground(dock_gc, &clOnline);
		    break;
		default:
    		    gdk_gc_set_foreground(dock_gc, &clUnk);
		    break;
	    }
	    gdk_draw_text(launch_pixmap, font, dock_gc, 6, 34+(i*10), prev_nick[i], strlen(prev_nick[i]));
	}
    gdk_font_unref(font);
}

/* from gg1
return true if clicked inside btn rectangle
*/
int btn_clicked(GdkRectangle *btn, int x, int y)
{
        return (x >= btn->x && x <= btn->x+btn->width &&
                y >= btn->y && y <= btn->y+btn->height);
}

/* do something if clicked on dockapp */
void dockapp_clicked(GtkWidget * widget, GdkEventButton * ev, gpointer data)
{
    /* left button clicked */
    if (ev->button==1) {
        print_debug("%s : left button clicked\n", GGadu_PLUGIN_NAME);

	/* on icon1 or icon2 ? */
	if (btn_clicked(&icon1, ev->x, ev->y) || btn_clicked(&icon2, ev->x, ev->y)) {
	    /* yes: stop blinking and hide 'new message' icon, redraw dockapp,
		say GUI to show invisible chats or main window */
	    if (blinker_id >0) {
		g_source_remove(blinker_id);
		blinker_id = 0;
	    }
	    blink_no = 0;
	    if (icon2_img != NULL) {
		g_object_unref(icon2_img);
		icon2_img = NULL;
	    }
	    draw_pixmap();
	    redraw_dockapp();
	    signal_emit_full(GGadu_PLUGIN_NAME, "gui show invisible chats", NULL, "main-gui", NULL);
	}
    }
}

/* function run from blinker timer */
gboolean msgicon_blink(gpointer data) {
    print_debug("%s: msgicon_blink\n",GGadu_PLUGIN_NAME);
    /* if blinking decrease number and redraw dockapp */
    if (blink_no > 1) {
	blink_no--;
	draw_pixmap();
	redraw_dockapp();
	return TRUE;	//timer still running
    }
    /* else stop timer */
    return FALSE;
}

/* Create dockapp window */
void create_dockapp()
{
	print_debug("%s : create_docapp\n",GGadu_PLUGIN_NAME);

	GdkGC *mask_gc;			//graphics context of mask

	/* all colors used in dockapp must be here */
	cmap = gdk_colormap_get_system ();
        gdk_colormap_alloc_color(cmap, &white, FALSE, TRUE);
        gdk_colormap_alloc_color(cmap, &black, FALSE, TRUE);
	gdk_colormap_alloc_color(cmap, &clBg, FALSE, TRUE);
	gdk_colormap_alloc_color(cmap, &clOffline, FALSE, TRUE);
	gdk_colormap_alloc_color(cmap, &clAway, FALSE, TRUE);
	gdk_colormap_alloc_color(cmap, &clOnline, FALSE, TRUE);
	gdk_colormap_alloc_color(cmap, &clUnk, FALSE, TRUE);

	/* prepare dockapp window */
	status_dockapp = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_wmclass(GTK_WINDOW(status_dockapp), "GM_Statusdockapp", "gg2");
	gtk_window_set_title(GTK_WINDOW(status_dockapp), "GNU Gadu 2");
	gtk_widget_set_size_request(status_dockapp, 64, 64);
	gtk_window_set_resizable(GTK_WINDOW(status_dockapp), FALSE);
	gtk_window_set_type_hint(GTK_WINDOW(status_dockapp), GDK_WINDOW_TYPE_HINT_DOCK);
	gtk_widget_set_app_paintable(status_dockapp, TRUE);
	gtk_widget_realize(status_dockapp);

        /* create the icon */
        icon_dockapp = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_widget_set_size_request(icon_dockapp, 64, 64);
        gtk_widget_set_uposition(icon_dockapp, 0, 0);
        gtk_widget_set_app_paintable(icon_dockapp, TRUE);

	/* must be before realize */
	gtk_widget_set_events(icon_dockapp, GDK_BUTTON_PRESS_MASK | GDK_EXPOSURE_MASK);
	g_signal_connect(G_OBJECT(icon_dockapp), "button-press-event", G_CALLBACK(dockapp_clicked), NULL);
	g_signal_connect(G_OBJECT(icon_dockapp), "expose-event", G_CALLBACK(redraw_dockapp), NULL);

	gtk_widget_realize(icon_dockapp);

        /* create the pixmap and the mask */
        launch_mask = gdk_pixmap_new(status_dockapp->window, 64, 64, 1);
        launch_pixmap = gdk_pixmap_new(status_dockapp->window, 64, 64, -1);

        /* create graphic contexts */
        dock_gc = gdk_gc_new(icon_dockapp->window);
        mask_gc = gdk_gc_new(launch_mask);

        /* draw the icon mask */
        gdk_gc_set_foreground(mask_gc, &black);
        gdk_draw_rectangle(launch_mask, mask_gc, TRUE,0,0,-1,-1);
        gdk_gc_set_foreground(mask_gc, &white);
        gdk_draw_rectangle(launch_mask, mask_gc, TRUE, 3, 3, 57, 57);

	draw_pixmap();

	gtk_widget_shape_combine_mask(status_dockapp, launch_mask, 0, 0);

	redraw_dockapp();

	gdk_window_set_icon(status_dockapp->window, NULL, launch_pixmap, launch_mask);
	gdk_window_reparent(icon_dockapp->window, status_dockapp->window, 0, 0);
	g_object_unref(mask_gc);

	gtk_widget_show_all(icon_dockapp);
	gtk_widget_show_all(status_dockapp);
}

void my_signal_receive(gpointer name, gpointer signal_ptr) {
	GGaduSignal *signal = (GGaduSignal *)signal_ptr;
        GSList *sigdata = (GSList *)signal->data;

        print_debug("%s : receive signal %d\n",GGadu_PLUGIN_NAME,signal->name);

	/* my status changed */
	if (signal->name == g_quark_from_static_string("dockapp status changed")) {
	    gchar *plugin_name = g_strdup(g_slist_nth_data(sigdata,0));
	    /* stop if wrong protocol */
	    if (g_strcasecmp(plugin_name, "gadu-gadu")) {
		g_free(plugin_name);
	        return;
	    }
	    icon1_img = g_slist_nth_data(sigdata,1);
	    draw_pixmap();
	    redraw_dockapp();
	    g_free(plugin_name);
	    return;
	}

	/* some user status changed */
	if (signal->name == g_quark_from_static_string("dockapp user notify")) {
	    int i;
	    gchar *plugin_name = g_strdup(g_slist_nth_data(sigdata,0));
	    /* stop if wrong protocol */
	    if (g_strcasecmp(plugin_name, "gadu-gadu")) {
		g_free(plugin_name);
		return;
	    }
	    gchar *nick = g_strdup(g_slist_nth_data(sigdata,1));
	    guint status = (guint) g_slist_nth_data(sigdata,2);
	    /* shift existent notifies */
	    for (i=0; i<NNICK-1; i++) {
		g_strlcpy(prev_nick[i], prev_nick[i+1], 9);
		prev_status[i] = prev_status[i+1];
	    }
	    /* add new notify */
	    g_strlcpy(prev_nick[NNICK-1], nick, 9);
	    prev_status[NNICK-1] = status;
	    draw_pixmap();
	    redraw_dockapp();
	    g_free(plugin_name);
	    g_free(nick);
	    return;
	}

	/* new message received */
	if (signal->name == g_quark_from_static_string("docklet set icon")) {
	    gchar *directory = g_strdup(g_slist_nth_data(sigdata,0));
	    gchar *filename = g_strdup(g_slist_nth_data(sigdata,1));

	    if (filename) {
		/* destroy old and create new icon */
		if (icon2_img != NULL)
		    g_object_unref(icon2_img);
		icon2_img = dockapp_create_pixbuf(directory, filename);
		/* stop previous blinker if exists */
		if (blinker_id >0)
		    g_source_remove(blinker_id);
		/* blink 2 times */
		blink_no = 5;
		blinker_id = g_timeout_add(500, (GtkFunction) msgicon_blink, NULL);

		draw_pixmap();
		redraw_dockapp();
		
		/* something (GUI plugin?) need to receive some data to not display chat window */
		signal->data_return = icon2_img;
		
	    }
	    g_free(directory);
	    g_free(filename);
	    return;
	}
}

void start_plugin() 
{
    print_debug("%s : start\n",GGadu_PLUGIN_NAME);

    create_dockapp();

    /* signals */
    /* my status changed */
    register_signal(handler, "dockapp status changed");
    /* status of some user changed */
    register_signal(handler, "dockapp user notify");

    /* join to docklet signal */
    /* message received */
    register_signal(handler,"docklet set icon");
}

/* Initialize dockapp plugin
Return plugin handler */
GGaduPlugin *initialize_plugin(gpointer conf_ptr) {
    print_debug("%s : initialize\n",GGadu_PLUGIN_NAME);

    /* Gtk have to be initialized */
    gtk_init(NULL, NULL);
    GGadu_PLUGIN_ACTIVATE(conf_ptr); /* It must be here */
    handler = (GGaduPlugin *)register_plugin(GGadu_PLUGIN_NAME,_("WindowMaker Dockapp"));
    register_signal_receiver((GGaduPlugin *)handler, (signal_func_ptr)my_signal_receive);
    return handler;
}

void destroy_plugin() {
    print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
    gtk_widget_destroy (GTK_WIDGET(status_dockapp));
    gtk_widget_destroy (GTK_WIDGET(icon_dockapp));
    g_object_unref(launch_pixmap);
    launch_pixmap = NULL;
    g_object_unref(launch_mask);
    launch_mask = NULL;
    g_object_unref(dock_gc);
    dock_gc = NULL;
    g_object_unref(cmap);
    cmap = NULL;
    g_object_unref(icon1_img);
    icon1_img = NULL;
    g_object_unref(icon2_img);
    icon2_img = NULL;
    //g_object_unref(icon3_img);
    //icon3_img = NULL;

    /* remove blinker function if exist */
    if (blinker_id >0) {
	g_source_remove(blinker_id);
	blinker_id = 0;
    }
    blink_no = 0;
}
