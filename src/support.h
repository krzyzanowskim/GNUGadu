/* $Id: support.h,v 1.1 2003/03/20 10:37:05 krzyzak Exp $ */

#ifndef GGadu_SUPPORT_H
#define GGadu_SUPPORT_H 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <time.h>
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

#define ggadu_convert(from_encoding,to_encoding,text,out) \
	if (text != NULL) {	\
	    out = g_convert(text,-1,to_encoding,from_encoding,NULL,NULL,NULL); \
	    if (out == NULL) { \
		print_debug("ggadu_convert : bez zmiany!!!\n"); \
		out = g_strdup(text); \
	    } \
	}

#define to_iso(from_encoding,text,out) \
	if (text != NULL) {	\
	    out = g_convert(text,-1,"ISO-8859-2",from_encoding,NULL,NULL,NULL); \
	    if (out == NULL) { \
		print_debug("to_iso : bez zmiany!!!\n"); \
		out = g_strdup(text); \
	    } \
	}

#define to_cp(from_encoding,text,out) \
	if (text != NULL) {	\
	    out = g_convert(text,-1,"CP1250",(from_encoding) ? from_encoding : "UTF-8",NULL,NULL,NULL); \
	    if (out == NULL) { \
		print_debug("to_cp : bez zmiany!!!\n"); \
		out = g_strdup(text); \
	    } \
	}


#define to_utf8(from_encoding,text,out) \
	if (text != NULL) {	\
	    out = g_convert(text,-1,"UTF-8",from_encoding,NULL,NULL,NULL); \
	    if (out == NULL) { \
		print_debug("to_utf8 : bez zmiany!!!\n"); \
		out = g_strdup(text); \
	    } \
	}

#define from_utf8(to_encoding,text,out) \
	if (text != NULL) {	\
	    out = g_convert(text,-1,to_encoding,"UTF-8",NULL,NULL,NULL); \
	    if (out == NULL) { \
		print_debug("from_utf8 : bez zmiany!!!\n"); \
	        out = g_strdup(text); \
	    } \
	} else { \
	    out = NULL; \
	}

#define ggadu_strcasecmp(s1,s2) g_utf8_collate(g_utf8_casefold(s1,-1) , g_utf8_casefold(s2,-1))

#define print_debug(...) print_debug_raw(__func__,__VA_ARGS__)

gboolean str_has_suffix (const gchar  *str, const gchar  *suffix);

void print_debug_raw(const gchar *func, const char *format, ...);

char *base64_decode(char *);

char *base64_encode(const char *buf);

void show_error(gchar *errstr);

gchar *get_timestamp(time_t t);

/* This is an internally used function to check if a pixmap file exists. */
gchar *check_file_exists(const gchar * directory,const gchar * filename);

char **array_make(const char *string, const char *sep, int max, int trim, int quotes);

const char *itoa(long int i);

gboolean write_line_to_file(gchar *path,gchar *line,gchar *enc);

#endif

