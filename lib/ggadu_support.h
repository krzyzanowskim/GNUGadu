/* $Id: ggadu_support.h,v 1.15 2004/12/26 22:23:16 shaster Exp $ */

/* 
 * GNU Gadu 2 
 * 
 * Copyright (C) 2001-2005 GNU Gadu Team 
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

/*! \file ggadu_support.h
    \brief Commonly used functions
*/


#ifndef GGadu_SUPPORT_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
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

#endif /* DOXYGEN_SHOULD_SHIP_THIS */


/*! @name Conversion Macros
*/

/*! \def to_iso
    \brief Macro that convert \a text from \a from_enc encoding to ISO-8859-2 encoding
*/
#define to_iso(from_enc,text) ggadu_convert(from_enc,"ISO-8859-2",text)

/*! \def to_cp
    \brief Macro that convert \a text from \a from_enc encoding to CP-1250 encoding
*/
#define to_cp(from_enc,text) ggadu_convert(from_enc,"CP1250",text)

/*! \def to_utf8
    \brief Macro that convert \a text from \a from_enc encoding to UTF-8 encoding
*/
#define to_utf8(from_enc,text) ggadu_convert(from_enc,"UTF-8",text)

/*! \def from_utf8
    \brief Macro that convert text to to_enc from UTF-8 encoding
*/
#define from_utf8(to_enc,text) ggadu_convert("UTF-8",to_enc,text)

/*! @name Debug
*/
/*! \def print_debug
    \brief Print debug information if compiled with --enable-debug option. Usage like printf()
*/
#define print_debug(...) print_debug_raw(__func__,__VA_ARGS__)

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define ggadu_iso_in_utf_str "łęóśążźćńĘŁÓĄŚŁŻŹĆŃ"
#endif

/*! @name Array related
*/
#define array_length(arr,type) sizeof(arr) / sizeof(type)
char		**array_make(const char *string, const char *sep, int max, int trim, int quotes);
void		array_free(char **array);
/*! @name Various functions */

gint		ggadu_strcasecmp(const gchar *s1,const gchar *s2);
gint		ggadu_strcmp(const gchar *s1,const gchar *s2);
gchar		*ggadu_get_image_path(const gchar * directory, const gchar * filename);
gboolean	ggadu_is_in_status(gint status, GSList * list);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
void		print_debug_raw(const gchar * func, const char *format, ...);
void		show_error(gchar * errstr);
const char	*itoa(long int i);
gchar		*get_timestamp(time_t t);
#endif

char		*base64_decode(char *);
char		*base64_encode(const char *buf);

gchar		*ggadu_strchomp(gchar * str);
gchar		*ggadu_convert(gchar * from_encoding, gchar * to_encoding, gchar * text);

gboolean	ggadu_save_history(GGaduHistoryType type, gchar *filepath, gchar *nick, GGaduMsg *msg);
gboolean	ggadu_write_line_to_file(gchar * path, gchar * line, gchar * enc);
gchar		*check_file_exists(const gchar * directory, const gchar * filename);

GGaduStatusPrototype *ggadu_find_status_prototype(GGaduProtocol * gp, gint status);


/*! @name DEPRECATED */
/* DEPRECATED */
void set_userlist_status(GGaduNotify * n, gchar * status_descr, GSList * userlist);
/* DEPRECATED */
GSList *ggadu_userlist_remove_id(GSList * userlist, gchar * id);
/* DEPRECATED */
GGaduContact *ggadu_find_contact_in_userlist(GSList * list, gchar * id);

#endif
