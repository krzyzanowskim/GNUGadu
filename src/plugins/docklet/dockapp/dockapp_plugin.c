/* $Id: dockapp_plugin.c,v 1.24 2004/10/20 10:50:47 krzyzak Exp $ */

/* 
 * Dockapp plugin for GNU Gadu 2 
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

#include <glib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <gg2_core.h>
#include "dockapp_plugin.h"


GGaduPlugin *handler;

GdkGC *gc;
GtkWidget *da = NULL;
GdkPixmap *launch_pixmap;
GtkWidget *status_dockapp = NULL;
GtkTooltips *tips;
char *tip;

//kolory
static GdkColor clUnk = { 0, 3000, 30000, 3000 };
static GdkColor clred = { 0, 65535, 3000, 3000 };
static GdkColor clAway = { 0, 65535, 3000, 65535 };
static GdkColor clblack = { 0, 0, 0, 0 };
static GdkColor clwhite = { 0, 65535, 65535, 65535 };
static GdkColor clOnline = { 0, 3000, 3000, 65535 };
static GdkColor clOffline = { 0, 65535, 3000, 3000 };


static gchar *this_configdir = NULL;

//Przyciski
GdkPixbuf *icon1_img;
GdkPixbuf *icon2_img;
GdkRectangle icon1 = { 5, 4, 16, 16 };
GdkRectangle icon2 = { 25, 4, 16, 16 };
GdkRectangle btnred = { 42, 5, 13, 15 };

#define NNICK 3
gchar prev_nick[NNICK][10] = { "\0\0\0\0\0\0\0\0\0\0", "\0\0\0\0\0\0\0\0\0\0", "\0\0\0\0\0\0\0\0\0\0" };
guint prev_status[NNICK] = { 0, 0, 0 };

guint blinker_id = 0;		/* id of blinker timer */
guint blink_no = 0;		/* show or hide new message icon (even=show, odd=dont show) */

GGadu_PLUGIN_INIT(DOCKLET_PLUGIN_NAME, GGADU_PLUGIN_TYPE_MISC);


//Ko�czy prac� plugina, funkcja exportowana - musi byc
void destroy_plugin()
{
	print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
	gtk_widget_destroy(da);
	gtk_widget_destroy(status_dockapp);
	g_object_unref(launch_pixmap);
	launch_pixmap = NULL;
	g_object_unref(gc);
	gc = NULL;
}


//Sprawdza czy x,y znajduje sie wewnatrz prostokata
int btn_clicked(GdkRectangle * btn, int x, int y)
{
	return (x >= btn->x && x <= btn->x + btn->width && y >= btn->y && y <= btn->y + btn->height);
}


//Narysuj wszystko z launcz_pixmap na ekran
void redraw()
{
	gdk_draw_pixmap(da->window, gc, launch_pixmap, 0, 0, 0, 0, 64, 64);
}





//Przerysowuje elementy dynamiczne pixmapy
void draw_pixmap()
{

	//tlo ikony
	gdk_gc_set_rgb_fg_color(gc, &clwhite);
	gdk_draw_rectangle(launch_pixmap, gc, TRUE, 3, 3, 57, 57);

	//Rysuje ikonki
	if (icon1_img != NULL)
		gdk_draw_pixbuf(launch_pixmap, gc, icon1_img, 0, 0, icon1.x, icon1.y, icon1.width, icon1.height, GDK_RGB_DITHER_NONE, 0, 0);

	// draw 'new message' icon 
	if ((icon2_img != NULL) && ((blink_no % 2) == 1))
		gdk_draw_pixbuf(launch_pixmap, gc, icon2_img, 0, 0, icon2.x, icon2.y, icon2.width, icon2.height, GDK_RGB_DITHER_NONE, 0, 0);

	int i;
	GdkFont *font;

	//Wyswietl 3 nicki w kolorach zalenych od statusu
	font = gdk_font_load("-misc-fixed-bold-r-normal-*-15-*-*-*-*-*-iso8859-2");
	gdk_gc_set_rgb_fg_color(gc, &clred);
	gdk_draw_text(launch_pixmap, font, gc, 42, 17, "X", 1);

	font = gdk_font_load("-misc-fixed-medium-r-normal-*-10-*-*-*-*-*-iso8859-2");
	for (i = 0; i < NNICK; i++)
	{
		switch (prev_status[i])
		{
		case GGADU_DOCKAPP_STATUS_OFFLINE:
			gdk_gc_set_rgb_fg_color(gc, &clOffline);
			break;
		case GGADU_DOCKAPP_STATUS_AWAY:
			gdk_gc_set_rgb_fg_color(gc, &clAway);
			break;
		case GGADU_DOCKAPP_STATUS_ONLINE:
			gdk_gc_set_rgb_fg_color(gc, &clOnline);
			break;
		default:
			gdk_gc_set_rgb_fg_color(gc, &clUnk);
		}
		gdk_draw_text(launch_pixmap, font, gc, 6, 34 + (i * 10), prev_nick[i], strlen(prev_nick[i]));
	}


	gdk_font_unref(font);
	gdk_gc_set_rgb_fg_color(gc, &clblack);
}


//Obsluga zdarzen klikniecia
void dockapp_clicked(GtkWidget * widget, GdkEventButton * ev, gpointer data)
{
	print_debug("%s : mouse button clicked\n", GGadu_PLUGIN_NAME);
	/* on icon1 or icon2 ? */
	if (btn_clicked(&btnred, ev->x, ev->y))
	{
		destroy_plugin();
	}
	else
	{
		/* yes: stop blinking and hide 'new message' icon, redraw dockapp,
		 * say GUI to show invisible chats or main window */
		if (blinker_id > 0)
		{
			g_source_remove(blinker_id);
			blinker_id = 0;
		}
		blink_no = 0;
		if (icon2_img != NULL)
		{
			g_object_unref(icon2_img);
			icon2_img = NULL;
		}
		draw_pixmap();
		redraw();
		signal_emit_full(GGadu_PLUGIN_NAME, "gui show invisible chats", NULL, "main-gui", NULL);
	}
}

//zaladuj rysunek subdir -podkatalog PACKAGE_DATA_DIR
GdkPixbuf *dockapp_create_pixbuf(const gchar * directory, const gchar * filename)
{
	gchar *path = NULL;
	GdkPixbuf *pixbuf = NULL;


	path = ggadu_get_image_path(directory, filename);

	print_debug("dockapp_create_pixbuf %s", path);
	pixbuf = gdk_pixbuf_new_from_file(path, NULL);
	g_free(path);
	return pixbuf;

}

/* function run from blinker timer */
gboolean msgicon_blink(gpointer data)
{
	print_debug("%s: msgicon_blink\n", GGadu_PLUGIN_NAME);
	/* if blinking decrease number and redraw dockapp */
	if (blink_no > 1)
	{
		blink_no--;
		draw_pixmap();
		redraw();
		return TRUE;	/* timer still running */
	}
	/* else stop timer */
	return FALSE;
}


void my_signal_receive(gpointer name, gpointer signal_ptr)
{
	GGaduSignal *signal = (GGaduSignal *) signal_ptr;
	GSList *sigdata = (GSList *) signal->data;

	//gchar *s1 = g_strdup(g_slist_nth_data(sigdata, 0));           
	//gchar *s2 = g_strdup(g_slist_nth_data(sigdata, 1));                   
	//gchar *s3 = g_strdup(g_slist_nth_data(sigdata, 2));                   

	//print_debug("************************[ DOCKAPP SIGNAL ]********************************");
	print_debug("%s : receive signal %d %s\n", GGadu_PLUGIN_NAME, signal->name, g_quark_to_string(signal->name));
	//print_debug("s1=%s\n",s1);
	//print_debug("s2=%s\n",s2);    
	//print_debug("s3=%s\n",s3);    
	//print_debug("***************************************************************************");           

	if (signal->name == g_quark_from_static_string("docklet set default icon"))
	{
		gchar *directory = g_build_filename(PACKAGE_DATA_DIR, "pixmaps", "icons", g_strdup(g_slist_nth_data(sigdata, 0)), NULL);
		gchar *filename = g_strdup(g_slist_nth_data(sigdata, 1));

		icon1_img = dockapp_create_pixbuf(directory, filename);
		draw_pixmap();
		redraw();
		g_free(filename);
		g_free(directory);
	}


	/* my status changed */
	if (signal->name == g_quark_from_static_string("docklet set icon"))
	{
		gchar *directory = g_build_filename(PACKAGE_DATA_DIR, "pixmaps", NULL);
		gchar *filename = g_strdup(g_slist_nth_data(sigdata, 1));
		gchar *msg = g_strdup(g_slist_nth_data(sigdata, 2));
		icon2_img = dockapp_create_pixbuf(directory, filename);

		// stop previous blinker if exists             
		if (blinker_id > 0)
			g_source_remove(blinker_id);
		// blink 2 times
		blink_no = 5;
		blinker_id = g_timeout_add(500, (GtkFunction) msgicon_blink, NULL);
		gtk_tooltips_set_tip(GTK_TOOLTIPS(tips), status_dockapp, msg, "");
		draw_pixmap();
		redraw();
		g_free(filename);
		g_free(directory);
		return;
	}

	/* my status changed */
	if (signal->name == g_quark_from_static_string("dockapp status changed"))
	{
		gchar *plugin_name = g_strdup(g_slist_nth_data(sigdata, 0));
		/* stop if wrong protocol */
		if (ggadu_strcasecmp(plugin_name, "gadu-gadu"))
		{
			g_free(plugin_name);
			return;
		}
		icon1_img = g_slist_nth_data(sigdata, 1);
		draw_pixmap();
		redraw();
		g_free(plugin_name);
		return;
	}
}

/* do when user notify changed */
void notify_callback(gchar * repo_name, gpointer key, gint actions)
{
	int i;
	GGaduContact *k = NULL;
	GGaduProtocol *p = NULL;
	gchar *dockapp_protocol, *utf;
	gpointer key2 = NULL, index = NULL;

	print_debug("%s : notify on protocol %s\n", GGadu_PLUGIN_NAME, repo_name);

	/* stop if no dockapp_protocol set or notify from other protocol */
	dockapp_protocol = ggadu_config_var_get(handler, "dockapp_protocol");
	if (!dockapp_protocol || ggadu_strcasecmp(dockapp_protocol, repo_name))
		return;

	/* stop if unknown contact */
	if ((k = ggadu_repo_find_value(repo_name, key)) == NULL)
		return;

	/* shift existent notifies */
	for (i = 0; i < NNICK - 1; i++)
	{
		g_strlcpy(prev_nick[i], prev_nick[i + 1], 9);
		prev_status[i] = prev_status[i + 1];
	}

	/* add new notify */
	g_strlcpy(prev_nick[NNICK - 1], (k->nick != NULL) ? k->nick : k->id, 9);

	/* find protocol in repo */
	index = ggadu_repo_value_first("_protocols_", REPO_VALUE_PROTOCOL, (gpointer *) & key2);
	while (index)
	{
		p = ggadu_repo_find_value("_protocols_", key2);
		utf = to_utf8("ISO-8859-2", p->display_name);
		if (!ggadu_strcasecmp(utf, dockapp_protocol))
			break;
		index = ggadu_repo_value_next("_protocols_", REPO_VALUE_PROTOCOL, (gpointer *) & key2, index);
	}

	if (!index)
		return;		/* unknown protocol */

	/* search for status online */
	if (g_slist_find(p->online_status, (gint *) k->status) != NULL)
		prev_status[NNICK - 1] = GGADU_DOCKAPP_STATUS_ONLINE;
	/* search for status away */
	else if (g_slist_find(p->away_status, (gint *) k->status) != NULL)
		prev_status[NNICK - 1] = GGADU_DOCKAPP_STATUS_AWAY;
	/* search for status offline */
	else if (g_slist_find(p->offline_status, (gint *) k->status) != NULL)
		prev_status[NNICK - 1] = GGADU_DOCKAPP_STATUS_OFFLINE;
	/* not found? set unknown status */
	else
		prev_status[NNICK - 1] = GGADU_DOCKAPP_STATUS_UNKNOWN;

	draw_pixmap();
	redraw();
}



//Funkcja startowa pluginu, exportowana,musi byc
void start_plugin()
{
	print_debug("%s : start\n", GGadu_PLUGIN_NAME);

	//Etykietki
	tips = gtk_tooltips_new();


	//Tworzy okno glowne programu
	status_dockapp = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(status_dockapp, 64, 64);
	gtk_window_set_title(GTK_WINDOW(status_dockapp), "GNU Gadu 2");
	gtk_window_set_resizable(GTK_WINDOW(status_dockapp), FALSE);
	gtk_window_set_wmclass(GTK_WINDOW(status_dockapp), "GM_window", "gg2");
	gtk_window_set_type_hint(GTK_WINDOW(status_dockapp), GDK_WINDOW_TYPE_HINT_DOCK);
	gtk_widget_set_app_paintable(status_dockapp, TRUE);

	gtk_tooltips_set_tip(GTK_TOOLTIPS(tips), status_dockapp, "Kliknij aby ukry� Program", "ppp");

	gtk_widget_realize(status_dockapp);



	//tworzy obszar rysowania
	da = gtk_drawing_area_new();
	gtk_widget_set_size_request(da, 64, 64);
	gtk_widget_set_events(da, GDK_BUTTON_PRESS_MASK | GDK_EXPOSURE_MASK);
	g_signal_connect(G_OBJECT(da), "expose_event", G_CALLBACK(redraw), NULL);
	g_signal_connect(G_OBJECT(da), "button-press-event", G_CALLBACK(dockapp_clicked), NULL);
	gtk_container_add(GTK_CONTAINER(status_dockapp), da);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(tips), da, "ppp", "ppp");

	gtk_widget_realize(da);

	//Opcje graficzne okna
	gc = gdk_gc_new(da->window);
	//Pixmapa - na niej nalezy wykonywac wszystkie rysunki
	//A dopiero potem przerysowa� za pomoc� redraw()
	launch_pixmap = gdk_pixmap_new(da->window, 64, 64, -1);

	//Rysuje tlo ikony
	gdk_gc_set_rgb_fg_color(gc, &clblack);
	gdk_draw_rectangle(launch_pixmap, gc, TRUE, 0, 0, -1, -1);


	draw_pixmap();
	redraw();

	//Windowmaker hints - tu sprowadza okno do ikony
	XWMHints wmhints;
	wmhints.initial_state = WithdrawnState;
	wmhints.flags = StateHint;
	XSetWMHints(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(status_dockapp->window), &wmhints);
	gdk_window_set_icon(status_dockapp->window, da->window, NULL, NULL);
	gdk_window_set_group(status_dockapp->window, status_dockapp->window);

	//Pokaz widgety
	gtk_widget_show_all(da);
	gtk_widget_show_all(status_dockapp);

	//rejestracja sygnalow
	register_signal(handler, "update config");
	register_signal(handler, "docklet set icon");
	register_signal(handler, "docklet set default icon");
	ggadu_repo_watch_add(NULL, REPO_ACTION_VALUE_CHANGE, REPO_VALUE_CONTACT, notify_callback);
}


/* Initialize dockapp plugin. Return plugin handler */
GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
	gchar *path = NULL;
	print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);

	gtk_init(NULL, NULL);
	GGadu_PLUGIN_ACTIVATE(conf_ptr);	/* It must be here */
	handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("Docklet-dockapp2"));
	register_signal_receiver((GGaduPlugin *) handler, (signal_func_ptr) my_signal_receive);
	ggadu_config_var_add(handler, "dockapp_protocol", VAR_STR);
	ggadu_config_var_add(handler, "dockapp_visible", VAR_BOOL);

	if (g_getenv("HOME_ETC"))
		this_configdir = g_build_filename(g_getenv("HOME_ETC"), "gg", NULL);
	else
		this_configdir = g_build_filename(g_get_home_dir(), ".gg", NULL);


	path = g_build_filename(this_configdir, "config", NULL);
	ggadu_config_set_filename((GGaduPlugin *) handler, path);
	g_free(path);
	if (!ggadu_config_read(handler))
		g_warning(_("Unable to read configuration file for plugin %s"), GGadu_PLUGIN_NAME);

	return handler;
}
