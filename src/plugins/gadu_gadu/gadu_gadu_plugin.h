/* $Id: gadu_gadu_plugin.h,v 1.25 2004/08/02 11:13:59 krzyzak Exp $ */

/* 
 * Gadu-Gadu plugin for GNU Gadu 2 
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

#ifndef GGadu_PROTOCOL_GADU_H
#define GGadu_PROTOCOL_GADU_H 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define ggadu_gg_save_history(_type,_msg,_nick)	\
	if (ggadu_config_var_get(handler, "log")) \
	{ \
	    gchar *path = g_build_filename(config->configdir, "history", _msg->id, NULL); \
	    ggadu_save_history(_type, path, _nick, _msg); \
	    g_free(path); \
	}
	

struct ggadu_gg_register
{
	gchar *email;
	gchar *password;
	gchar *token_id;
	gchar *token;
	gboolean update_config;
};

static GQuark CHANGE_STATUS_SIG;
static GQuark CHANGE_STATUS_DESCR_SIG;
static GQuark SEND_MESSAGE_SIG;
static GQuark ADD_USER_SIG;
static GQuark CHANGE_USER_SIG;
static GQuark UPDATE_CONFIG_SIG;
static GQuark SEARCH_SIG;
static GQuark EXIT_SIG;
static GQuark ADD_USER_SEARCH_SIG;
static GQuark GET_CURRENT_STATUS_SIG;
static GQuark SEND_FILE_SIG;
static GQuark GET_FILE_SIG;
static GQuark GET_USER_MENU_SIG;
static GQuark REGISTER_ACCOUNT;
static GQuark USER_REMOVE_USER_SIG;

enum
{
    GGADU_GADU_GADU_CONFIG_ID,
    GGADU_GADU_GADU_CONFIG_PASSWORD,
    GGADU_GADU_GADU_CONFIG_SERVER,
    GGADU_GADU_GADU_CONFIG_PROXY,
    GGADU_GADU_GADU_CONFIG_SOUND_CHAT_FILE,
    GGADU_GADU_GADU_CONFIG_SOUND_MSG_FILE,
    GGADU_GADU_GADU_CONFIG_SOUND_APP_FILE,
    GGADU_GADU_GADU_CONFIG_HISTORY,
    GGADU_GADU_GADU_CONFIG_AUTOCONNECT,
    GGADU_GADU_GADU_CONFIG_AUTOCONNECT_STATUS,
    GGADU_GADU_GADU_CONFIG_REASON,
    GGADU_GADU_GADU_CONFIG_FRIENDS_MASK,
    GGADU_GADU_GADU_CONFIG_DCC,
    GGADU_GADU_GADU_REGISTER_EMAIL,
    GGADU_GADU_GADU_REGISTER_PASSWORD,
    GGADU_GADU_GADU_REGISTER_TOKEN_ID,
    GGADU_GADU_GADU_REGISTER_TOKEN,
    GGADU_GADU_GADU_REGISTER_IMAGE,
    GGADU_GADU_GADU_REGISTER_UPDATE_CONFIG,
    GGADU_GADU_GADU_SELECTED_FILE,
    GGADU_GADU_GADU_CONTACT
};

GGaduPlugin *initialize_plugin(gpointer conf_ptr);

gpointer config_init_plugin(GGaduPlugin * plugin_handler);

gboolean test_chan(GIOChannel * source, GIOCondition condition, gpointer data);

gboolean test_chan_dcc(GIOChannel * source, GIOCondition condition, gpointer data);

gboolean test_chan_dcc_get(GIOChannel * source, GIOCondition condition, gpointer data);

void my_signal_receive(gpointer name, gpointer signal_ptr);

void start_plugin();

void destroy_plugin();

void gadu_gadu_enable_dcc_socket(gboolean state);

gpointer gadu_gadu_login(gpointer desc, gint status);

void wyjdz_signal_handler();

void load_contacts(gchar * encoding);

gboolean import_userlist(gchar * list);

void test();

void save_addressbook_file();

gpointer user_preferences_action(gpointer user_data);

#endif
