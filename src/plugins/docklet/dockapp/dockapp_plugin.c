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
#include "dialog.h"
#include "menu.h"

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

static GGaduMenu *dockapp_menu;
static gchar *this_configdir = NULL;

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
		case GGADU_DOCKAPP_STATUS_OFFLINE:
    		    gdk_gc_set_foreground(dock_gc, &clOffline);
		    break;
		case GGADU_DOCKAPP_STATUS_AWAY:
    		    gdk_gc_set_foreground(dock_gc, &clAway);
		    break;
		case GGADU_DOCKAPP_STATUS_ONLINE:
    		    gdk_gc_set_foreground(dock_gc, &clOnline);
		    break;
		default:
    		    gdk_gc_set_foreground(dock_gc, &clUnk);
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

/* create dockapp preferences window */
gpointer dockapp_preferences_action ()
{
    GSList *pluginlist_names = NULL;		/* list of plugins */
    gpointer key = NULL, index = NULL;
    gchar *utf = NULL, *current_proto;
    GGaduProtocol *proto;

    /* prepare list of protocol plugins */
    current_proto = config_var_get(handler, "dockapp_protocol");
    if (current_proto)
	pluginlist_names = g_slist_append(pluginlist_names, current_proto);
    index = ggadu_repo_value_first("_protocols_", REPO_VALUE_PROTOCOL, (gpointer *)&key);
    while (index) {
	/* key == name but better find display_name */
	proto = ggadu_repo_find_value("_protocols_", key);
	to_utf8("ISO-8859-2", proto->display_name, utf);
	if (!current_proto || ggadu_strcasecmp(utf, current_proto))
	    pluginlist_names = g_slist_append(pluginlist_names, utf);
	index = ggadu_repo_value_next("_protocols_", REPO_VALUE_PROTOCOL, (gpointer *)&key, index);
    }

    /* create dialog */
    GGaduDialog *d = ggadu_dialog_new();
    ggadu_dialog_set_title(d, _("Dockapp plugin configuration"));
    ggadu_dialog_set_type(d, GGADU_DIALOG_CONFIG);
    ggadu_dialog_callback_signal(d, "update config");

    ggadu_dialog_add_entry(&(d->optlist), GGADU_DOCKAPP_CONFIG_PROTOCOL, _("Protocol"), VAR_LIST,
			    pluginlist_names, VAR_FLAG_NONE);

    signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");

    g_slist_free(pluginlist_names);
    return NULL;
}

/* create menu for this plugin */
GGaduMenu *build_plugin_menu()
{
    GGaduMenu *root = ggadu_menu_create();
    GGaduMenu *item_gg = ggadu_menu_add_item(root, "Dockapp", NULL, NULL);
    
    ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Preferences"), dockapp_preferences_action, NULL));

    return root;
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

/* do when user notify changed */
void notify_callback(gchar * repo_name, gpointer key, gint actions)
{
    print_debug("%s : notify on protocol %s\n", GGadu_PLUGIN_NAME, repo_name);

    gchar *dockapp_protocol, *utf;
    GGaduContact *k = NULL;
    GGaduProtocol *p = NULL;
    int i;
    gpointer key2 = NULL, index = NULL;

    /* stop if no dockapp_protocol set or notify from other protocol */
    dockapp_protocol = config_var_get(handler, "dockapp_protocol");
    if (!dockapp_protocol || ggadu_strcasecmp(dockapp_protocol, repo_name))
	return;

    /* stop if unknown contact */
    if ((k = ggadu_repo_find_value(repo_name, key)) == NULL)
	return;
    
    /* shift existent notifies */
    for (i=0; i<NNICK-1; i++) {
	g_strlcpy(prev_nick[i], prev_nick[i+1], 9);
	prev_status[i] = prev_status[i+1];
    }

    /* add new notify */
    g_strlcpy(prev_nick[NNICK-1], (k->nick != NULL)?k->nick:k->id, 9);

    /* find protocol in repo */
    index = ggadu_repo_value_first("_protocols_", REPO_VALUE_PROTOCOL, (gpointer *)&key2);
    while (index) {
	p = ggadu_repo_find_value("_protocols_", key2);
	to_utf8("ISO-8859-2", p->display_name, utf);
	if (!ggadu_strcasecmp(utf, dockapp_protocol))
	    break;
	index = ggadu_repo_value_next("_protocols_", REPO_VALUE_PROTOCOL, (gpointer *)&key2, index);
    }
    if (!index)
	return;		/* unknown protocol */

    /* search for status online */
    if (g_slist_find(p->online_status, (gint *)k->status) != NULL)
	prev_status[NNICK-1] = GGADU_DOCKAPP_STATUS_ONLINE;
    /* search for status away */
    else if (g_slist_find(p->away_status, (gint *)k->status) != NULL)
	prev_status[NNICK-1] = GGADU_DOCKAPP_STATUS_AWAY;
    /* search for status offline */
    else if (g_slist_find(p->offline_status, (gint *)k->status) != NULL)
	prev_status[NNICK-1] = GGADU_DOCKAPP_STATUS_OFFLINE;
    /* not found? set unknown status */
    else
	prev_status[NNICK-1] = GGADU_DOCKAPP_STATUS_UNKNOWN;

    draw_pixmap();
    redraw_dockapp();
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

	/* read new settings, remember and save */
	if (signal->name == g_quark_from_static_string("update config")) {
	    GGaduDialog *d = signal->data;
	    GSList *tmplist = d->optlist;

	    if (d->response == GGADU_OK) {
		while (tmplist) {
		    GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
		    switch (kv->key) {
			case GGADU_DOCKAPP_CONFIG_PROTOCOL: {
			    gchar *iso = NULL;
			    int i;
			    print_debug("changing var setting dockapp_protocol to %s\n", kv->value);
			    from_utf8("ISO-8859-2", kv->value, iso);
			    if (ggadu_strcasecmp(config_var_get(handler, "dockapp_protocol"),iso)) {
				/* dockapp_plugin really changed, clear existent notifies */
				for (i=0; i<NNICK; i++)
				    g_strlcpy(prev_nick[i], "\0\0\0\0\0\0\0\0\0\0", 9);
			    };
			    config_var_set(handler, "dockapp_protocol", iso);
			    break;
			}
		    }
		    tmplist = tmplist->next;
		}
		config_save(handler);
		draw_pixmap();
		redraw_dockapp();
	    }
	    GGaduDialog_free(d);
	    return;
	}
}

void start_plugin() 
{
    print_debug("%s : start\n",GGadu_PLUGIN_NAME);

    /* create and register menu for this plugin */
    dockapp_menu = build_plugin_menu();
    signal_emit(GGadu_PLUGIN_NAME, "gui register menu", dockapp_menu, "main-gui");

    create_dockapp();

    /* signals */
    /* my status changed */
    //register_signal(handler, "dockapp status changed");
    /* status of some user changed */
    //register_signal(handler, "dockapp user notify");

    register_signal(handler, "update config");

    /* join to docklet signal */
    /* message received */
    register_signal(handler,"docklet set icon");
    
    ggadu_repo_watch_add(NULL, REPO_ACTION_VALUE_CHANGE, REPO_VALUE_CONTACT, notify_callback);
}

/* Initialize dockapp plugin
Return plugin handler */
GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
    gchar *path = NULL;

    print_debug("%s : initialize\n",GGadu_PLUGIN_NAME);
    
    /* Gtk have to be initialized */
    gtk_init(NULL, NULL);
    GGadu_PLUGIN_ACTIVATE(conf_ptr); /* It must be here */
    handler = (GGaduPlugin *)register_plugin(GGadu_PLUGIN_NAME,_("WindowMaker Dockapp"));
    register_signal_receiver((GGaduPlugin *)handler, (signal_func_ptr)my_signal_receive);

    /* configure plugin */
    config_var_add(handler, "dockapp_protocol", VAR_STR);
    
    if (g_getenv("CONFIG_DIR"))
	this_configdir = g_build_filename(g_get_home_dir(), g_getenv("CONFIG_DIR"), "gg", NULL);
    else
	this_configdir = g_build_filename(g_get_home_dir(), ".gg", NULL);
    path = g_build_filename(this_configdir, "config", NULL);
    set_config_file_name((GGaduPlugin *) handler, path);
    if (!config_read(handler))
	g_warning(_("Unable to read configuration file for plugin %s"), GGadu_PLUGIN_NAME);

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
    if (dockapp_menu) {
	signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", dockapp_menu, "main-gui");
	ggadu_menu_free(dockapp_menu);
    }

    /* remove blinker function if exist */
    if (blinker_id >0) {
	g_source_remove(blinker_id);
	blinker_id = 0;
    }
    blink_no = 0;
}
