/* $Id: dockapp_plugin.c,v 1.29 2005/02/17 16:52:43 andyx_x Exp $ */

/* 
 * Dockapp plugin for GNU Gadu 2 
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
#include <config.h>
#endif

#include <glib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <gg2_core.h>
#include "dockapp.xpm"
#include "dockapp_plugin.h"


GGaduPlugin *handler;
static gchar *dockapp_configdir = NULL;

GdkGC *gc;
GtkWidget *da = NULL;
GdkPixmap *launch_pixmap;
GdkPixmap *source_pixmap;
GdkBitmap *source_mask;
GtkWidget *status_dockapp = NULL;
GtkTooltips *tips;
PangoLayout * pText;
char *tip;
 
//kolory
static GdkColor clUnk = { 0, 3000, 30000, 3000 };
static GdkColor clred = { 0, 65535, 3000, 3000 };
static GdkColor clAway = { 0, 65535, 3000, 65535 };
static GdkColor clblack = { 0, 0, 0, 0 };
static GdkColor clOnline = { 0, 3000, 3000, 65535 };
static GdkColor clOffline = { 0, 65535, 3000, 3000 };
static GdkColor clBackground = { 0, 0, 0, 0 };


//Przyciski
GdkPixbuf *icon1_img;
GdkPixbuf *icon2_img;
GdkRectangle icon1 = { 7, 6, 16, 16 };
GdkRectangle icon2 = { 27, 6, 16, 16 };
GdkRectangle btnred = { 42, 5, 13, 15 };

#define NNICK 3
#define NNICK_LEN 20
gchar prev_nick[NNICK][NNICK_LEN];
guint prev_status[NNICK] = { 0, 0, 0 };

guint blinker_id = 0;		/* id of blinker timer */
guint blink_no = 0;		/* show or hide new message icon (even=show, odd=dont show) */

GGadu_PLUGIN_INIT(DOCKLET_PLUGIN_NAME, GGADU_PLUGIN_TYPE_MISC);


//Koñczy pracê plugina, funkcja exportowana - musi byc
void destroy_plugin()
{
	print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
	gtk_widget_destroy(da);
	gtk_widget_destroy(status_dockapp);
//	g_object_unref(da);
//	g_object_unref(launch_pixmap);
//	launch_pixmap = NULL;
	g_object_unref(gc);
//	gc = NULL;
}


//Sprawdza czy x,y znajduje sie wewnatrz prostokata
int btn_clicked(GdkRectangle * btn, int x, int y)
{
	return (x >= btn->x && x <= btn->x + btn->width && y >= btn->y && y <= btn->y + btn->height);
}


//Narysuj wszystko z launcz_pixmap na ekran
void redraw()
{
	gdk_draw_drawable(da->window, gc, launch_pixmap, 0, 0, 0, 0, 64, 64);
}





//Przerysowuje elementy dynamiczne pixmapy
void draw_pixmap()
{

	//tlo ikony
	gdk_draw_pixmap(launch_pixmap, gc , source_pixmap , 0 , 0 , 0 , 0 , 64 ,64);

	//Rysuje ikonki
	if (icon1_img != NULL)
		gdk_draw_pixbuf(launch_pixmap, gc, icon1_img, 0, 0, icon1.x, icon1.y, icon1.width, icon1.height, GDK_RGB_DITHER_NONE, 0, 0);

	// draw 'new message' icon 
	if ((icon2_img != NULL) && ((blink_no % 2) == 1))
		gdk_draw_pixbuf(launch_pixmap, gc, icon2_img, 0, 0, icon2.x, icon2.y, icon2.width, icon2.height, GDK_RGB_DITHER_NONE, 0, 0);

	int i;

	//Wyswietl 3 nicki w kolorach zalenych od statusu	
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
			
		pango_layout_set_text (pText,prev_nick[i],-1);
		gdk_draw_layout(launch_pixmap,gc, 6, 24 + (i * 11), pText);
	}
	
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
		/* stop if no protocol */
		if (ggadu_strcasecmp(plugin_name, "None") == 0)
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

	/* update config */
	if (signal->name == g_quark_from_static_string("update config"))
	{
		GGaduDialog *dialog = signal->data;
		GSList *tmplist = ggadu_dialog_get_entries(dialog); 
		
		if (ggadu_dialog_get_response(dialog) == GGADU_OK)
		{
			while (tmplist)
			{
				GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;

				switch (kv->key)
				{
					case GGADU_DOCKAPP_USERFONT:
					print_debug("changing var setting userfont to %s\n", kv->value);
					ggadu_config_var_set(handler, "userfont", kv->value);
					gtk_widget_modify_font(da,pango_font_description_from_string(kv->value));
					gdk_window_shape_combine_mask((da->window), source_mask, 0, 0);
					break; 		
					
					case GGADU_DOCKAPP_PROTOCOL:
					print_debug("changing var setting protocol to %s\n", ((GSList *) kv->value)->data);
					ggadu_config_var_set(handler, "protocol", ((GSList *) kv->value)->data);
					break; 		
					
					case GGADU_DOCKAPP_COLOR_ONLINE:
					print_debug("changing var setting online color to %s\n", kv->value);
					ggadu_config_var_set(handler, "color_online", kv->value);
					gdk_color_parse (kv->value, &clOnline);
					break; 		
					
					case GGADU_DOCKAPP_COLOR_AWAY:
					print_debug("changing var setting away color to %s\n", kv->value);
					ggadu_config_var_set(handler, "color_away", kv->value);
					gdk_color_parse (kv->value, &clAway);
					break; 		

					case GGADU_DOCKAPP_COLOR_OFFLINE:
					print_debug("changing var setting offline color to %s\n", kv->value);
					ggadu_config_var_set(handler, "color_offline", kv->value);
					gdk_color_parse (kv->value, &clOffline);
					break; 		
					
					case GGADU_DOCKAPP_COLOR_BACK:
					print_debug("changing var setting back color to %s\n", kv->value);
					ggadu_config_var_set(handler, "color_back", kv->value);
					gdk_color_parse (kv->value, &clBackground);
					break; 		

				}
				
				tmplist = tmplist->next;
			}
			
			ggadu_config_save(handler);
			draw_pixmap();
			redraw();
		}
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
	int user_status;

	print_debug("%s : notify on protocol %s\n", GGadu_PLUGIN_NAME, repo_name);

	/* stop if no dockapp_protocol set or notify from other protocol */
	dockapp_protocol = ggadu_config_var_get(handler, "protocol");
	if (!dockapp_protocol || ( ggadu_strcasecmp(dockapp_protocol, "All")  && ggadu_strcasecmp(dockapp_protocol, repo_name  )))
		return;

	/* stop if unknown contact */
	if ((k = ggadu_repo_find_value(repo_name, key)) == NULL)
		return;


	/* find protocol in repo */
	
	index = ggadu_repo_value_first("_protocols_", REPO_VALUE_PROTOCOL, (gpointer *) & key2);
	while (index)
	{
		p = ggadu_repo_find_value("_protocols_", key2);
		utf = to_utf8("ISO-8859-2", p->display_name);
		if (!ggadu_strcasecmp(utf, repo_name))
			break;
		index = ggadu_repo_value_next("_protocols_", REPO_VALUE_PROTOCOL, (gpointer *) & key2, index);
	}

	if (!index)
		return;		/* unknown protocol */

	/* search for status online */
	if (g_slist_find(p->online_status, (gint *) k->status) != NULL)
		user_status = GGADU_DOCKAPP_STATUS_ONLINE;
	/* search for status away */
	else if (g_slist_find(p->away_status, (gint *) k->status) != NULL)
		user_status = GGADU_DOCKAPP_STATUS_AWAY;
	/* search for status offline */
	else if (g_slist_find(p->offline_status, (gint *) k->status) != NULL)
		user_status = GGADU_DOCKAPP_STATUS_OFFLINE;
	/* not found? set unknown status */
	else
		user_status = GGADU_DOCKAPP_STATUS_UNKNOWN;

	/* check if last notify exists */
	
	for (i = NNICK - 1; i >= 0; i--)
	{
		if ( strncmp (prev_nick[i] , (k->nick != NULL) ? k->nick : k->id , NNICK_LEN - 1) == 0 )
		{
			if ( prev_status[i] == user_status )
				return;
			else
				break;
		}
	}
		
	/* shift existent notifies */
	for (i = 0; i < NNICK - 1; i++)
	{
		g_strlcpy(prev_nick[i], prev_nick[i + 1], NNICK_LEN - 1);
		prev_status[i] = prev_status[i + 1];
	}

	/* add new notify */
	g_strlcpy(prev_nick[NNICK - 1], (k->nick != NULL) ? k->nick : k->id, NNICK_LEN - 1);

	prev_status[NNICK - 1] = user_status;

	draw_pixmap();
	redraw();
}

gpointer user_preferences_action(gpointer user_data)
{
	GGaduDialog *dialog = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("Dockapp plugin configuration"), "update config"); 
	GSList *protocol_names = NULL;g_slist_append(protocol_names, "All" );
	gpointer key2 = NULL, index = NULL;	
	gchar * utf;
	GGaduProtocol *p = NULL;


	protocol_names = g_slist_append(protocol_names, "All" );	
	if ( strcmp(ggadu_config_var_get(handler, "protocol"),"All" ) == 0 ) 
		protocol_names = g_slist_prepend(protocol_names, "All" );		
	
	index = ggadu_repo_value_first("_protocols_", REPO_VALUE_PROTOCOL, (gpointer *) & key2);
	while (index)
	{
		p = ggadu_repo_find_value("_protocols_", key2);
		utf = to_utf8("ISO-8859-2", p->display_name);
		index = ggadu_repo_value_next("_protocols_", REPO_VALUE_PROTOCOL, (gpointer *) & key2, index);
		protocol_names = g_slist_append(protocol_names, utf);		
		if ( strcmp(ggadu_config_var_get(handler, "protocol"),utf ) == 0 ) 
			protocol_names = g_slist_prepend(protocol_names, utf );		
		
	}
	protocol_names = g_slist_append(protocol_names,"None");	
	if ( strcmp(ggadu_config_var_get(handler, "protocol"),"None" ) == 0 ) 
		protocol_names = g_slist_prepend(protocol_names, "None" );		

	ggadu_dialog_add_entry(dialog, GGADU_DOCKAPP_PROTOCOL, _("Notify on protocol"), VAR_LIST, protocol_names, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_DOCKAPP_USERFONT, _("User status font"), VAR_FONT_CHOOSER, ggadu_config_var_get(handler, "userfont"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, GGADU_DOCKAPP_COLOR_ONLINE, _("Online status color"), VAR_COLOUR_CHOOSER, ggadu_config_var_get(handler, "color_online"), VAR_FLAG_ADVANCED);
	ggadu_dialog_add_entry(dialog, GGADU_DOCKAPP_COLOR_AWAY, _("Away status color"), VAR_COLOUR_CHOOSER, ggadu_config_var_get(handler, "color_away"), VAR_FLAG_ADVANCED);
	ggadu_dialog_add_entry(dialog, GGADU_DOCKAPP_COLOR_OFFLINE, _("Offline status color"), VAR_COLOUR_CHOOSER, ggadu_config_var_get(handler, "color_offline"), VAR_FLAG_ADVANCED);
	ggadu_dialog_add_entry(dialog, GGADU_DOCKAPP_COLOR_BACK, _("Background color"), VAR_COLOUR_CHOOSER, ggadu_config_var_get(handler, "color_back"), VAR_FLAG_ADVANCED);

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", dialog, "main-gui"); 	

	g_slist_free(protocol_names); 

	return NULL;
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

	gtk_tooltips_set_tip(GTK_TOOLTIPS(tips), status_dockapp, "Kliknij aby ukryæ Program", "ppp");

	gtk_widget_realize(status_dockapp);



	//tworzy obszar rysowania
	da = gtk_drawing_area_new();
	gtk_widget_set_size_request(da, 64, 64);
	gtk_widget_set_events(da, GDK_BUTTON_PRESS_MASK | GDK_EXPOSURE_MASK);
	g_signal_connect(G_OBJECT(da), "expose_event", G_CALLBACK(redraw), NULL);
	g_signal_connect(G_OBJECT(da), "button-press-event", G_CALLBACK(dockapp_clicked), NULL);
	gtk_container_add(GTK_CONTAINER(status_dockapp), da);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(tips), da, "ppp", "ppp");	
	pText = gtk_widget_create_pango_layout(da,NULL);
	gtk_widget_realize(da);

	//ustwawiamy kolory
	gdk_color_parse (ggadu_config_var_get(handler, "color_online"), &clOnline);
	gdk_color_parse (ggadu_config_var_get(handler, "color_away"), &clAway);
	gdk_color_parse (ggadu_config_var_get(handler, "color_offline"), &clOffline);
	gdk_color_parse (ggadu_config_var_get(handler, "color_back"), &clBackground);

	//Opcje graficzne okna
	gc = gdk_gc_new(da->window);
	
	// pixmapa zawierajaca to z czego bedziemy kopiowac	oraz maske dockapp-a
	source_pixmap = gdk_pixmap_create_from_xpm_d((status_dockapp->window), &(source_mask), NULL, dockapp_xpm); 		
	gdk_gc_set_rgb_fg_color(gc, &clBackground);
	gdk_draw_rectangle(source_pixmap, gc, TRUE, 4, 4, 56, 56);	
	gtk_widget_modify_font(da,pango_font_description_from_string("Sans bold 16"));
	pText = gtk_widget_create_pango_layout(da,"X");
	gdk_gc_set_rgb_fg_color(gc, &clred);
	gdk_draw_layout(source_pixmap,gc, btnred.x , btnred.y, pText);
	gtk_widget_modify_font(da,pango_font_description_from_string(ggadu_config_var_get(handler, "userfont")));
	
	//Pixmapa - na niej nalezy wykonywac wszystkie rysunki
	//A dopiero potem przerysowaæ za pomoc± redraw()
	launch_pixmap = gdk_pixmap_new(da->window, 64, 64, -1);



	draw_pixmap();
	redraw();

	//Windowmaker hints - tu sprowadza okno do ikony
	XWMHints wmhints;
	wmhints.initial_state = WithdrawnState;
	wmhints.flags = StateHint | IconWindowHint | IconPositionHint | WindowGroupHint;
	wmhints.icon_x = 0;
	wmhints.icon_y = 0;
	wmhints.icon_window = GDK_WINDOW_XWINDOW(da->window);
	wmhints.window_group = GDK_WINDOW_XWINDOW(status_dockapp->window);

	// ustawienie maski dla dockapp-a
	gdk_window_shape_combine_mask((status_dockapp->window), source_mask, 0, 0);
	gdk_window_shape_combine_mask((da->window), source_mask, 0, 0);

	//Pokaz widgety 
	gtk_widget_show_all(da);
	gtk_widget_show_all(status_dockapp);

	XSetWMHints(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(status_dockapp->window), &wmhints);

	//rejestracja sygnalow
	register_signal(handler, "update config");
	register_signal(handler, "docklet set icon");
	register_signal(handler, "docklet set default icon");
	ggadu_repo_watch_add(NULL, REPO_ACTION_VALUE_CHANGE, REPO_VALUE_CONTACT, notify_callback);

	GGaduMenu *root = ggadu_menu_create();
	GGaduMenu *item_tl = ggadu_menu_add_item(root, "_Dockapp", NULL, NULL);
		
	ggadu_menu_add_submenu(item_tl, ggadu_menu_new_item(_("_Preferences"), user_preferences_action, NULL));

	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", root, "main-gui"); 	

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
	
	if (g_getenv("HOME_ETC"))
		dockapp_configdir = g_build_filename(g_getenv("HOME_ETC"), "gg2", NULL);
	else
		dockapp_configdir = g_build_filename(g_get_home_dir(), ".gg2", NULL); 	
	
	path = g_build_filename(dockapp_configdir, "dockapp", NULL);
	ggadu_config_set_filename((GGaduPlugin *) handler, path);
	g_free(path);
	
	ggadu_config_var_add_with_default(handler, "protocol", VAR_STR, "All");
//	ggadu_config_var_add(handler, "visible", VAR_BOOL);
	ggadu_config_var_add_with_default(handler, "userfont", VAR_STR, "Sans 10");
	ggadu_config_var_add_with_default(handler, "color_online", VAR_STR, "#0B0BFF");
	ggadu_config_var_add_with_default(handler, "color_away", VAR_STR, "#FF0BFF");	
	ggadu_config_var_add_with_default(handler, "color_offline", VAR_STR, "#FF0B0B");
	ggadu_config_var_add_with_default(handler, "color_back", VAR_STR, "#EAEA75");
	
	if (!ggadu_config_read(handler))
		g_warning(_("Unable to read configuration file for plugin %s"), GGadu_PLUGIN_NAME);

	memset(prev_nick,0,sizeof(prev_nick));		
	return handler;
}
