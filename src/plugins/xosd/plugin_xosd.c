/* $Id: plugin_xosd.c,v 1.28 2004/03/27 08:23:25 krzyzak Exp $ */

/*
 * XOSD plugin for GNU Gadu 2
 *
 * Copyright (C) 2003-2004 GNU Gadu Team
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
#include "ggadu_conf.h"
#include "signals.h"
#include "ggadu_menu.h"
#include "ggadu_support.h"
#include "ggadu_dialog.h"
#include "plugin_xosd.h"

/*
static gchar *font = "-adobe-helvetica-bold-r-normal--24-240-75-75-p-138,-*-*-*-R-Normal--*-180-100-100-*-*,-*-*-*-*-*--*-*-*-*-*-*";
*/
gint NUMLINES;
gchar *FONT;
gchar *COLOUR;
gint TIMEOUT;
gint SHADOW_OFFSET;
gint HORIZONTAL_OFFSET;
gint VERTICAL_OFFSET;
gint ALIGN;
gint POS;

gint fine = 1;
gint timer = -1;
GGaduPlugin *handler;
xosd *osd = NULL;

GGaduMenu *menu_pluginmenu;

GGadu_PLUGIN_INIT("xosd", GGADU_PLUGIN_TYPE_MISC);

/* helper */
gint ggadu_xosd_get_align(void)
{
	gchar *conf_align = (gchar *) ggadu_config_var_get(handler, "align");
	gint result = GGADU_XOSD_DEFAULT_ALIGN;	/* defaults to XOSD_center */

	if (conf_align)
	{
		if (!ggadu_strcasecmp(conf_align, "left"))
			result = XOSD_left;
		else if (!ggadu_strcasecmp(conf_align, "right"))
			result = XOSD_right;
		else if (!ggadu_strcasecmp(conf_align, "center"))
			result = XOSD_center;
		else
			print_debug("xosd: No align variable found, setting default\n");
	}
	else
		print_debug("xosd: No align variable found, setting default\n");

	return result;
}

/* helper */
gint ggadu_xosd_get_pos(void)
{
	gchar *conf_pos = (gchar *) ggadu_config_var_get(handler, "pos");
	gint result = GGADU_XOSD_DEFAULT_POS;	/* defaults to XOSD_top */

	if (conf_pos)
	{
		if (!ggadu_strcasecmp(conf_pos, "top"))
			result = XOSD_top;
		else if (!ggadu_strcasecmp(conf_pos, "bottom"))
			result = XOSD_bottom;
		else if (!ggadu_strcasecmp(conf_pos, "middle"))
			result = XOSD_middle;
		else
			print_debug("xosd: No pos variable found, setting default\n");
	}
	else
		print_debug("xosd: No pos variable found, setting default\n");

	return result;
}

void my_signal_receive(gpointer name, gpointer signal_ptr)
{
	GGaduSignal *signal = (GGaduSignal *) signal_ptr;

	print_debug("%s : receive signal %d\n", GGadu_PLUGIN_NAME, signal->name);

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
				case GGADU_XOSD_CONFIG_COLOUR:
					ggadu_config_var_set(handler, "colour", kv->value);
					break;
				case GGADU_XOSD_CONFIG_NUMLINES:
					ggadu_config_var_set(handler, "numlines", kv->value);
					break;
				case GGADU_XOSD_CONFIG_TIMEOUT:
					ggadu_config_var_set(handler, "timeout", kv->value);
					break;
				case GGADU_XOSD_CONFIG_TIMESTAMP:
					ggadu_config_var_set(handler, "timestamp", kv->value);
					break;
				case GGADU_XOSD_CONFIG_ALIGN:
					ggadu_config_var_set(handler, "align", ((GSList *)kv->value)->data);
					break;
				case GGADU_XOSD_CONFIG_POS:
					ggadu_config_var_set(handler, "pos", ((GSList *)kv->value)->data);
					break;
				case GGADU_XOSD_CONFIG_FONT:
					ggadu_config_var_set(handler, "font", kv->value);
					break;
				case GGADU_XOSD_CONFIG_SHADOW_OFFSET:
					ggadu_config_var_set(handler, "shadow_offset", kv->value);
					break;
				case GGADU_XOSD_CONFIG_HORIZONTAL_OFFSET:
					ggadu_config_var_set(handler, "horizontal_offset", kv->value);
					break;
				case GGADU_XOSD_CONFIG_VERTICAL_OFFSET:
					ggadu_config_var_set(handler, "vertical_offset", kv->value);
					break;
				}
				tmplist = tmplist->next;
			}
			ggadu_config_save(handler);
			set_configuration();
		}
		GGaduDialog_free(dialog);
		return;
	}

	if (signal->name == g_quark_from_static_string("xosd show message"))
	{
		gchar *data = signal->data;

		if (fine)
		{
			gchar *w, *msg;
			gpointer ts;

			w = from_utf8("ISO8859-2", data);

			if (((ts = ggadu_config_var_get(handler, "timestamp")) != NULL) && ((gboolean) ts == TRUE))
				msg = g_strdup_printf("[%s] %s", get_timestamp(0), w);
			else
				msg = g_strdup(w);

			if (timer != -1)
			{
				g_source_remove(timer);
				timer = -1;
				if (xosd_is_onscreen(osd))
					xosd_hide(osd);
			}

			xosd_scroll(osd, 1);
			xosd_display(osd, xosd_get_number_lines(osd) - 1, XOSD_string, msg);

			g_free(w);
			g_free(msg);
		}

		return;
	}
	return;
}

gint set_configuration(void)
{
	/* Set defaults */
	FONT = GGADU_XOSD_DEFAULT_FONT;
	COLOUR = GGADU_XOSD_DEFAULT_COLOUR;
	NUMLINES = GGADU_XOSD_DEFAULT_NUMLINES;
	TIMEOUT = GGADU_XOSD_DEFAULT_TIMEOUT;
	SHADOW_OFFSET = GGADU_XOSD_DEFAULT_SHADOW_OFFSET;
	HORIZONTAL_OFFSET = GGADU_XOSD_DEFAULT_HORIZONTAL_OFFSET;
	VERTICAL_OFFSET = GGADU_XOSD_DEFAULT_VERTICAL_OFFSET;
	ALIGN = ggadu_xosd_get_align();
	POS = ggadu_xosd_get_pos();

	if (!ggadu_config_var_check(handler, "numlines"))
		print_debug("xosd: No numlines config found, setting default\n");
	else
		NUMLINES = (gint) ggadu_config_var_get(handler, "numlines");

	if (NUMLINES < 1)
	{
		print_debug("xosd: NUMLINES < 1?! Are you nuts?!\n");
		NUMLINES = (gint) GGADU_XOSD_DEFAULT_NUMLINES;
	}

	if (osd)
	{
		if (xosd_is_onscreen(osd))
			xosd_hide(osd);

		xosd_destroy(osd);
	}

	/* create new xosd box */
	osd = xosd_create(NUMLINES);

	if (!osd)
	{
		fine = 0;
		return 0;
	}
	fine = 1;

	/* Read settings from configfile */
	if (!ggadu_config_var_check(handler, "font"))
		print_debug("xosd: No font config found, setting default\n");
	else
		FONT = (gchar *) ggadu_config_var_get(handler, "font");

	if (!ggadu_config_var_check(handler, "colour"))
		print_debug("xosd: No colour config found, setting default\n");
	else
		COLOUR = (gchar *) ggadu_config_var_get(handler, "colour");

	if (!ggadu_config_var_check(handler, "timeout"))
		print_debug("xosd: No timeout config found, setting default\n");
	else
		TIMEOUT = (gint) ggadu_config_var_get(handler, "timeout");

	if (!ggadu_config_var_check(handler, "shadow_offset"))
		print_debug("xosd: No shadow_offset config found, setting default\n");
	else
		SHADOW_OFFSET = (gint) ggadu_config_var_get(handler, "shadow_offset");

	if (!ggadu_config_var_check(handler, "horizontal_offset"))
		print_debug("xosd: No horizontal_offset config found, setting default\n");
	else
		HORIZONTAL_OFFSET = (gint) ggadu_config_var_get(handler, "horizontal_offset");

	if (!ggadu_config_var_check(handler, "vertical_offset"))
		print_debug("xosd: No vertical_offset config found, setting default\n");
	else
		VERTICAL_OFFSET = (gint) ggadu_config_var_get(handler, "vertical_offset");

	/* *INDENT-OFF* */
	print_debug("FONT=%s COLOUR=%s TIMEOUT=%d SHADOW_OFFSET=%d HORIZONTAL_OFFSET=%d VERTICAL_OFFSET=%d ALIGN=%d POS=%d\n", FONT, COLOUR, TIMEOUT, SHADOW_OFFSET, HORIZONTAL_OFFSET, VERTICAL_OFFSET, ALIGN, POS);
	/* *INDENT-ON* */

	/* Fallback to defaults if something fails */
	if (xosd_set_font(osd, FONT) == -1)
		xosd_set_font(osd, GGADU_XOSD_DEFAULT_FONT);

	if (xosd_set_colour(osd, COLOUR) == -1)
		xosd_set_colour(osd, GGADU_XOSD_DEFAULT_COLOUR);

	if (xosd_set_timeout(osd, TIMEOUT) == -1)
		xosd_set_timeout(osd, GGADU_XOSD_DEFAULT_TIMEOUT);

	if (xosd_set_shadow_offset(osd, SHADOW_OFFSET) == -1)
		xosd_set_shadow_offset(osd, GGADU_XOSD_DEFAULT_SHADOW_OFFSET);

	if (xosd_set_horizontal_offset(osd, HORIZONTAL_OFFSET) == -1)
		xosd_set_horizontal_offset(osd, GGADU_XOSD_DEFAULT_HORIZONTAL_OFFSET);

	if (xosd_set_vertical_offset(osd, VERTICAL_OFFSET) == -1)
		xosd_set_vertical_offset(osd, GGADU_XOSD_DEFAULT_VERTICAL_OFFSET);

	if (xosd_set_align(osd, ALIGN) == -1)
		xosd_set_align(osd, GGADU_XOSD_DEFAULT_ALIGN);

	if (xosd_set_pos(osd, POS) == -1)
		xosd_set_pos(osd, GGADU_XOSD_DEFAULT_POS);

	/* Say hello again */
	xosd_display(osd, 0, XOSD_string, GGADU_XOSD_WELCOME_STRING);

	return 1;
}

gboolean osd_hide_window(gpointer user_data)
{
	if (xosd_is_onscreen(osd))
		xosd_hide(osd);

	timer = -1;

	return FALSE;
}

gpointer osd_show_messages(gpointer user_data)
{
	if (timer != -1)
		return NULL;

	if (xosd_is_onscreen(osd))
		xosd_hide(osd);

	xosd_show(osd);

	timer = g_timeout_add((ggadu_config_var_get(handler, "timeout") ? (guint)
			       ggadu_config_var_get(handler, "timeout") * 1000 : 3000), osd_hide_window, NULL);

	return NULL;
}

gpointer osd_preferences(gpointer user_data)
{
	GGaduDialog *d = NULL;
	GSList *align_list = NULL;
	GSList *pos_list = NULL;
	gint align_cur = ggadu_xosd_get_align();
	gint pos_cur = ggadu_xosd_get_pos();

	print_debug("%s: Preferences\n", "XOSD");

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

	d = ggadu_dialog_new(GGADU_DIALOG_CONFIG, _("XOSD Preferences"), "update config");

	/* *INDENT-OFF* */
	ggadu_dialog_add_entry(d, GGADU_XOSD_CONFIG_TIMESTAMP, _("Timestamp"), VAR_BOOL, (gpointer) ggadu_config_var_get(handler,"timestamp"), VAR_FLAG_NONE);
	ggadu_dialog_add_entry(d, GGADU_XOSD_CONFIG_COLOUR, _("Colour"), VAR_COLOUR_CHOOSER, (gpointer) COLOUR, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(d, GGADU_XOSD_CONFIG_ALIGN, _("Alignment"), VAR_LIST, align_list, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(d, GGADU_XOSD_CONFIG_POS, _("Position"), VAR_LIST, pos_list, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(d, GGADU_XOSD_CONFIG_NUMLINES, _("Number of lines"), VAR_INT, (gpointer) NUMLINES, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(d, GGADU_XOSD_CONFIG_TIMEOUT, _("Timeout"), VAR_INT, (gpointer) TIMEOUT, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(d, GGADU_XOSD_CONFIG_HORIZONTAL_OFFSET, _("Horizontal offset"), VAR_INT, (gpointer) HORIZONTAL_OFFSET, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(d, GGADU_XOSD_CONFIG_VERTICAL_OFFSET, _("Vertical offset"), VAR_INT, (gpointer) VERTICAL_OFFSET, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(d, GGADU_XOSD_CONFIG_SHADOW_OFFSET, _("Shadow offset"), VAR_INT, (gpointer) SHADOW_OFFSET, VAR_FLAG_NONE);
	/* *INDENT-ON* */

	signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");

	g_slist_free(pos_list);
	g_slist_free(align_list);

	return NULL;
}

GGaduMenu *build_plugin_menu()
{
	GGaduMenu *root = ggadu_menu_create();
	GGaduMenu *item_gg = ggadu_menu_add_item(root, "XOSD", NULL, NULL);
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Preferences"), osd_preferences, NULL));
	ggadu_menu_add_submenu(item_gg, ggadu_menu_new_item(_("Show messages"), osd_show_messages, NULL));

	return root;
}

void start_plugin()
{
	print_debug("%s : start_plugin\n", GGadu_PLUGIN_NAME);

	/* Menu stuff */
	print_debug("%s : Create Menu\n", GGadu_PLUGIN_NAME);
	menu_pluginmenu = build_plugin_menu();
	signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_pluginmenu, "main-gui");

	if (!set_configuration())
		return;

	xosd_display(osd, 0, XOSD_string, GGADU_XOSD_WELCOME_STRING);
}

#ifdef PERL_EMBED
void perl_xosd_show_message(GGaduSignal * signal, gchar * perl_func, void *pperl)
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

	sv_name = sv_2mortal(newSVpv(g_quark_to_string(signal->name), 0));
	sv_src = sv_2mortal(newSVpv(signal->source_plugin_name, 0));
	if (signal->destination_plugin_name)
		sv_dst = sv_2mortal(newSVpv(signal->destination_plugin_name, 0));
	else
		sv_dst = sv_2mortal(newSVpv("", 0));
	sv_data = sv_2mortal(newSVpv(signal->data, 0));

	PUSHMARK(SP);
	XPUSHs(sv_name);
	XPUSHs(sv_src);
	XPUSHs(sv_dst);
	XPUSHs(sv_data);
	PUTBACK;

	count = call_pv(perl_func, G_DISCARD);

	if (count == 0)
	{
		gchar *dst;
		signal->name = g_quark_try_string(SvPV(sv_name, junk));
		signal->source_plugin_name = g_strdup(SvPV(sv_src, junk));
		dst = SvPV(sv_dst, junk);
		if (dst[0] != '\0')
			signal->destination_plugin_name = g_strdup(dst);
		signal->data = g_strdup(SvPV(sv_data, junk));
	}

	FREETMPS;
	LEAVE;
}
#endif

GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
	gchar *this_configdir = NULL;
	gchar *path = NULL;
	GGadu_PLUGIN_ACTIVATE(conf_ptr);
	
	print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);

	handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("On Screen Display"));

	register_signal(handler, "xosd show message");
#ifdef PERL_EMBED
	register_signal_perl("xosd show message", perl_xosd_show_message);
#endif
	register_signal(handler, "update config");

	print_debug("%s : READ CONFIGURATION\n", GGadu_PLUGIN_NAME);

	ggadu_config_var_add(handler, "font", VAR_STR);
	ggadu_config_var_add(handler, "colour", VAR_STR);
	ggadu_config_var_add(handler, "timeout", VAR_INT);
	ggadu_config_var_add(handler, "shadow_offset", VAR_INT);
	ggadu_config_var_add(handler, "horizontal_offset", VAR_INT);
	ggadu_config_var_add(handler, "vertical_offset", VAR_INT);
	ggadu_config_var_add(handler, "timestamp", VAR_BOOL);
	ggadu_config_var_add(handler, "align", VAR_STR);
	ggadu_config_var_add(handler, "pos", VAR_STR);
	ggadu_config_var_add(handler, "numlines", VAR_INT);

	if (g_getenv("HOME_ETC"))
		this_configdir = g_build_filename(g_getenv("HOME_ETC"), "gg2", NULL);
	else
		this_configdir = g_build_filename(g_get_home_dir(), ".gg2", NULL);

	path = g_build_filename(this_configdir, "xosd", NULL);
	ggadu_config_set_filename((GGaduPlugin *) handler, path);
	g_free(path);

	g_free(this_configdir);

	if (!ggadu_config_read(handler))
		g_warning(_("Unable to read configuration file for plugin %s"), "xosd");

	register_signal_receiver((GGaduPlugin *) handler, (signal_func_ptr) my_signal_receive);

	return handler;
}

void destroy_plugin()
{
	print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);

	if (timer != -1)
	{
		g_source_remove(timer);
		timer = -1;
	}

	/* free willy! */
	if (osd)
	{
		if (xosd_is_onscreen(osd))
			xosd_hide(osd);

		xosd_destroy(osd);
	}

	if (menu_pluginmenu)
	{
		signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", menu_pluginmenu, "main-gui");
		ggadu_menu_free(menu_pluginmenu);
	}
}
