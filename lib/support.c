/* $Id: support.c,v 1.12 2004/01/17 00:44:58 shaster Exp $ */

/*
 * (C) Copyright 2001-2002 Igor Popik. Released under terms of GPL license.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <glib.h>
#include <sys/stat.h>
#include <time.h>
#include "support.h"
#include "unified-types.h"


gchar *ggadu_get_image_path(const gchar * directory, const gchar * filename)
{
    gchar *found_filename, *iconsdir;
    GSList *dir = NULL;

    /* We first try any pixmaps directories set by the application. */
    dir = g_slist_prepend(dir, PACKAGE_DATA_DIR "/pixmaps");
    dir = g_slist_prepend(dir, PACKAGE_DATA_DIR "/pixmaps/emoticons");
#ifdef GGADU_DEBUG
    dir = g_slist_prepend(dir, PACKAGE_SOURCE_DIR "/pixmaps");
    dir = g_slist_prepend(dir, PACKAGE_SOURCE_DIR "/pixmaps/emoticons");
#endif
    if (directory)
    {
	iconsdir = g_build_filename(PACKAGE_DATA_DIR, "pixmaps", "icons", directory, NULL);
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
	g_warning(_("Couldn't find pixmap file: %s"), filename);
	g_slist_free(dir);
	return NULL;
    }

    g_slist_free(dir);
    return found_filename;
}

gboolean is_in_status(gint status, GSList * list)
{
    GSList *tmp = list;
    gint st = 0;

    if (!list)
	return FALSE;

    while (tmp)
    {
	st = *(gint *) & tmp->data;
	if (st == status)
	    return TRUE;
	tmp = tmp->next;
    }

    return FALSE;
}

void set_userlist_status(GGaduNotify * n, gchar * status_descr, GSList * userlist)
{
    GSList *slistmp = userlist;

    if (slistmp == NULL)
	return;

    print_debug("set_userlist_status : id = %s, status = %d\n", n->id, n->status);

    while (slistmp)
    {
	GGaduContact *k = slistmp->data;

	if ((k != NULL) && (!ggadu_strcasecmp(n->id, k->id)))
	{
	    k->status = n->status;
	    k->ip = n->ip;

	    if (k->status_descr)
	    {
		g_free(k->status_descr);
		k->status_descr = NULL;
	    }

	    if ((status_descr) && (strlen(status_descr) > 0))
		k->status_descr = status_descr;

	    break;
	}
	slistmp = slistmp->next;
    }
}

GSList *ggadu_userlist_remove_id(GSList * userlist, gchar * id)
{

    GSList *slistmp = userlist;

    g_return_val_if_fail(userlist != NULL, NULL);
    g_return_val_if_fail(id != NULL, NULL);

    while (slistmp)
    {
	GGaduContact *k = slistmp->data;
	if (!ggadu_strcasecmp(k->id, id))
	    return g_slist_remove(userlist, k);
	slistmp = slistmp->next;
    }

    return NULL;
}

GGaduContact *ggadu_find_contact_in_userlist(GSList * list, gchar * id)
{
    GSList *tmp = list;
    GGaduContact *k;

    while (list)
    {
	k = (GGaduContact *) tmp->data;
	if (!ggadu_strcasecmp(k->id, id))
	    return k;
	list = list->next;
    }

    return NULL;
}

gboolean str_has_suffix(const gchar * str, const gchar * suffix)
{
    int str_len = 0;
    int suffix_len = 0;

    g_return_val_if_fail(str != NULL, FALSE);
    g_return_val_if_fail(suffix != NULL, FALSE);

    str_len = strlen(str);
    suffix_len = strlen(suffix);

    if (str_len < suffix_len)
	return FALSE;

    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

void print_debug_raw(const gchar * func, const char *format, ...)
{
#ifdef GGADU_DEBUG
    static char *prev_func_name = NULL;
    gchar *message = NULL;
    va_list ap;

    va_start(ap, format);

    if ((prev_func_name != NULL) && g_strcasecmp(prev_func_name, func))	/* if prev_func_name is NOT the same as func */
	g_print("\e[2mFunction : \e[0;36m%s\e[0m\n", func);

    g_free(prev_func_name);
    prev_func_name = g_strdup(func);
    message = g_strdup_vprintf(format, ap);
    g_print("  %s", message);

    if (message[strlen(message) - 1] != '\n')
	g_print("\n");

    g_free(message);
    va_end(ap);
#endif
}

/*
 *    base64_*() - autorstwa brygady RR czyli EKG team'u ;-)
 */

static char base64_charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * base64_encode()
 * zapisuje ci±g znaków w base64. alokuje pamiêæ. 
 */
char *base64_encode(const char *buf)
{
    char *out, *res;
    int i = 0, j = 0, k = 0, len = strlen(buf);

    res = out = malloc((len / 3 + 1) * 4 + 2);

    if (!res)
	return NULL;

    while (j <= len)
    {
	switch (i % 4)
	{
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
 * wczytuje ci±g znaków base64, zwraca zaalokowany buforek.
 */
char *base64_decode(char *buf)
{
    char *res, *save, *foo, val;
    const char *end;
    int index = 0;

    if (!buf)
	return NULL;

    save = res = calloc(1, (strlen(buf) / 4 + 1) * 3 + 2);

    if (!save)
	return NULL;

    end = buf + strlen(buf);

    while (*buf && buf < end)
    {
	if (*buf == '\r' || *buf == '\n')
	{
	    buf++;
	    continue;
	}
	if (!(foo = strchr(base64_charset, *buf)))
	    foo = base64_charset;
	val = (int) (foo - base64_charset);
	buf++;
	switch (index)
	{
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



void show_error(gchar * errstr)
{
    g_error(errstr);
}

/* This is an internally used function to check if a pixmap file exists. */
gchar *check_file_exists(const gchar * directory, const gchar * filename)
{
    gchar *full_filename = NULL;

    if (directory != NULL)
	full_filename = g_build_filename(directory, filename, NULL);
    else
	full_filename = g_strdup(filename);

    if (g_file_test(full_filename, G_FILE_TEST_IS_REGULAR))
	return full_filename;

    g_free(full_filename);
    return NULL;
}



gchar *get_timestamp(time_t t)
{
    struct tm *tajm = NULL;
    time_t _t;
    gint max_l = 10;		/* 12:34:54\0 */
    gchar *buf = g_malloc0(max_l);

    if (t)
	_t = t;
    else
	time(&_t);

    tajm = localtime(&_t);
    strftime(buf, max_l, "%T", tajm);

    return buf;
}


/* From EKG project
 * array_make()
 *
 * tworzy tablicê tekstów z jednego, rozdzielonego podanymi znakami.
 *
 *  - string - tekst wej¶ciowy,
 *  - sep - lista elementów oddzielaj±cych,
 *  - max - maksymalna ilo¶æ elementów tablicy. je¶li równe 0, nie ma
 *          ograniczeñ rozmiaru tablicy.
 *  - trim - czy wiêksz± ilo¶æ elementów oddzielaj±cych traktowaæ jako
 *           jeden (na przyk³ad spacje, tabulacja itp.)
 *  - quotes - czy pola mog± byæ zapisywane w cudzys³owiach lub
 *             apostrofach z escapowanymi znakami.
 *
 * zaalokowan± tablicê z zaalokowanymi ci±gami znaków, któr± nale¿y
 * zwolniæ funkcj± array_free()
 */
char **array_make(const char *string, const char *sep, int max, int trim, int quotes)
{
    const char *p, *q;
    char **result = NULL;
    int items, last = 0;

    for (p = string, items = 0;;)
    {
	int len = 0;
	char *token = NULL;

	if (max && items >= max - 1)
	    last = 1;

	if (trim)
	{
	    while (*p && strchr(sep, *p))
		p++;
	    if (!*p)
		break;
	}

	if (!last && quotes && (*p == '\'' || *p == '\"'))
	{
	    char sep = *p;

	    for (q = p + 1, len = 0; *q; q++, len++)
	    {
		if (*q == '\\')
		{
		    q++;
		    if (!*q)
			break;
		}
		else if (*q == sep)
		    break;
	    }

	    if ((token = calloc(1, len + 1)))
	    {
		char *r = token;

		for (q = p + 1; *q; q++, r++)
		{
		    if (*q == '\\')
		    {
			q++;

			if (!*q)
			    break;

			switch (*q)
			{
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
		    }
		    else if (*q == sep)
		    {
			break;
		    }
		    else
			*r = *q;
		}

		*r = 0;
	    }

	    p = q + 1;

	}
	else
	{
	    for (q = p, len = 0; *q && (last || !strchr(sep, *q)); q++, len++);
	    token = calloc(1, len + 1);
	    strncpy(token, p, len);
	    token[len] = 0;
	    p = q;
	}

	result = realloc(result, (items + 2) * sizeof (char *));
	result[items] = token;
	result[++items] = NULL;

	if (!*p)
	    break;

	p++;
    }

    if (!items)
	result = calloc(1, sizeof (char *));

    return result;
}

void array_free(char **array)
{
    char **tmp;

    if (!array)
	return;

    for (tmp = array; *tmp; tmp++)
	g_free(*tmp);

    g_free(array);
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

gboolean write_line_to_file(gchar * path, gchar * line, gchar * enc)
{
    GIOChannel *ch;
    gchar *dir;

    g_return_val_if_fail(path != NULL, FALSE);

    dir = g_path_get_dirname(path);
    if (strcmp(dir, ".") && !g_file_test(dir, G_FILE_TEST_EXISTS) && !g_file_test(dir, G_FILE_TEST_IS_DIR))
    {
	mkdir(dir, 0700);
    }
    else if (!g_file_test(dir, G_FILE_TEST_IS_DIR))
    {
	g_print("Unable to open/create directory %s\n", dir);
	g_free(dir);
	return FALSE;
    }
    g_free(dir);

    ch = g_io_channel_new_file(path, "a+", NULL);
    if (!ch)
    {
	g_print("Unable to open requested file %s for write\n", path);
	return FALSE;
    }
    g_io_channel_set_encoding(ch, enc, NULL);

    g_io_channel_write_chars(ch, line, -1, NULL, NULL);
    g_io_channel_shutdown(ch, TRUE, NULL);
    return TRUE;
}

gpointer ggadu_find_extension(GGaduPlugin * handler, gint type)
{
    GSList *extensions;

    if (!handler)
	return NULL;

    extensions = handler->extensions;
    while (extensions)
    {
	GGaduPluginExtension *ext = extensions->data;

	if (ext->type == type)
	    return ext;

	extensions = extensions->next;
    }
    return NULL;
}
