/* $Id: GUI_plugin.c,v 1.26 2003/06/22 17:36:00 krzyzak Exp $ */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "support.h"
#include "menu.h"
#include "gui_main.h"
#include "GUI_plugin.h"
#include "gui_chat.h"
#include "gui_support.h"
#include "gui_dialogs.h"
#include "gui_userview.h"
#include "gui_preferences.h"
#include "gtkanimlabel.h"

extern GGaduConfig *config;
extern GGaduPlugin *gui_handler;

extern GSList *protocols;
extern GSList *emoticons;

extern GtkWidget *window;
extern gboolean tree;

extern GtkItemFactory *item_factory;

GtkWidget *main_menu_bar = NULL;
GtkWidget *status_hbox = NULL;
GtkWidget *view_container = NULL;

/* used with tree user view */
GtkAccelGroup *accel_group = NULL;


void set_selected_users_list (GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, gpointer data)
{
    GSList **users = data;
    GGaduContact *k = NULL;

    gtk_tree_model_get (model, iter, 2, &k, -1);

    *users = g_slist_append (*users, k);

}

gboolean nick_list_clicked (GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
    gui_protocol *gp = NULL;
    gchar *plugin_name = NULL;
    GtkTreeViewColumn *treevc = NULL;
    GtkTreePath *treepath = NULL;
    GSList *selectedusers = NULL;


    /* count how many rows is selected and set list "selectedusers" */
    gtk_tree_selection_selected_foreach (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)), set_selected_users_list,
					 &selectedusers);

    if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
      {
	  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
	  GGaduContact *k = NULL;
	  GtkTreeIter iter;

	  if (!gtk_tree_view_get_path_at_pos
	      (GTK_TREE_VIEW (widget), event->x, event->y, &treepath, &treevc, NULL, NULL))
	      return FALSE;
	  print_debug ("GDK_2BUTTON_PRESS\n");
	  gtk_tree_model_get_iter (model, &iter, treepath);
	  gtk_tree_model_get (model, &iter, 2, &k, -1);

	  if (!k)
	      return FALSE;

	  if (!tree)
	    {
		plugin_name = g_object_get_data (G_OBJECT (user_data), "plugin_name");
		gp = gui_find_protocol (plugin_name, protocols);
	    }
	  else
	    {
		gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 3, &gp, -1);
	    }

	  if (gp != NULL)
	    {
		gui_chat_session *session = gui_session_find (gp, k->id);

		if (!session)
		  {
		      session = g_new0 (gui_chat_session, 1);
		      session->id = g_strdup (k->id);
		      gp->chat_sessions = g_slist_append (gp->chat_sessions, session);
		  }

		if (!session->chat)
		    session->chat = create_chat (session, gp->plugin_name, k->id, TRUE);

		gui_chat_append (session->chat, NULL, TRUE);
	    }

	  print_debug ("gui-main : clicked : %s : %s\n", k->id, plugin_name);

	  gtk_tree_path_free (treepath);
      }

    if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1))
      {
	  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
	  GtkTreeIter iter;
	  GGaduContact *k = NULL;
	  gchar *markup_id = NULL;
	  gchar *markup_desc = NULL;
	  gboolean is_desc = FALSE;
	  gchar *desc_text = NULL;
	  gchar *ip = NULL;
	  GtkTooltips *tooltip = NULL;
	  GtkWidget *add_info_label_desc = NULL;

	  if (!gtk_tree_view_get_path_at_pos
	      (GTK_TREE_VIEW (widget), event->x, event->y, &treepath, &treevc, NULL, NULL))
	      return FALSE;

	  print_debug ("GDK_BUTTON_PRESS\n");

	  gtk_tree_model_get_iter (model, &iter, treepath);

	  if (!tree)
	    {
		plugin_name = g_object_get_data (G_OBJECT (user_data), "plugin_name");
		gp = gui_find_protocol (plugin_name, protocols);
	    }
	  else
	    {
		gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 3, &gp, -1);
	    }


	  gtk_tree_model_get (model, &iter, 2, &k, -1);
	  if (!gp)
	      return FALSE;

	  add_info_label_desc = g_object_get_data (G_OBJECT (gp->add_info_label), "add_info_label_desc");

	  tooltip = gtk_tooltips_new ();

	  if (k)
	    {
		if (k->ip)
		  {
		      gchar **strtab;

		      strtab = g_strsplit (k->ip, ":", 2);
		      switch (atoi (strtab[1]))
			{
//                      case 0: ip = g_strdup_printf("\n[brak p2p]");break;
			case 1:
			    ip = g_strdup_printf ("\n[NAT %s]", strtab[0]);
			    break;
			case 2:
			    ip = g_strdup_printf (_("\n[not in userlist]"));
			    break;
			default:
			    ip = g_strdup_printf ("\n[%s]", strtab[0]);
			}
		      g_strfreev (strtab);
		  }

		if (k->status_descr)
		  {
		      gchar *desc_esc = g_markup_escape_text (k->status_descr, strlen (k->status_descr));
		      desc_text = g_strdup_printf ("%s", desc_esc);
		      is_desc = TRUE;
		      g_free (desc_esc);
		  }

		markup_id = g_strdup_printf ("<span size=\"small\">Id: <b>%s</b> %s</span>", k->id, ip ? ip : "");
		markup_desc = (k->status_descr) ? g_strdup_printf ("<span size=\"small\">%s</span>", desc_text) : NULL;

		gtk_tooltips_set_tip (tooltip, gtk_widget_get_ancestor (add_info_label_desc, GTK_TYPE_EVENT_BOX),
				      k->status_descr, "caption");
	    }
	  else
	    {
		GGaduStatusPrototype *sp;
		gint status;

		status = (gint) signal_emit ("main-gui", "get current status", NULL, gp->plugin_name);
		sp = gui_find_status_prototype (gp->p, status);

		markup_id = g_strdup_printf ("<span size=\"small\"><b>%s</b></span>", gp->p->display_name);
		markup_desc =
		    (sp) ? g_strdup_printf ("<span size=\"small\"><b>%s</b></span>", sp->description) : _("(None)");
		is_desc = TRUE;
		gtk_tooltips_set_tip (tooltip, gtk_widget_get_ancestor (add_info_label_desc, GTK_TYPE_EVENT_BOX), NULL,
				      "caption");
	    }

	  gtk_tooltips_enable (tooltip);

	  gtk_label_set_markup (GTK_LABEL (gp->add_info_label), markup_id);

	  gtk_anim_label_set_text (GTK_ANIM_LABEL (add_info_label_desc), markup_desc);
	  gtk_anim_label_animate (GTK_ANIM_LABEL (add_info_label_desc), TRUE);

	  if (!GTK_WIDGET_VISIBLE (gp->add_info_label))
	    {
		gtk_widget_show (gp->add_info_label);
	    }

	  if (is_desc)
	    {
		gtk_widget_show (add_info_label_desc);
	    }
	  else
	    {
		gtk_anim_label_animate (GTK_ANIM_LABEL (add_info_label_desc), FALSE);
		gtk_widget_hide (add_info_label_desc);
	    }

	  g_free (markup_id);
	  g_free (markup_desc);
	  g_free (desc_text);
	  g_free (ip);
      }

    if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3))
      {
	  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
	  GtkItemFactory *ifactory = NULL;
	  GGaduMenu *umenu = NULL;
	  GtkTreeIter iter;

	  print_debug ("main-gui : wcisnieto prawy klawisz ? %s\n",
		       g_object_get_data (G_OBJECT (user_data), "plugin_name"));

	  print_debug ("GDK_BUTTON_PRESS 3\n");

	  if (!gtk_tree_view_get_path_at_pos
	      (GTK_TREE_VIEW (widget), event->x, event->y, &treepath, &treevc, NULL, NULL))
	      return FALSE;

	  gtk_tree_model_get_iter (model, &iter, treepath);

	  if (!tree)
	    {
		plugin_name = g_object_get_data (G_OBJECT (user_data), "plugin_name");
		gp = gui_find_protocol (plugin_name, protocols);
	    }
	  else
	    {
		GGaduContact *k = NULL;

		gtk_tree_model_get (model, &iter, 2, &k, -1);

		if (k)
		    gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 3, &gp, -1);
	    }

	  umenu = signal_emit ("main-gui", "get user menu", selectedusers, plugin_name);
	  
	  if (!umenu) return FALSE;
	  
	  ifactory = gtk_item_factory_new (GTK_TYPE_MENU, "<name>", NULL);

	  if (selectedusers)
	    {
		gui_produce_menu_for_factory (umenu, ifactory, NULL, selectedusers);
		gtk_item_factory_popup (ifactory, event->x_root, event->y_root, event->button, event->time);
	    }

	  gtk_tree_path_free (treepath);
	  ggadu_menu_free (umenu);

	  return TRUE;
      }
    return FALSE;
}

void gui_show_hide_window ()
{

    if (!GTK_WIDGET_VISIBLE (window))
	gtk_widget_show_all (window);
    else
      {
	  gint top, left;
	  gtk_window_get_position (GTK_WINDOW (window), &left, &top);
	  config_var_set (gui_handler, "top", (gpointer) top);
	  config_var_set (gui_handler, "left", (gpointer) left);
	  gtk_widget_hide (window);
      }
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 						PRIVATE STUFF
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * Wyj¶cie z programu poprzez menu
 */
void gui_quit (GtkWidget * widget, gpointer user_data)
{
    gint width, height;
    gint top, left;

    gtk_window_get_size (GTK_WINDOW (window), &width, &height);

    config_var_set (gui_handler, "height", (gpointer) height);
    config_var_set (gui_handler, "width", (gpointer) width);

    gtk_window_get_position (GTK_WINDOW (window), &left, &top);

    config_var_set (gui_handler, "top", (gpointer) top);
    config_var_set (gui_handler, "left", (gpointer) left);

    config_save (gui_handler);

    signal_emit ("main-gui", "exit", NULL, NULL);
    g_main_loop_quit (config->main_loop);
}

/*
 * Obs³uguje sygna³ "skasownaia" g³ównego okna
 */

gboolean gui_main_window_delete (GtkWidget * window, GdkEvent * event, gpointer user_data)
{
    GGaduPlugin *p = find_plugin_by_name ("docklet");
    print_debug ("delete event\n");
    if (!p)
      {
	  gui_quit (window, NULL);
      }
    else
      {
	  gui_show_hide_window ();
      }
    return TRUE;
}


/*
 * Tworzy okno aplikacji
 */
void gui_main_window_create (gboolean visible)
{
    GtkWidget *main_vbox = NULL;
    GdkPixbuf *image;
    gint width, height;
    gint top, left;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name (window, "ggadu_window");
    gtk_window_set_wmclass (GTK_WINDOW (window), "GM_NAME", "GNUGadu");
    gtk_window_set_title (GTK_WINDOW (window), "GNU Gadu 2");
    gtk_window_set_modal (GTK_WINDOW (window), FALSE);

    width = (gint) config_var_get (gui_handler, "width");
    height = (gint) config_var_get (gui_handler, "height");


    if (width <= 0 || width > 3000)
	width = DEFAULT_WIDTH;

    if (height <= 0 || height > 3000)
	height = DEFAULT_HEIGHT;

    gtk_window_set_default_size (GTK_WINDOW (window), width, height);

    top = (gint) config_var_get (gui_handler, "top");
    left = (gint) config_var_get (gui_handler, "left");

    if (top < 0 || top > 3000)
	top = 0;

    if (left < 0 || left > 3000)
	left = 0;

    /* HINT: some window managers may ignore this with default configuration */
    gtk_window_move (GTK_WINDOW (window), left, top);

    image = create_pixbuf (GGADU_DEFAULT_ICON_FILENAME);
    gtk_window_set_icon (GTK_WINDOW (window), image);
    gdk_pixbuf_unref (image);

    main_vbox = gtk_vbox_new (FALSE, 0);

    gtk_box_pack_start (GTK_BOX (main_vbox), main_menu_bar, FALSE, FALSE, 0);

//    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gui_main_window_destroy), NULL);
    g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (gui_main_window_delete), NULL);

    gtk_container_add (GTK_CONTAINER (window), main_vbox);

    view_container = gtk_vbox_new (FALSE, 2);
    gtk_box_pack_start (GTK_BOX (main_vbox), view_container, TRUE, TRUE, 0);

    gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

    status_hbox = gtk_hbox_new (FALSE, 2);
    gtk_box_pack_start (GTK_BOX (main_vbox), status_hbox, FALSE, TRUE, 2);

    if (visible)
	gtk_widget_show_all (GTK_WIDGET (window));

    if (tree)
	gui_create_tree ();
}

gboolean status_blinker (gpointer data)
{
    gui_protocol *gp = (gui_protocol *) data;
    GdkPixbuf *tmp;
    GdkPixbuf *image = NULL;
    GtkWidget *status_image;

//  print_debug ("status_blinker %p\n", data);

    if (!data)
      {
	  return FALSE;		/* stop blinking */
      }

    /* a little weird algorythm ;) */
    tmp = gp->blinker_image1;
    gp->blinker_image1 = gp->blinker_image2;
    gp->blinker_image2 = tmp;

    image = gp->blinker_image1;
    status_image = gtk_bin_get_child (GTK_BIN (gp->statuslist_eventbox));
    gtk_image_set_from_pixbuf (GTK_IMAGE (status_image), image);

    return TRUE;
}

void change_status (GPtrArray * ptra)
{
    GGaduStatusPrototype *sp = g_ptr_array_index (ptra, 0);
    gchar *plugin_source = g_ptr_array_index (ptra, 1);
    gui_protocol *gp = NULL;
    GGaduStatusPrototype *sp1;
    gint status;
//    GtkWidget *status_image = g_ptr_array_index(ptra, 2);
//    GtkWidget *image = create_image(sp->image);

//    gtk_image_set_from_pixbuf(GTK_IMAGE(status_image), gtk_image_get_pixbuf(GTK_IMAGE(image)));
//    gtk_widget_destroy(image);


    gp = gui_find_protocol (plugin_source, protocols);
    if (gp && !is_in_status (sp->status, gp->p->offline_status) && config_var_get (gui_handler, "blink"))
      {
	  gp->aaway_timer = -1;

	  if (gp->blinker > 0)
	      g_source_remove (gp->blinker);
	  gp->blinker = -1;

	  status = (gint) signal_emit ("main-gui", "get current status", NULL, gp->plugin_name);
	  print_debug ("%d %d\n", status, *(int *) &gp->p->offline_status->data);
	  sp1 = gui_find_status_prototype (gp->p, status != 0 ? status : *(int *) &gp->p->offline_status->data);
	  if (sp1 != NULL && is_in_status (status, gp->p->offline_status))
	    {
		gp->blinker_image1 = create_pixbuf (sp1->image);
		gp->blinker_image2 = create_pixbuf (sp->image);
		print_debug ("gui: blinking %s and %s\n", sp1->image, sp->image);
		gp->blinker =
		    g_timeout_add (config_var_get (gui_handler, "blink_interval") ? (gint)
				   config_var_get (gui_handler, "blink_interval") : 500, status_blinker, gp);
	    }
      }
    else if (is_in_status (sp->status, gp->p->offline_status) && gp->blinker > 0)
      {
	  g_source_remove (gp->blinker);
	  gp->blinker = -1;
      }

    signal_emit ("main-gui", "change status", sp, plugin_source);
    /* ZONK */
    /* te tablice "ptra" kiedys tam mozna zwolic, ale nie mozna tutaj */
}


GtkWidget *create_status_menu (gui_protocol * gp, GtkWidget * status_image)
{
    GSList *statuslist = gp->p->statuslist;
    GtkWidget *menu = gtk_menu_new ();
    GtkWidget *item = NULL;

    while (statuslist)
      {
	  GPtrArray *ptra = NULL;
	  GGaduStatusPrototype *sp = statuslist->data;

	  if (!sp->receive_only)
	    {
		item = gtk_image_menu_item_new_with_label (sp->description);
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), create_image (sp->image));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

		ptra = g_ptr_array_new ();

		g_ptr_array_add (ptra, sp);
		g_ptr_array_add (ptra, gp->plugin_name);
		g_ptr_array_add (ptra, status_image);

		g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (change_status), ptra);

		gtk_widget_show_all (item);
	    }

	  statuslist = statuslist->next;
      }

    return menu;
}

/*
 * Buduje standardowe menu
 */
void gui_build_default_menu ()
{
    GtkItemFactoryEntry menu_items[] = {
	{"/_GnuGadu", NULL, NULL, 0, "<Branch>"},
	{_("/GnuGadu/_Preferences"), NULL, gui_preferences, 0, "<StockItem>", GTK_STOCK_PREFERENCES},
	{_("/GnuGadu/"), "", NULL, 0, 0},
	{_("/GnuGadu/_About"), "<CTRL>a", gui_about, 0, "<StockItem>", GTK_STOCK_DIALOG_INFO},
	{_("/GnuGadu/_Quit"), "<CTRL>q", gui_quit, 0, "<StockItem>", GTK_STOCK_QUIT},
	{"/_Menu", NULL, NULL, 0, "<Branch>"},
    };
    gint Nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

    accel_group = gtk_accel_group_new ();

    item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);

    gtk_item_factory_create_items (item_factory, Nmenu_items, menu_items, NULL);

    main_menu_bar = gtk_item_factory_get_widget (item_factory, "<main>");
}

/*
 * SIGNAL CALLBACK FUNCTIONS
 */
void gui_msg_receive (GGaduSignal * signal)
{
    GGaduMsg *msg = signal->data;
    gui_protocol *gp = NULL;

    if ((msg == NULL) || (msg->id == NULL))
      {
	  print_debug ("main-gui : gui_msg_receive : ((msg == NULL) || (msg->id == NULL) - return !!!!\n");
	  return;
      }

    gp = gui_find_protocol (signal->source_plugin_name, protocols);

    print_debug ("%s : %s -> %s | %s \n", "gui-main", msg->id, msg->message, signal->source_plugin_name);

    if (gp != NULL)
      {
	  gui_chat_session *session = NULL;
	  gboolean showwindow = config_var_get (gui_handler, "chat_window_auto_show") ? TRUE : FALSE;

	  if (msg->class == GGADU_CLASS_CONFERENCE)
	      session = gui_session_find_confer (gp, msg->recipients);
	  else
	      session = gui_session_find (gp, msg->id);

	  if (!session)
	    {
		session = g_new0 (gui_chat_session, 1);
		session->id = g_strdup (msg->id);
		gp->chat_sessions = g_slist_append (gp->chat_sessions, session);
	    }

	  if (msg->message == NULL)
	      showwindow = TRUE;

	  if (!session->chat)
	    {
		GSList *sigdata = NULL;

		/* shasta: new "docklet set [default] icon" approach */
		sigdata = g_slist_append (sigdata, (gchar *) config_var_get (gui_handler, "icons"));
		sigdata = g_slist_append (sigdata, (gpointer) "new-msg.png");
		sigdata = g_slist_append (sigdata, _("You Have Message"));

		if (showwindow == FALSE)
		  {
		      if (signal_emit_full ("main-gui", "docklet set icon", sigdata, NULL, (gpointer) g_slist_free) ==
			  NULL)
			{
			    showwindow = TRUE;
			}
		      else
			{
			    showwindow = FALSE;
			}
		  }


		session->recipients = msg->recipients;
		session->chat = create_chat (session, gp->plugin_name, msg->id, showwindow);

		if ((gint) config_var_get (gui_handler, "use_xosd_for_new_msgs") == TRUE && find_plugin_by_name ("xosd")
		    && msg->message)
		  {
		      GGaduContact *k = gui_find_user (msg->id, gp);
		      signal_emit ("main-gui", "xosd show message",
				   g_strdup_printf (_("New message from %s"), (k ? k->nick : msg->id)), "xosd");
		  }

	    }

	  gui_chat_append (session->chat, msg, FALSE);
      }
}

void gui_produce_menu_for_factory (GGaduMenu * menu, GtkItemFactory * item_factory, gchar * root, gpointer user_data)
{
    GGaduMenu *node = menu;
    GGaduMenu *child = NULL;
    gchar *prev_root = NULL;

    if (G_NODE_IS_ROOT (node))
	node = g_node_first_child (node);
    else
	node = g_node_first_sibling (node);

    while (node)
      {
	  GtkItemFactoryEntry *e = g_new0 (GtkItemFactoryEntry, 1);
	  GGaduMenuItem *it = node->data;

	  if (g_node_first_child (node) != NULL)
	    {
		e->item_type = g_strdup ("<Branch>");
		e->callback = NULL;
	    }
	  else
	    {
		e->item_type = g_strdup ("<Item>");
		e->callback = it->callback;	/* to sa rozne callbacki ale function_ptr dobrze sie sprawuje jako it->callback */
	    }

	  prev_root = root;

	  if (root)
	      e->path = g_strdup_printf ("%s/%s", root, it->label);
	  else
	      e->path = g_strdup_printf ("/%s", it->label);

	  print_debug ("%s  %s\n", e->item_type, e->path);
	  gtk_item_factory_create_item (item_factory, e, user_data, 1);

	  if ((child = g_node_first_child (node)) != NULL)
	    {
		root = e->path;
		gui_produce_menu_for_factory (child, item_factory, root, user_data);
		root = prev_root;
	    }

	  node = node->next;

	  g_free (e);
      }

}

GSList *gui_read_emoticons (gchar * path)
{
    GIOChannel *ch = NULL;
    GSList *emotlist = NULL;
    GString *line = g_string_new ("");
    gchar *stmp = NULL;
    gchar **emot = NULL;

    print_debug ("%s\n", path);
    ch = g_io_channel_new_file (path, "r", NULL);

    if (!ch)
	return NULL;

    g_io_channel_set_encoding (ch, NULL, NULL);

    while (g_io_channel_read_line_string (ch, line, NULL, NULL) == G_IO_STATUS_NORMAL)
      {
	  stmp = line->str;
	  emot = array_make (stmp, "\t", 2, 1, 1);

	  if (emot)
	    {
		if (emot[1])
		  {
		      gui_emoticon *gemo = g_new0 (gui_emoticon, 1);
		      gemo->emoticon = emot[0];
		      gemo->file = g_strstrip (emot[1]);
		      emotlist = g_slist_append (emotlist, gemo);
		  }
	    }

      }

    g_string_free (line, TRUE);
    g_io_channel_shutdown (ch, TRUE, NULL);
    return emotlist;
}

void gui_config_emoticons ()
{

    if (config_var_get (gui_handler, "emot"))
      {
	  gchar *path;

	  path = g_build_filename (PACKAGE_DATA_DIR, "pixmaps", G_DIR_SEPARATOR_S, "emoticons", "emoticons.def", NULL);
	  emoticons = gui_read_emoticons (path);
	  g_free (path);

	  // read from .gg2/emoticons.def, delete after
	  if (!emoticons)
	    {
		path = g_build_filename (config->configdir, "emoticons.def", NULL);
		emoticons = gui_read_emoticons (path);
		g_free (path);
	    }
      }
    else
      {
	  GSList *tmplist = emoticons;

	  while (tmplist)
	    {
		gui_emoticon *gemo = tmplist->data;
		g_free (gemo->emoticon);
		g_free (gemo->file);
		g_free (gemo);
		tmplist = tmplist->next;
	    }

	  g_slist_free (emoticons);
	  emoticons = NULL;
      }
}

void gui_load_theme ()
{
    gchar *themepath;
    gchar *themefilename;

    themefilename = g_strconcat (config_var_get (gui_handler, "theme"), ".theme", NULL);
    themepath = g_build_filename (PACKAGE_DATA_DIR, "themes", themefilename, NULL);

    print_debug ("%s : Loading theme from %s\n", "main-gui", themepath);
    gtk_rc_parse (themepath);
    gtk_widget_reset_rc_styles (GTK_WIDGET (window));
    gtk_rc_reparse_all ();
    g_free (themepath);
    g_free (themefilename);
}

void gui_reload_images ()
{
    GSList *sigdata = NULL;

//    gui_user_view_refresh();

    sigdata = g_slist_append (sigdata, (gchar *) config_var_get (gui_handler, "icons"));
    sigdata = g_slist_append (sigdata, GGADU_DEFAULT_ICON_FILENAME);
    sigdata = g_slist_append (sigdata, "GNU Gadu 2");

    signal_emit_full ("main-gui", "docklet set default icon", sigdata, NULL, (gpointer) g_slist_free);
}
