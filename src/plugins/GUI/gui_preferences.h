/* $Id: gui_preferences.h,v 1.2 2004/01/17 00:44:59 shaster Exp $ */
#include <gtk/gtk.h>

enum
{
    PLUGINS_MGR_NAME,
    PLUGINS_MGR_ENABLE,
    PLUGINS_MGR_COUNT
};

void gui_preferences(GtkWidget * widget, gpointer data);
