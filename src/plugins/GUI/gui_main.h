/* $Id: gui_main.h,v 1.9 2003/10/27 21:46:19 krzyzak Exp $ */

#ifndef GGadu_GUI_PLUGIN_MAIN_H
#define GGadu_GUI_PLUGIN_MAIN_H 1

#include <gtk/gtk.h>
#include "unified-types.h"

// default size of a window

#define DEFAULT_WIDTH 160
#define DEFAULT_HEIGHT 488

/* default icon's filename */
#define GGADU_DEFAULT_ICON_FILENAME "icon.png"
#define GGADU_MSG_ICON_FILENAME "new-msg.png"

typedef struct  {
    gchar *id;
    GtkWidget *chat;
    GSList *recipients;
} gui_chat_session;


typedef struct {
    gchar 	  *plugin_name;
    GSList 	  *userlist;
    GSList	  *chat_sessions;
    GtkListStore  *users_liststore;
    GtkWidget     *add_info_label;
    GtkWidget	  *statuslist_eventbox;
    gchar 	  *tree_path;
    guint	  blinker;
    GdkPixbuf	  *blinker_image1;
    GdkPixbuf	  *blinker_image2;
    guint	  aaway_timer;
    GGaduProtocol *p;
} gui_protocol;

typedef struct {
    gchar *emoticon;
    gchar *file;
} gui_emoticon;

typedef struct {
	gchar *signal_name;
	void (*handler_func)();
} gui_signal_handler;

enum {
    CHAT_TYPE_CLASSIC,
    CHAT_TYPE_TABBED
};


void grm_free(gpointer signal);

void gautl_free(gpointer signal);

void gn_free(gpointer signal);

#endif
