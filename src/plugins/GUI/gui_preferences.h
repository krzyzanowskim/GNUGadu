/* $Id: gui_preferences.h,v 1.1 2003/03/20 10:37:06 krzyzak Exp $ */
#include <gtk/gtk.h>

enum 
{
    PLUGINS_MGR_NAME,
    PLUGINS_MGR_ENABLE,
    PLUGINS_MGR_COUNT
};

void gui_preferences(GtkWidget *widget, gpointer data);
