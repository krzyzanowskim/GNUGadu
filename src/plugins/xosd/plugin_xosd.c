/* $Id: plugin_xosd.c,v 1.9 2003/05/12 09:42:19 thrulliq Exp $ */
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

#ifdef PERL_EMBED
#include <EXTERN.h>
#include <perl.h>
#endif

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

gint get_align(void) {
    gchar *conf_align = (gchar *) config_var_get(handler, "align");
    gint result = XOSD_center;	/* defaults to XOSD_center */

    if (conf_align) {
	if (!ggadu_strcasecmp(conf_align, "left"))
	    result = XOSD_left;
	else if (!ggadu_strcasecmp(conf_align, "right"))
	    result = XOSD_right;
	else if (!ggadu_strcasecmp(conf_align, "center"))
	    result = XOSD_center;
	else
	    print_debug ("xosd: No align variable found, setting default\n");
    } else
	print_debug ("xosd: No align variable found, setting default\n");

    return result;
}

gint get_pos(void) {
    gchar *conf_pos = (gchar *) config_var_get(handler, "pos");
    gint result = XOSD_top;	/* defaults to XOSD_top */

    if (conf_pos) {
	if (!ggadu_strcasecmp(conf_pos, "top"))
	    result = XOSD_top;
	else if (!ggadu_strcasecmp(conf_pos, "bottom"))
	    result = XOSD_bottom;
	else if (!ggadu_strcasecmp(conf_pos, "middle"))
	    result = XOSD_middle;
	else
	    print_debug ("xosd: No pos variable found, setting default\n");
    } else
	print_debug ("xosd: No pos variable found, setting default\n");

    return result;
}

void my_signal_receive(gpointer name, gpointer signal_ptr) {
	gchar *w = NULL;
	GGaduSignal *signal = (GGaduSignal *)signal_ptr;

        print_debug("%s : receive signal %s\n",GGadu_PLUGIN_NAME,(gchar *)signal->name);
        
        if (!ggadu_strcasecmp(signal->name,"update config"))
        {
            GGaduDialog *d = signal->data;
            GSList *tmplist = d->optlist;
	    
	    if (d->response == GGADU_OK) {
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
                	case GGADU_XOSD_CONFIG_ALIGN:
                    	    print_debug("changing var setting align to %s\n", kv->value);
                    	    config_var_set(handler, "align", kv->value);
                    	    break;
                	case GGADU_XOSD_CONFIG_POS:
                    	    print_debug("changing var setting pos to %s\n", kv->value);
                    	    config_var_set(handler, "pos", kv->value);
                    	    break;
            	    }
            	    tmplist = tmplist->next;
        	}
        	config_save(handler);
        	set_configuration();
	    }
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

gint set_configuration (void) {
    /* * * * Default set * * * */
    
    NUMLINES = 5;
    FONT = "-misc-fixed-bold-r-*-*-15-*-*-*-*-*-*-2";
    COLOUR = "yellow";
    TIMEOUT = 2;
    SHADOW_OFFSET = 1;
    SCROLLLINES = 1;

/* no need for these two, get_{align|pos} does it for us. */
/*  ALIGN = XOSD_center;
    POS = XOSD_top; */

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

    ALIGN = get_align();
    POS = get_pos();

    print_debug ("FONT=%s COLOUR=%s TIMEOUT=%d SHADOW_OFFSET=%d SCROLLLINES=%d ALIGN=%d POS=%d\n", FONT, COLOUR, TIMEOUT, SHADOW_OFFSET, SCROLLLINES, ALIGN, POS);
    xosd_set_font(osd, FONT);
    xosd_set_colour(osd, COLOUR);
    xosd_set_timeout(osd, TIMEOUT);
    xosd_set_shadow_offset(osd, SHADOW_OFFSET);
    xosd_scroll (osd, SCROLLLINES);
    xosd_set_align(osd, ALIGN);
    xosd_set_pos(osd, POS);

    return 1;
}

gpointer osd_preferences (gpointer user_data)
{
    GGaduDialog *d = NULL;
    GSList *align_list = NULL;
    GSList *pos_list = NULL;
    gint align_cur = get_align();
    gint pos_cur = get_pos();

    print_debug ("%s: Preferences\n", "X OSD");

    d = ggadu_dialog_new();
    ggadu_dialog_set_title(d,_("X OSD Preferences"));
    ggadu_dialog_callback_signal(d,"update config");
    ggadu_dialog_set_type(d, GGADU_DIALOG_CONFIG);

    /* current setting first, then all the rest */
    if (align_cur == XOSD_left)
	align_list = g_slist_append(align_list, "left");
    else if (align_cur == XOSD_center)
	align_list = g_slist_append(align_list, "center");
    else if (align_cur == XOSD_right)
	align_list = g_slist_append(align_list, "right");

    align_list = g_slist_append(align_list, "left");
    align_list = g_slist_append(align_list, "center");
    align_list = g_slist_append(align_list, "right");

    /* current setting first, then all the rest */
    if (pos_cur == XOSD_top)
	pos_list = g_slist_append(pos_list, "top");
    else if (pos_cur == XOSD_middle)
	pos_list = g_slist_append(pos_list, "middle");
    else if (pos_cur == XOSD_bottom)
	pos_list = g_slist_append(pos_list, "bottom");
    pos_list = g_slist_append(pos_list, "top");
    pos_list = g_slist_append(pos_list, "middle");
    pos_list = g_slist_append(pos_list, "bottom");

    ggadu_dialog_add_entry(&(d->optlist), GGADU_XOSD_CONFIG_COLOUR, _("Colour"), VAR_STR, (gpointer)COLOUR, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_XOSD_CONFIG_NUMLINES, _("Number of lines"), VAR_INT, (gpointer)NUMLINES, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_XOSD_CONFIG_TIMEOUT, _("Timeout"), VAR_INT, (gpointer)TIMEOUT, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_XOSD_CONFIG_TIMESTAMP, _("Timestamp"), VAR_BOOL, (gpointer)config_var_get(handler,"timestamp"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_XOSD_CONFIG_ALIGN, _("Alignment"), VAR_LIST, align_list, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_XOSD_CONFIG_POS, _("Position"), VAR_LIST, pos_list, VAR_FLAG_NONE);
    
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

void start_plugin() {
    print_debug("%s : start_plugin\n",GGadu_PLUGIN_NAME);

    /* Menu stuff */
    print_debug ("%s : Create Menu\n", GGadu_PLUGIN_NAME);
    menu_pluginmenu = build_plugin_menu();
    signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_pluginmenu, "main-gui");

    if (!set_configuration()) {
        return;
    }
    xosd_display(osd,0,XOSD_string,"GNU Gadu 2");
}

#ifdef PERL_EMBED
void perl_xosd_show_message (GGaduSignal *signal, gchar *perl_func, void *pperl)
{
  int count, junk;
  SV *sv_name;
  SV *sv_src;
  SV *sv_dst;
  SV *sv_data;
  PerlInterpreter *my_perl = (PerlInterpreter *) pperl;
    
  dSP;
 
  ENTER;
  SAVETMPS;

  sv_name = sv_2mortal (newSVpv (signal->name, 0));
  sv_src  = sv_2mortal (newSVpv (signal->source_plugin_name, 0));
  if (signal->destination_plugin_name)
    sv_dst  = sv_2mortal (newSVpv (signal->destination_plugin_name, 0));
  else
    sv_dst  = sv_2mortal (newSVpv ("", 0));
  sv_data = sv_2mortal (newSVpv (signal->data, 0));

  PUSHMARK (SP);
  XPUSHs (sv_name);
  XPUSHs (sv_src);
  XPUSHs (sv_dst);
  XPUSHs (sv_data);
  PUTBACK;

  count = call_pv (perl_func, G_DISCARD);

  if (count == 0)
  {
    gchar *dst;
    signal->name = g_strdup (SvPV (sv_name, junk));
    signal->source_plugin_name = g_strdup (SvPV (sv_src, junk));
    dst = SvPV (sv_dst, junk);
    if (dst[0] != '\0')
      signal->destination_plugin_name = g_strdup (dst);
    signal->data = g_strdup (SvPV (sv_data, junk));
  }

  FREETMPS;
  LEAVE;
}
#endif

GGaduPlugin *initialize_plugin(gpointer conf_ptr) {
    gchar *this_configdir = NULL;

    print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);
    
    GGadu_PLUGIN_ACTIVATE(conf_ptr);

    handler = (GGaduPlugin *)register_plugin(GGadu_PLUGIN_NAME,_("On Screen Display"));
    
    register_signal(handler,"xosd show message");
#ifdef PERL_EMBED
    register_signal_perl ("xosd show message", perl_xosd_show_message);
#endif
    register_signal(handler,"update config");
    
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
    
    g_free(this_configdir);
             
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
      ggadu_menu_free (menu_pluginmenu);
    }
}
