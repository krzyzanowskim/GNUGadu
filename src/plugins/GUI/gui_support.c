/* $Id: gui_support.c,v 1.4 2003/09/16 22:56:08 shaster Exp $ */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "support.h"
#include "signals.h"
#include "plugins.h"
#include "gui_main.h"
#include "gui_chat.h"

extern GGaduPlugin *gui_handler;

void gui_remove_all_chat_sessions(gpointer protocols_list) 
{
    GSList *protocols_local = (GSList *)protocols_list;
    
    while (protocols_local) {
	gui_protocol *protocol = (gui_protocol *)protocols_local->data;
	GSList *sessions = (GSList *)protocol->chat_sessions;
	
	print_debug("zwalniam sesje dla %s\n",protocol->plugin_name);
	
	while (sessions) {
	    gui_chat_session *s = (gui_chat_session *)sessions->data;
	    
	    g_slist_free(s->recipients);
	    g_free(s->id);
	    g_free(s);
	    sessions->data = NULL;
	    
	    sessions = sessions->next;
	}
	
	g_slist_free(protocol->chat_sessions);
	protocol->chat_sessions = NULL;
	
	protocols_local = protocols_local->next;
    }
    

}


gui_chat_session *gui_session_find(gui_protocol *gp, gchar *id)
{
    GSList *tmpsesje = NULL;
    gui_chat_session *session = NULL;
	
    if ((id == NULL) || (gp == NULL) || (gp->chat_sessions == NULL))
	return NULL;

    tmpsesje = gp->chat_sessions;

    while (tmpsesje) 
    {
    	session = ((gui_chat_session *)tmpsesje->data);

	if ((g_slist_length(session->recipients) < 2) && (!ggadu_strcasecmp(session->id, id)))
		return session;

	tmpsesje = tmpsesje->next;
    }
	
    return NULL;
}

gui_chat_session *gui_session_find_confer(gui_protocol *gp, GSList *recipients)
{
    gui_chat_session *session = NULL;
    GSList *tmpsesje    = NULL;
    GSList *sess_tmprec = NULL, *tmprec = NULL; /* ask for tmprec to compare with sess_tmprec */
	
    if ((recipients == NULL) || (gp == NULL) || (gp->chat_sessions == NULL))
	return NULL;

    tmpsesje = gp->chat_sessions;

    while (tmpsesje) 
    {
	gint hit = 0;
    
    	session = (gui_chat_session *)tmpsesje->data;
	if (tmpsesje == NULL) {
	    tmpsesje = tmpsesje->next;
	    continue;
	}
	
	/* musze sprawdzic czy session->recipients sa takie same jak recipients */
	
	tmprec     = recipients;	    
	while (tmprec)	/* compare */
	{
	    sess_tmprec = session->recipients; /* session recipients list */
	    while (sess_tmprec)	/* with that */
	    {
		if (!ggadu_strcasecmp((gchar *)sess_tmprec->data, (gchar *)tmprec->data)) hit++;
		/* print_debug("COMPARE %s %s\n",(gchar *)sess_tmprec->data, (gchar *)tmprec->data); */
		sess_tmprec = sess_tmprec->next;
	    }
	    tmprec = tmprec->next;
	}

	print_debug("HIT = %d, recipients_length = %d\n",hit,g_slist_length(recipients));
	if (hit == g_slist_length(recipients))
	    return session;
	
	tmpsesje = tmpsesje->next;
    }
	
    return NULL;
}

gui_protocol *gui_find_protocol(gchar *plugin_name, GSList *protocolsl)
{
    GSList *tmp = (GSList *)protocolsl;
    
    if (!tmp || !plugin_name) return NULL;
    
    while (tmp) 
    {
	gui_protocol *gp = (gui_protocol *)tmp->data;

	if (gp && !ggadu_strcasecmp(gp->plugin_name, plugin_name))
	    return gp;
	    
	tmp = tmp->next;
    }

    return NULL;
}

GGaduStatusPrototype *gui_find_status_prototype(GGaduProtocol *gp, guint status)
{
    GSList *tmp = NULL;
    
    if (gp == NULL) return NULL;
    
    tmp = gp->statuslist;
    
    while (tmp) 
    {
	GGaduStatusPrototype *sp = tmp->data;

	if ((sp) && (sp->status == status))
	    return sp;
	    
	tmp = tmp->next;
    }

    return NULL;
}

GGaduContact *gui_find_user(gchar *id, gui_protocol *gp)
{    
    GSList *ulist = NULL;
    
    if (!gp || !id) return NULL;
    
    ulist = gp->userlist;

    while (ulist) 
    {
        GGaduContact *k = ulist->data;
	
	if ((k != NULL) && (!ggadu_strcasecmp(id, k->id)))
	    return k;
	    
        ulist = ulist->next;
    }
    
    return NULL;
}

/* This is an internally used function to create pixmaps. */
GtkWidget *create_image(const gchar * filename) {
	GtkWidget 	*image		= NULL;
	gchar 		*found_filename = NULL;
	GSList		*dir		= NULL;
	gchar 		*iconsdir	= NULL;
		
	/* We first try any pixmaps directories set by the application. */
	dir = g_slist_prepend(dir,PACKAGE_DATA_DIR "/pixmaps");
	dir = g_slist_prepend(dir,PACKAGE_DATA_DIR "/pixmaps/emoticons");
#ifdef GGADU_DEBUG
	dir = g_slist_prepend(dir,PACKAGE_SOURCE_DIR "/pixmaps");
	dir = g_slist_prepend(dir,PACKAGE_SOURCE_DIR "/pixmaps/emoticons");
#endif
	if (config_var_get(gui_handler, "icons")) {
	    iconsdir = g_build_filename(PACKAGE_DATA_DIR, "pixmaps", "icons", config_var_get(gui_handler, "icons"), NULL);
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

GdkPixbuf *create_pixbuf(const gchar * filename) {
	gchar 		*found_filename = NULL;
	GdkPixbuf 	*pixbuf		= NULL;
	GSList		*dir		= NULL;
	gchar 		*iconsdir	= NULL;

	if (!filename || !filename[0])
		return NULL;

	/* We first try any pixmaps directories set by the application. */
	dir = g_slist_prepend(dir,PACKAGE_DATA_DIR "/pixmaps");
	dir = g_slist_prepend(dir,PACKAGE_DATA_DIR "/pixmaps/emoticons");
#ifdef GGADU_DEBUG
	dir = g_slist_prepend(dir,PACKAGE_SOURCE_DIR "/pixmaps");
	dir = g_slist_prepend(dir,PACKAGE_SOURCE_DIR "/pixmaps/emoticons");
#endif
	if (config_var_get(gui_handler, "icons")) {
	    iconsdir = g_build_filename(PACKAGE_DATA_DIR, "pixmaps", "icons", config_var_get(gui_handler, "icons"), NULL);
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

GtkWidget *lookup_widget(GtkWidget * widget, const gchar * widget_name)
{
	GtkWidget *parent, *found_widget;

	for (;;) 
	{
		if (GTK_IS_MENU(widget))
		    parent = gtk_menu_get_attach_widget(GTK_MENU(widget));
		else
		    parent = widget->parent;

		if (parent == NULL)
			break;

		widget = parent;
	}

	found_widget = (GtkWidget *) g_object_get_data(G_OBJECT(widget), widget_name);
	
	if (!found_widget)
		g_warning("Widget not found: %s", widget_name);
		
	return found_widget;
}
