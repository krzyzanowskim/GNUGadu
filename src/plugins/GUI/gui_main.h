/* $Id: gui_main.h,v 1.4 2003/05/24 10:03:23 zapal Exp $ */

#ifndef GGadu_GUI_PLUGIN_MAIN_H
#define GGadu_GUI_PLUGIN_MAIN_H 1

#include "unified-types.h"

// default size of a window

#define DEFAULT_WIDTH 160
#define DEFAULT_HEIGHT 488

/* default icon's filename */
#define GGADU_DEFAULT_ICON_FILENAME "icon.png"

typedef struct  {
    gchar *id;
    GtkWidget *chat;
    GSList *recipients;
} gui_chat_session;

typedef struct {
    gchar 	  *plugin_name;
    GSList 	  *userlist;
    GSList	  *chat_sessions;
    GGaduMenu 	  *userlist_menu;
    GtkListStore  *users_liststore;
    GtkWidget     *add_info_label;
    GtkWidget	  *statuslist_eventbox;
    GGaduProtocol *p;
    gchar 	  *tree_path;
    guint	  blinker;
    gchar	  *blinker_image1;
    gchar	  *blinker_image2;
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
