/* $Id: support.c,v 1.4 2003/04/21 15:15:02 krzyzak Exp $ */

/*
 * (C) Copyright 2001-2002 Igor Popik. Released under terms of GPL license.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <glib.h>
#include <sys/stat.h>
#include <time.h>
#include "support.h"
#include "unified-types.h"

void set_userlist_status(GGaduNotify *n, gchar *status_descr, GSList *userlist)
{
    GSList *slistmp = userlist;
    
    if (slistmp == NULL) return;
    
    print_debug("set_userlist_status : id = %s, status = %d\n",n->id,n->status); 
    
    while (slistmp) 
    { 
		GGaduContact *k = slistmp->data;
	
		if ((k != NULL) && (!ggadu_strcasecmp(n->id,k->id))) {
	    	k->status = n->status;
				k->ip = n->ip;
	    
			if (k->status_descr) {
				g_free(k->status_descr);
				k->status_descr = NULL;
			}
	    
			if (status_descr)
				k->status_descr = status_descr;

	    break;
		}
	slistmp = slistmp->next;
	}
}



gboolean str_has_suffix (const gchar  *str, const gchar  *suffix) {
  int str_len = 0;
  int suffix_len = 0;
  
  g_return_val_if_fail (str != NULL, FALSE);
  g_return_val_if_fail (suffix != NULL, FALSE);

  str_len = strlen (str);
  suffix_len = strlen (suffix);

  if (str_len < suffix_len)
    return FALSE;

  return strcmp (str + str_len - suffix_len, suffix) == 0;
}

void print_debug_raw(const gchar *func, const char *format, ...)
{
#ifdef DEBUG
	static char *func_name;
	va_list ap;
	va_start(ap, format);
	
	if (func_name == NULL) func_name = g_strdup(func);
	
	if (strcmp(func_name,func))
	{
	    printf("%s()\n",func);
	    g_free(func_name);
	    func_name = g_strdup(func);
	}
	printf("\t");
	vprintf(format, ap);
	va_end(ap);
#endif
}

/*
 *
 *    base64_*() - autorstwa brygady RR czyli EKG team'u ;-)
 *
 */ 

static char base64_charset[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * base64_encode()
 *
 * zapisuje ci�g znak�w w base64. alokuje pami��. 
 */
char *base64_encode(const char *buf)
{
	char *out, *res;
	int i = 0, j = 0, k = 0, len = strlen(buf);
	
	res = out = g_malloc0((len / 3 + 1) * 4 + 2);

	if (!res)
		return NULL;
	
	while (j <= len) {
		switch (i % 4) {
			case 0:
				k = (buf[j] & 252) >> 2;
				break;
			case 1:
				if (j < len)
					k = ((buf[j] & 3) << 4) | ((buf[j + 1] & 240) >> 4);
				else
					k = (buf[j] & 3) << 4;

				j++;
				break;
			case 2:
				if (j < len)
					k = ((buf[j] & 15) << 2) | ((buf[j + 1] & 192) >> 6);
				else
					k = (buf[j] & 15) << 2;

				j++;
				break;
			case 3:
				k = buf[j++] & 63;
				break;
		}
		*out++ = base64_charset[k];
		i++;
	}

	if (i % 4)
		for (j = 0; j < 4 - (i % 4); j++, out++)
			*out = '=';
	
	*out = 0;
	
	return res;
}

/*
 * base64_decode()
 *
 * wczytuje ci�g znak�w base64, zwraca zaalokowany buforek.
 */
char *base64_decode(char *buf)
{
	char *res, *save, *end, *foo, val;
	int index = 0;
	
	if (!(save = res = calloc(1, (strlen(buf) / 4 + 1) * 3 + 2))) {
		g_print("// base64_decode() not enough memory\n");
		return NULL;
	}

	end = buf + strlen(buf);

	while (*buf && buf < end) {
		if (*buf == '\r' || *buf == '\n') {
			buf++;
			continue;
		}
		if (!(foo = strchr(base64_charset, *buf)))
			foo = base64_charset;
		val = (int)foo - (int)base64_charset;
		*buf = 0;
		buf++;
		switch (index) {
			case 0:
				*res |= val << 2;
				break;
			case 1:
				*res++ |= val >> 4;
				*res |= val << 4;
				break;
			case 2:
				*res++ |= val >> 2;
				*res |= val << 6;
				break;
			case 3:
				*res++ |= val;
				break;
		}
		index++;
		index %= 4;
	}
	*res = 0;
	
	return save;
}

void show_error(gchar *errstr) 
{
    g_error(errstr);
}

/* This is an internally used function to check if a pixmap file exists. */
gchar *check_file_exists(const gchar * directory, const gchar * filename)
{
	gchar *full_filename = NULL;
	
	if (directory != NULL) {
	    full_filename = g_build_filename(directory, filename,NULL);
	} else {
	    full_filename = g_strdup(filename);
	}
	
	if (g_file_test(full_filename,G_FILE_TEST_IS_REGULAR))
		return full_filename;
		
	g_free(full_filename);
	
	return NULL;
}



gchar *get_timestamp(time_t t)
{
	struct tm *tajm = NULL;
	time_t _t;
	gint max_l = 10; // 12:34:54\0
	gchar *buf = g_malloc0(max_l);
	
	if (t) 
	    _t = t;
	else
	    time(&_t);
	    
	tajm = localtime(&_t);
	
	strftime(buf, max_l, "%T", tajm);
	
//	free(tajm);  ZONK dlaczego nie mozna free ?

	return buf;
}


/* From EKG project
 * array_make()
 *
 * tworzy tablic� tekst�w z jednego, rozdzielonego podanymi znakami.
 *
 *  - string - tekst wej�ciowy,
 *  - sep - lista element�w oddzielaj�cych,
 *  - max - maksymalna ilo�� element�w tablicy. je�li r�wne 0, nie ma
 *          ogranicze� rozmiaru tablicy.
 *  - trim - czy wi�ksz� ilo�� element�w oddzielaj�cych traktowa� jako
 *           jeden (na przyk�ad spacje, tabulacja itp.)
 *  - quotes - czy pola mog� by� zapisywane w cudzys�owiach lub
 *             apostrofach z escapowanymi znakami.
 *
 * zaalokowan� tablic� z zaalokowanymi ci�gami znak�w, kt�r� nale�y
 * zwolni� funkcj� array_free()
 */
char **array_make(const char *string, const char *sep, int max, int trim, int quotes)
{
	const char *p, *q;
	char **result = NULL;
	int items, last = 0;

	for (p = string, items = 0; ; ) {
		int len = 0;
		char *token = NULL;

		if (max && items >= max - 1)
			last = 1;
		
		if (trim) {
			while (*p && strchr(sep, *p))
				p++;
			if (!*p)
				break;
		}

		if (!last && quotes && (*p == '\'' || *p == '\"')) {
			char sep = *p;

			for (q = p + 1, len = 0; *q; q++, len++) {
				if (*q == '\\') {
					q++;
					if (!*q)
						break;
				} else if (*q == sep)
					break;
			}

			if ((token = calloc(1, len + 1))) {
				char *r = token;
			
				for (q = p + 1; *q; q++, r++) {
					if (*q == '\\') {
						q++;
						
						if (!*q)
							break;
						
						switch (*q) {
							case 'n':
								*r = '\n';
								break;
							case 'r':
								*r = '\r';
								break;
							case 't':
								*r = '\t';
								break;
							default:
								*r = *q;
						}
					} else if (*q == sep) {
						break;
					} else 
						*r = *q;
				}
				
				*r = 0;
			}
			
			p = q + 1;
			
		} else {
			for (q = p, len = 0; *q && (last || !strchr(sep, *q)); q++, len++);
			token = calloc(1, len + 1);
			strncpy(token, p, len);
			token[len] = 0;
			p = q;
		}
		
		result = realloc(result, (items + 2) * sizeof(char*));
		result[items] = token;
		result[++items] = NULL;

		if (!*p)
			break;

		p++;
	}

	if (!items)
		result = calloc(1, sizeof(char*));

	return result;
}

const char *itoa(long int i)
{
	static char bufs[10][16];
	static int index = 0;
	char *tmp = bufs[index++];

	if (index > 9)
		index = 0;
	
	snprintf(tmp, 16, "%ld", i);

	return tmp;
}

gboolean write_line_to_file(gchar *path,gchar *line,gchar *enc)
{
	GIOChannel *ch;
	    
	ch = g_io_channel_new_file(path,"a+",NULL);
	g_io_channel_set_encoding(ch,enc,NULL);

	g_io_channel_write_chars(ch,line,-1,NULL,NULL);	    
	g_io_channel_shutdown(ch,TRUE,NULL);
	return TRUE;
}
