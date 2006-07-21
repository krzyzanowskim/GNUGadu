/* $Id: gui_support.c,v 1.20 2006/07/21 22:34:58 krzyzak Exp $ */

/* 
 * GUI (gtk+) plugin for GNU Gadu 2 
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
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "ggadu_support.h"
#include "signals.h"
#include "ggadu_conf.h"
#include "plugins.h"
#include "gui_main.h"
#include "gui_chat.h"
#include <string.h>

extern GGaduPlugin *gui_handler;

gint gui_count_visible_tabs(GtkNotebook * notebook)
{
	gint all_count, i;
	gint count = 0;

	if (!G_IS_OBJECT(notebook) || !GTK_IS_WIDGET(notebook))
		return count;

	all_count = gtk_notebook_get_n_pages(notebook);
	for (i = 0; i < all_count; i++)
	{
		if (GTK_WIDGET_VISIBLE(gtk_notebook_get_nth_page(notebook, i)))
			count++;
	}
	return count;
}


void gui_remove_all_chat_sessions(gpointer protocols_list)
{
	GSList *protocols_local = (GSList *) protocols_list;

	while (protocols_local)
	{
		gui_protocol *protocol = (gui_protocol *) protocols_local->data;
		GSList *sessions = (GSList *) protocol->chat_sessions;

		print_debug("remove chat session for %s", protocol->plugin_name);

		while (sessions)
		{
			gui_chat_session *s = (gui_chat_session *) sessions->data;

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

gboolean gui_check_for_sessions(GSList * protocolsl)
{
	GSList *tmp = (GSList *) protocolsl;

	while (tmp)
	{
		gui_protocol *gp = (gui_protocol *) tmp->data;

		if (gp && gp->chat_sessions && (g_slist_length(gp->chat_sessions) > 0))
			return TRUE;

		tmp = tmp->next;
	}

	return FALSE;
}

gui_chat_session *gui_session_find(gui_protocol * gp, gchar * id)
{
	GSList *tmpsesje = NULL;
	gui_chat_session *session = NULL;

	if ((id == NULL) || (gp == NULL) || (gp->chat_sessions == NULL))
		return NULL;

	tmpsesje = gp->chat_sessions;

	while (tmpsesje)
	{
		session = ((gui_chat_session *) tmpsesje->data);

		if ((g_slist_length(session->recipients) < 2) && (!ggadu_strcasecmp(session->id, id)))
			return session;

		tmpsesje = tmpsesje->next;
	}

	return NULL;
}

gui_chat_session *gui_session_find_confer(gui_protocol * gp, GSList * recipients)
{
	gui_chat_session *session = NULL;
	GSList *tmpsesje = NULL;
	GSList *sess_tmprec = NULL, *tmprec = NULL;	/* ask for tmprec to compare with sess_tmprec */

	if ((recipients == NULL) || (gp == NULL) || (gp->chat_sessions == NULL))
		return NULL;

	tmpsesje = gp->chat_sessions;

	while (tmpsesje)
	{
		gint hit = 0;

		session = (gui_chat_session *) tmpsesje->data;
		if (tmpsesje == NULL)
		{
			tmpsesje = tmpsesje->next;
			continue;
		}

		/* musze sprawdzic czy session->recipients sa takie same jak recipients */

		tmprec = recipients;
		while (tmprec)	/* compare */
		{
			sess_tmprec = session->recipients;	/* session recipients list */
			while (sess_tmprec)	/* with that */
			{
				if (!ggadu_strcasecmp((gchar *) sess_tmprec->data, (gchar *) tmprec->data))
					hit++;
				/* print_debug("COMPARE %s %s\n",(gchar *)sess_tmprec->data, (gchar *)tmprec->data); */
				sess_tmprec = sess_tmprec->next;
			}
			tmprec = tmprec->next;
		}

		print_debug("HIT = %d, recipients_length = %d\n", hit, g_slist_length(recipients));
		if (hit == g_slist_length(recipients))
			return session;

		tmpsesje = tmpsesje->next;
	}

	return NULL;
}

gui_protocol *gui_find_protocol(gchar * plugin_name, GSList * protocolsl)
{
	GSList *tmp = (GSList *) protocolsl;

	if (!tmp || !plugin_name)
		return NULL;

	while (tmp)
	{
		gui_protocol *gp = (gui_protocol *) tmp->data;

		if (gp && !ggadu_strcasecmp(gp->plugin_name, plugin_name))
			return gp;

		tmp = tmp->next;
	}

	return NULL;
}

/*GGaduStatusPrototype *gui_find_status_prototype(GGaduProtocol * gp, gint status)
{
	GSList *tmp = NULL;

	if (gp == NULL)
		return NULL;

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
*/
GGaduContact *gui_find_user(gchar * id, gui_protocol * gp)
{
	GSList *ulist = NULL;

	if (!gp || !id)
		return NULL;

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
GtkWidget *create_image(const gchar * filename)
{
	GtkWidget *image = NULL;
	gchar *found_filename = NULL;
	GSList *dir = NULL;
	gchar *iconsdir = NULL;

	/* We first try any pixmaps directories set by the application. */
	dir = g_slist_prepend(dir, PACKAGE_DATA_DIR "/pixmaps");
	dir = g_slist_prepend(dir, PACKAGE_DATA_DIR "/pixmaps/emoticons");
#ifdef GGADU_DEBUG
	dir = g_slist_prepend(dir, PACKAGE_SOURCE_DIR "/pixmaps");
	dir = g_slist_prepend(dir, PACKAGE_SOURCE_DIR "/pixmaps/emoticons");
#endif
	dir = g_slist_prepend(dir, "/");

	if (ggadu_config_var_get(gui_handler, "icons"))
	{
		iconsdir = g_build_filename(PACKAGE_DATA_DIR, "pixmaps", "icons", ggadu_config_var_get(gui_handler, "icons"), NULL);
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
		print_debug("Couldn't find pixmap file: %s", filename);
		g_slist_free(dir);
		g_free(iconsdir);
		return NULL;
	}

	image = gtk_image_new_from_file(found_filename);

	g_slist_free(dir);
	g_free(iconsdir);
	g_free(found_filename);

	return image ? image : NULL;
}

GdkPixbuf *create_pixbuf(const gchar * filename)
{
	gchar *found_filename = NULL;
	GdkPixbuf *pixbuf = NULL;
	GSList *dir = NULL;
	GSList *dir_tmp = NULL;
	gchar *iconsdir = NULL;

	if (!filename || !filename[0])
		return NULL;

	/* We first try any pixmaps directories set by the application. */
	dir = g_slist_prepend(dir, PACKAGE_DATA_DIR "/pixmaps");
	dir = g_slist_prepend(dir, PACKAGE_DATA_DIR "/pixmaps/emoticons");
#ifdef GGADU_DEBUG
	dir = g_slist_prepend(dir, PACKAGE_SOURCE_DIR "/pixmaps");
	dir = g_slist_prepend(dir, PACKAGE_SOURCE_DIR "/pixmaps/emoticons");
#endif
	dir = g_slist_prepend(dir, "/");

	if (ggadu_config_var_get(gui_handler, "icons"))
	{
		iconsdir = g_build_filename(PACKAGE_DATA_DIR, "pixmaps", "icons", ggadu_config_var_get(gui_handler, "icons"), NULL);
		dir = g_slist_prepend(dir, iconsdir);
	}

	dir_tmp = dir;
	while (dir_tmp)
	{
		found_filename = check_file_exists((gchar *) dir_tmp->data, filename);

		if (found_filename)
			break;

		dir_tmp = dir_tmp->next;
	}

	/* If we haven't found the pixmap, try the source directory. */
	if (!found_filename)
		found_filename = check_file_exists("../pixmaps", filename);

	if (!found_filename)
	{
		print_debug("Couldn't find pixmap file: %s", filename);
		return NULL;
	}

	pixbuf = gdk_pixbuf_new_from_file(found_filename, NULL);

	/* there is memleak with data ot dir slist, but not sure how to free it right now
	   becaue of const strings */
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

gchar * 
ggadu_escape_html(const char *html) {
	const char *c = html;
	GString *ret;

	if (html == NULL)
		return NULL;

	ret = g_string_new("");

	while (*c) {
		switch (*c) {
			case '&':
				ret = g_string_append(ret, "&amp;");
				break;
			case '<':
				ret = g_string_append(ret, "&lt;");
				break;
			case '>':
				ret = g_string_append(ret, "&gt;");
				break;
			case '"':
				ret = g_string_append(ret, "&quot;");
				break;
			default:
				ret = g_string_append_c(ret, *c);
		}
		c++;
	}

	return g_string_free(ret, FALSE);
}


gboolean
gaim_str_has_prefix(const char *s, const char *p)
{
	if (!strncmp(s, p, strlen(p)))
		return TRUE;

	return FALSE;
}

void
gaim_str_strip_char(char *text, char thechar)
{
	int i, j;

	g_return_if_fail(text != NULL);

	for (i = 0, j = 0; text[i]; i++)
		if (text[i] != thechar)
			text[j++] = text[i];

	text[j++] = '\0';
}

char *
gaim_unescape_html(const char *html) {
	char *unescaped = NULL;

	if (html != NULL) {
		const char *c = html;
		GString *ret = g_string_new("");
		while (*c) {
			if (!strncmp(c, "&amp;", 5)) {
				ret = g_string_append_c(ret, '&');
				c += 5;
			} else if (!strncmp(c, "&lt;", 4)) {
				ret = g_string_append_c(ret, '<');
				c += 4;
			} else if (!strncmp(c, "&gt;", 4)) {
				ret = g_string_append_c(ret, '>');
				c += 4;
			} else if (!strncmp(c, "&quot;", 6)) {
				ret = g_string_append_c(ret, '"');
				c += 6;
			} else if (!strncmp(c, "&apos;", 6)) {
				ret = g_string_append_c(ret, '\'');
				c += 6;
			} else if (!strncmp(c, "<br>", 4)) {
				ret = g_string_append_c(ret, '\n');
				c += 4;
			} else {
				ret = g_string_append_c(ret, *c);
				c++;
			}
		}

		unescaped = ret->str;
		g_string_free(ret, FALSE);
	}
	return unescaped;

}

