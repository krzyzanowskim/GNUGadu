/* $Id: plugin_xosd.c,v 1.3 2003/03/23 17:58:32 thrulliq Exp $ */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time.h>
#include <signal.h>
#include <sys/stat.h>

#include <xosd.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "menu.h"
#include "support.h"
#include "dialog.h"
#include "plugin_xosd.h"

//static gchar *font = "-adobe-helvetica-bold-r-normal--24-240-75-75-p-138,-*-*-*-R-Normal--*-180-100-100-*-*,-*-*-*-*-*--*-*-*-*-*-*";
int fine = 1;
GGaduPlugin *handler;
xosd *osd;

GGaduMenu *menu_pluginmenu;

GGadu_PLUGIN_INIT("xosd", GGADU_PLUGIN_TYPE_MISC);

void my_signal_receive(gpointer name, gpointer signal_ptr) {
	gchar *w = NULL;
	GGaduSignal *signal = (GGaduSignal *)signal_ptr;
/*	gchar *p1;
	gchar *p2;
*/	
        print_debug("%s : receive signal %s\n",GGadu_PLUGIN_NAME,(gchar *)signal->name);
        
        if (!ggadu_strcasecmp(signal->name,"update config"))
        {
            GGaduDialog *d = signal->data;
            GSList *tmplist = d->optlist;
            while (tmplist) 
            {
                GGaduKeyValue *kv = (GGaduKeyValue *)tmplist->data;
                switch (kv->key)
                {
                    case GGADU_XOSD_CONFIG_COLOUR:
                        print_debug("changing var setting colour to %s\n", kv->value);
                        config_var_set(handler, "colour", kv->value);
                        break;
                    case GGADU_XOSD_CONFIG_NUMLINES:
                        print_debug("changing var setting numlines to %d\n", kv->value);
                        config_var_set(handler, "numlines", kv->value);
                        break;
                    case GGADU_XOSD_CONFIG_TIMEOUT:
                        print_debug("changing var setting timeout to %d\n", kv->value);
                        config_var_set(handler, "timeout", kv->value);
                        break;
                    case GGADU_XOSD_CONFIG_TIMESTAMP:
                        print_debug("changing var setting timestamp to %d\n", kv->value);
                        config_var_set(handler, "timestamp", kv->value);
                        break;
/*                    case GGADU_XOSD_CONFIG_ALIGN:
                        if (kv->value == 0)
                            p1 = "left";
                        else if (kv->value == 1)
                            p1 = "center";
                        else if (kv->value == 2)
                            p1 = "right";
                        else break;
                        print_debug("changing var setting align to %s\n", p1);
                        print_debug(" ALIGN = %s\n", p1);
                        config_var_set("align", p1, handler);
                        break;
                    case GGADU_XOSD_CONFIG_POS:
                        switch ((gint)kv->value) {
                            case 0: p2 = "top"; break;
                            case 1: p2 = "bottom"; break;
                            case 2: p2 = "middle"; break;
                        }
                        print_debug("changing var setting pos to %s\n", p2);
                        config_var_set("pos", p2, handler);
                        break;
*/
                }
                tmplist = tmplist->next;
            }
            config_save(handler);
            set_configuration();
            GGaduDialog_free(d); 
            return;
        }

	if (!ggadu_strcasecmp(signal->name,"xosd show message")) {
	    gchar *msg = signal->data;
	    from_utf8("ISO8859-2",msg,w);

	    if (fine) {
		gpointer ts;
		
	        xosd_scroll (osd, 1);
		if (((ts = config_var_get(handler, "timestamp")) != NULL) && ((gboolean)ts == TRUE)) {
		    gchar *ww = NULL;
		    ww = g_strconcat("[",get_timestamp(0),"] ",w,NULL);
		    g_free(w);
		    w = ww;
		}

		xosd_display(osd,xosd_get_number_lines(osd)-1,XOSD_string,w);
	    }

	    g_free(w);
	    
	    return;
	}
    return;
}

gint set_configuration () {
    gchar *conf = (gchar *)config_var_get(handler, "align");
    gchar *conf1 = (gchar *)config_var_get(handler, "pos");

    /* * * * Default set * * * */
    
    NUMLINES = 5;
    FONT = "-misc-fixed-bold-r-*-*-15-*-*-*-*-*-*-2";
    COLOUR = "yellow";
    TIMEOUT = 2;
    SHADOW_OFFSET = 1;
    SCROLLLINES = 1;
    ALIGN = XOSD_center;
    POS = XOSD_top;

    /* * * * Read configuration of the xosd from config file (f.e. ~/.gg2/xosd)
             The variables are:
	       numlines (int)
	       font ["-adobe-helvetica-bold-r-normal--24-240-75-75-p-138,-*-*-*-R-Normal--*-180-100-100-*-*,-*-*-*-*-*--*-*-*-*-*-*", ...]
               colour ["red", "blue", "green", "yellow", ...]
               timeout (int)
               shadow_offset (int)
               scrolllines (int)
               align ["right", "center", "left"]
               pos ["top", "bottom", "middle"]
    * * * */

    if (!config_var_get(handler, "numlines")) {
        print_debug ("xosd: No numlines config found, setting default\n");
    } else {
        NUMLINES = (gint)config_var_get(handler, "numlines");
    }
    osd = xosd_create(NUMLINES);

    if(!osd) {
        fine = 0;
        return 0;
    };
        fine = 1;
    
                                                            

    /* * * * Reading from the file * * * */
        
    if (!config_var_get(handler, "font")) {
        print_debug ("xosd: No font config found, setting default\n");
    } else {
        FONT = (gchar *)config_var_get(handler, "font");
    }    

    if (!config_var_get(handler, "colour")) {
        print_debug ("xosd: No colour config found, setting default\n");
    } else {
        COLOUR = (gchar *)config_var_get(handler, "colour");
    }                

    if (!config_var_get(handler, "timeout")) {
        print_debug ("xosd: No timeout config found, setting default\n");
    } else {
        TIMEOUT = (gint)config_var_get(handler, "timeout");
    }                

    if (!config_var_get(handler, "shadow_offset")) {
        print_debug ("xosd: No shadow_offset config found, setting default\n");
    } else {
        SHADOW_OFFSET = (gint)config_var_get(handler, "shadow_offset");
    }                
    if (!config_var_get(handler, "scrolllines")) {
        print_debug ("xosd: No scrolllines config found, setting default\n");
    } else {
        SCROLLLINES =  (gint) config_var_get(handler, "scrolllines");
    }
    if (conf) {
	if (!ggadu_strcasecmp(conf, "left"))
	    ALIGN = XOSD_left;
	else if (!ggadu_strcasecmp(conf, "right"))
	    ALIGN = XOSD_right;
	else if (!ggadu_strcasecmp(conf, "center"))
	    ALIGN = XOSD_center;
	else
    	    print_debug ("xosd: No align config found, setting default\n");
    }
    
    if (conf1) {
	if (!ggadu_strcasecmp(conf1, "top"))
	    POS = XOSD_top;
	else if (!ggadu_strcasecmp(conf1, "bottom"))
	    POS = XOSD_bottom;
	else if (!ggadu_strcasecmp(conf1, "middle"))
	    POS = XOSD_middle;
	else
    	    print_debug ("xosd: No pos config found, setting default\n");
    }
    
    print_debug ("FONT=%s COLOUR=%s TIMEOUT=%d SHADOW_OFFSET=%d SCROLLLINES=%d ALIGN=%d POS=%d", FONT, COLOUR, TIMEOUT, SHADOW_OFFSET, SCROLLLINES, ALIGN, POS);
    xosd_set_font(osd, FONT);
    xosd_set_colour(osd, COLOUR);
    xosd_set_timeout(osd, TIMEOUT);
    xosd_set_shadow_offset(osd, SHADOW_OFFSET);
    xosd_scroll (osd, SCROLLLINES);
    xosd_set_align(osd, ALIGN);
    xosd_set_pos(osd, POS);

    return 1;
}

void start_plugin() {
    register_signal(handler,"xosd show message");
    register_signal(handler,"update config");
    print_debug("%s : start_plugin\n",GGadu_PLUGIN_NAME);

    if ( set_configuration() == 0 ) {
        return;
    }
    xosd_display(osd,0,XOSD_string,"GNU Gadu 2");
}

gpointer osd_preferences (gpointer user_data)
{
    GGaduDialog *d = NULL;
    print_debug ("%s: Preferences\n", "X OSD");
    d = ggadu_dialog_new();
    ggadu_dialog_set_title(d,_("X OSD Preferences"));
    ggadu_dialog_callback_signal(d,"update config");
    ggadu_dialog_set_type(d, GGADU_DIALOG_CONFIG);
    
    ggadu_dialog_add_entry(&(d->optlist), GGADU_XOSD_CONFIG_COLOUR, _("Colour"), VAR_STR, (gpointer)COLOUR, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_XOSD_CONFIG_NUMLINES, _("Number of lines"), VAR_INT, (gpointer)NUMLINES, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_XOSD_CONFIG_TIMEOUT, _("Timeout"), VAR_INT, (gpointer)TIMEOUT, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_XOSD_CONFIG_TIMESTAMP, _("Timestamp"), VAR_BOOL, (gpointer)config_var_get(handler,"timestamp"), VAR_FLAG_NONE);
//    ggadu_dialog_add_entry(&(d->optlist), GGADU_XOSD_CONFIG_ALIGN, _("Align \n[left=0 center=1 right=2]"), VAR_INT, ALIGN, VAR_FLAG_NONE);
//    ggadu_dialog_add_entry(&(d->optlist), GGADU_XOSD_CONFIG_POS, _("Position \n[top=0 bottom=1 middle=2]"), VAR_INT, POS, VAR_FLAG_NONE);
    
    signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");
    
    return NULL;
}

GGaduMenu *build_plugin_menu ()
{
    GGaduMenu *root     = ggadu_menu_create();
    GGaduMenu *item_gg  = ggadu_menu_add_item(root,"X OSD",NULL,NULL);
    ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Preferences"),osd_preferences, NULL) );
    
    return root;   
}

GGaduPlugin *initialize_plugin(gpointer conf_ptr) {
    gchar *this_configdir = NULL;

    print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);
    
    GGadu_PLUGIN_ACTIVATE(conf_ptr);

    handler = (GGaduPlugin *)register_plugin(GGadu_PLUGIN_NAME,_("On Screen Display"));
    
    /* Menu stuff */
    print_debug ("%s : Create Menu\n", GGadu_PLUGIN_NAME);
    menu_pluginmenu = build_plugin_menu();
    signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_pluginmenu, "main-gui");
    
    
    print_debug("%s : READ CONFIGURATION\n", GGadu_PLUGIN_NAME);
    
    config_var_add(handler, "font",  VAR_STR);
    config_var_add(handler, "colour",  VAR_STR);
    config_var_add(handler, "timeout",    VAR_INT);
    config_var_add(handler, "shadow_offset" ,   VAR_INT);
    config_var_add(handler, "timestamp" ,   VAR_BOOL);
    config_var_add(handler, "align", VAR_STR);
    config_var_add(handler, "pos", VAR_STR);
    config_var_add(handler, "scrolllines", VAR_INT);
    config_var_add(handler, "numlines", VAR_INT);
    
    if (g_getenv("CONFIG_DIR"))
        this_configdir = g_build_filename(g_get_home_dir(),g_getenv("CONFIG_DIR"),"gg2",NULL);
    else
        this_configdir = g_build_filename(g_get_home_dir(),".gg2",NULL);
    
    set_config_file_name((GGaduPlugin *)handler, g_build_filename(this_configdir,"xosd",NULL));
                     
    if (!config_read(handler))
            g_warning(_("Unable to read configuration file for plugin %s"),"xosd");
	    
    register_signal_receiver((GGaduPlugin *)handler, (signal_func_ptr)my_signal_receive);
    
    return handler;
}

void destroy_plugin() {
    print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
    if (menu_pluginmenu)
    {
      signal_emit (GGadu_PLUGIN_NAME, "gui unregister menu", menu_pluginmenu, "main-gui");
    }
}
