/* $Id: ggadu_support.h,v 1.8 2004/09/07 14:59:18 krzyzak Exp $ */

/* 
 * GNU Gadu 2 
 * 
 * Copyright (C) 2001-2004 GNU Gadu Team 
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

#ifndef GGadu_SUPPORT_H
#define GGadu_SUPPORT_H 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <time.h>
#include <stdio.h>

#ifndef GG2_CORE_H
#include "ggadu_types.h"
#endif
/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#define array_length(arr,type) sizeof(arr) / sizeof(type)
#define to_iso(from_enc,text) ggadu_convert(from_enc,"ISO-8859-2",text);
#define to_cp(from_enc,text) ggadu_convert(from_enc,"CP1250",text);
#define to_utf8(from_enc,text) ggadu_convert(from_enc,"UTF-8",text);
#define from_utf8(to_enc,text) ggadu_convert("UTF-8",to_enc,text);
#define ggadu_strcasecmp(s1,s2) g_utf8_collate(g_utf8_casefold(s1 ? s1 : "",-1) , g_utf8_casefold(s2 ? s2 : "",-1))
#define print_debug(...) print_debug_raw(__func__,__VA_ARGS__)

#define ggadu_iso_in_utf_str "łęóśążźćńĘŁÓĄŚŁŻŹĆŃ"


gchar		*ggadu_get_image_path(const gchar * directory, const gchar * filename);
gboolean	is_in_status(gint status, GSList * list);

void		print_debug_raw(const gchar * func, const char *format, ...);
void		show_error(gchar * errstr);

char		*base64_decode(char *);
char		*base64_encode(const char *buf);

gchar		*get_timestamp(time_t t);

char		**array_make(const char *string, const char *sep, int max, int trim, int quotes);
void		array_free(char **array);

gchar		*ggadu_strchomp(gchar * str);
gchar		*ggadu_convert(gchar * from_encoding, gchar * to_encoding, gchar * text);
const char	*itoa(long int i);
gboolean	str_has_suffix(const gchar * str, const gchar * suffix);

gboolean	ggadu_save_history(GGaduHistoryType type, gchar *filepath, gchar *nick, GGaduMsg *msg);
gboolean	write_line_to_file(gchar * path, gchar * line, gchar * enc);
/* This is an internally used function to check if a pixmap file exists. */
gchar		*check_file_exists(const gchar * directory, const gchar * filename);


/* DEPRECATED */
void set_userlist_status(GGaduNotify * n, gchar * status_descr, GSList * userlist);
/* DEPRECATED */
GSList *ggadu_userlist_remove_id(GSList * userlist, gchar * id);
/* DEPRECATED */
GGaduContact *ggadu_find_contact_in_userlist(GSList * list, gchar * id);

#endif
