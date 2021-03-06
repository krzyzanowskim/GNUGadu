/* $Id: ggadu_support.c,v 1.26 2005/04/12 15:48:53 krzyzak Exp $ */

/* 
 * GNU Gadu 2 
 * 
 * Copyright (C) 2001-2002 Igor Popik
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <glib.h>
#include <sys/stat.h>
#include <time.h>
#include "ggadu_support.h"

/*! \fn ggadu_strcasecmp(const gchar *s1,const gchar *s2)
    \brief A case-insensitive string comparison
    
    corresponding to the standard strcasecmp() function on platforms which support it.
    Best if arguments is in UTF-8 encoding, but work for ASCII as well.
*/
gint ggadu_strcasecmp(const gchar *s1,const gchar *s2)
{
    gint ret = 0;
    gchar *casefold1 = g_utf8_casefold(s1 ? s1 : "",-1);
    gchar *casefold2 = g_utf8_casefold(s2 ? s2 : "",-1);
    
    ret = g_utf8_collate(casefold1,casefold2);

    g_free(casefold1);
    g_free(casefold2);
    return ret;
}

/*! \fn ggadu_strcmp(const gchar *s1,const gchar *s2)
    \brief A case sensitive string comparison
    
    corresponding to the standard strcmp() function on platforms which support it.
    Best if arguments is in UTF-8 encoding, but work for ASCII as well.
*/
gint ggadu_strcmp(const gchar *s1,const gchar *s2)
{
    gint ret = 0;
    ret = g_utf8_collate(s1,s2);
    return ret;
}

/*! \fn ggadu_convert(gchar * from_encoding, gchar * to_encoding, gchar * text)
    \brief Convert between various text encodings
    
    \param from_encoding Encoding code to convert from
    \param to_encoding Encoding code to convert to
    \param text Text to convert
    @return Newly-allocated string
*/
gchar *ggadu_convert(gchar * from_encoding, gchar * to_encoding, gchar * text)
{
	gchar *out = NULL;
	GError *err = NULL;

	if (!text)
		return NULL;

	if (!(out = g_convert(text, -1, to_encoding, from_encoding ? from_encoding : "UTF-8", NULL, NULL, &err)))
	{
		if (err != NULL)
		{
			g_warning("Unable to convert : %s", err->message);
			g_error_free(err);
			return NULL;
		}
		out = g_strdup(text);
	}
	
	if (err)
	    g_error_free(err);
	    
	return out;
}

/*! \fn ggadu_strchomp(gchar * str)
    \brief Removes trailing whitespace from a string.
    
    \param str a string to remove the trailing whitespace from.
    @return string
*/
gchar *ggadu_strchomp(gchar * str)
{
	gchar *string = str;

	if (!str || (strlen(str) == 0))
		return str;

	while (string[strlen(string) - 1] == '\n')
	{
		string = g_strchomp(string ? string : "");
	}

	return g_strdup(string);
}

/*! \fn ggadu_get_image_path(const gchar * directory, const gchar * filename)
    \brief Search for images in GNU Gadu resources
    
    \param directory Icon Theme name
    \param filename Requested graphics file name from Icon Theme
    @return Full path to requested file
*/
gchar *ggadu_get_image_path(const gchar * directory, const gchar * filename)
{
	gchar *found_filename, *iconsdir;
	GSList *dir = NULL;
	GSList *dir_tmp = NULL;

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
		g_warning(_("Couldn't find pixmap file: %s"), filename);
		g_slist_free(dir);
		return NULL;
	}

	/* still memeak with data of this list */
	g_slist_free(dir);
	return found_filename;
}

/*! \fn ggadu_is_in_status(gint status, GSList * list)
    \brief Check if status code is at given status list
    
    \param status Status code
    \param list Status list to look into
    @return TRUE if \a status is at the \a list
*/
gboolean ggadu_is_in_status(gint status, GSList * list)
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

/* DEPRECATED */
/*! @ingroup DEPRECATED
    \fn set_userlist_status(GGaduNotify * n, gchar * status_descr, GSList * userlist)
    \brief this function in DEPRECATED
*/
void set_userlist_status(GGaduNotify * n, gchar * status_descr, GSList * userlist)
{
	GSList *slistmp = userlist;

	print_debug("\e[0;33mTHIS FUNCTION IS DEPRECATED AS IT USE USERLIST INSTEAD REPO\e[0m\n");
	
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

/* DEPRECATED */
/*! @ingroup DEPRECATED
    \fn ggadu_userlist_remove_id(GSList * userlist, gchar * id)
    \brief this function in DEPRECATED
*/
GSList *ggadu_userlist_remove_id(GSList * userlist, gchar * id)
{

	GSList *slistmp = userlist;

	print_debug("\e[0;33mTHIS FUNCTION IS DEPRECATED AS IT USE USERLIST INSTEAD REPO\e[0m\n");
	
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

/* DEPRECATED */
/*! @ingroup DEPRECATED
    \fn ggadu_find_contact_in_userlist(GSList * list, gchar * id)
    \brief this function in DEPRECATED
*/
GGaduContact *ggadu_find_contact_in_userlist(GSList * list, gchar * id)
{
	GSList *tmp = list;
	GGaduContact *k;

	print_debug("\e[0;33mTHIS FUNCTION IS DEPRECATED AS IT USE USERLIST INSTEAD REPO\e[0m\n");
	
	while (tmp)
	{
		k = (GGaduContact *) tmp->data;
		if (!ggadu_strcasecmp(k->id, id))
			return k;
			
		tmp = tmp->next;
	}

	return NULL;
}

void print_debug_raw(const gchar * func, const char *format, ...)
{
#ifdef GGADU_DEBUG
	static char *prev_func_name = NULL;
	gchar *message = NULL;
	va_list ap;

	va_start(ap, format);

	if ((prev_func_name != NULL) && ggadu_strcasecmp(prev_func_name, func))	/* if prev_func_name is NOT the same as func */
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
 * zapisuje ci�g znak�w w base64. alokuje pami��. 
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
 * wczytuje ci�g znak�w base64, zwraca zaalokowany buforek.
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

/*! \fn check_file_exists(const gchar * directory, const gchar * filename)
    \brief Check if a file exists.
    
    @return full path to file or NULL if not exists
*/
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
	static gchar buf[10];
	struct tm *tajm = NULL;
	time_t _t;


	if (t)
		_t = t;
	else
		time(&_t);

	tajm = localtime(&_t);
	strftime(buf, 10, "%T", tajm);

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
			for (q = p, len = 0; *q && (last || !strchr(sep, *q)); q++, len++) ;
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

/*! \fn ggadu_write_line_to_file(gchar * path, gchar * line, gchar * enc)
    \brief Write a line to file
    
    \param path Path to file
    \param line One line text
    \param enc Encoding name used for this file
    
    @return TRUE on success
*/
gboolean ggadu_write_line_to_file(gchar * path, gchar * line, gchar * enc)
{
	GIOChannel *ch;
	gchar *dir;

	g_return_val_if_fail(path != NULL, FALSE);
	
	if (!path)
	    return FALSE;

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
	g_io_channel_unref(ch);
	
	return TRUE;
}

gboolean ggadu_save_history(GGaduHistoryType type, gchar *filepath, gchar *nick, GGaduMsg *msg)
{
	gchar *line = NULL;
	gchar *esc_line = NULL;
	gchar *final_line = NULL;
	gboolean ret = FALSE;
	
	g_return_val_if_fail(filepath != NULL,FALSE);
	g_return_val_if_fail(msg   != NULL,FALSE);
	g_return_val_if_fail(nick  != NULL,FALSE);
	
	switch (type)
	{
	    case GGADU_HISTORY_TYPE_RECEIVE:
		line = g_strdup_printf("chatrcv,%s,%s,%d,%d,%s", msg->id, nick, msg->time, (int)time(0), msg->message);
	    break;
	    case GGADU_HISTORY_TYPE_SEND:
		line = g_strdup_printf("chatsend,%s,%s,%d,%s", msg->id, nick, (int)time(0), msg->message);
	    break;
	}
	
	esc_line = g_strescape(line,"");
	final_line = g_strconcat(esc_line,"\n",NULL);
	ret = ggadu_write_line_to_file(filepath, final_line, NULL);

	g_free(final_line);
	g_free(esc_line);
	g_free(line);
	return ret; 
}

/*!
    aaaa
    @see GGaduProtocol
    @return newly allocated GGaduStatusPrototype
*/
GGaduStatusPrototype *ggadu_find_status_prototype(GGaduProtocol * gp, gint status)
{
	GSList *tmp = NULL;

	if (!gp || !gp->statuslist)
		return NULL;

	tmp = gp->statuslist;
	while (tmp)
	{
		GGaduStatusPrototype *sp = tmp->data;
		
		if ((sp) && (sp->status == status))
		{
			GGaduStatusPrototype *sp_ret = g_new0(GGaduStatusPrototype,1);

			sp_ret->status = sp->status;
			
			if (sp->description)
			    sp_ret->description = g_strdup(sp->description);

			if (sp->status_description)
			    sp_ret->status_description = g_strdup(sp->status_description);

			if (sp->image)
			    sp_ret->image = g_strdup(sp->image);

			sp_ret->receive_only = sp_ret->receive_only;
			return sp_ret;

//			return sp;
			
		}
		
		tmp = tmp->next;
	}
	return NULL;
}

gboolean ggadu_spawn(const gchar *command, const gchar *argument_value)
{
	gchar **argvp;
	gint  argcp;
	gboolean ret = FALSE;
	
	if (g_shell_parse_argv(command,&argcp,&argvp,NULL))
	{
		int childpid; // GPid
		gint i = 0;
		for (i=0;i<argcp;i++)
		{
		    if (!strcmp(argvp[i],"%s"))
		    {
			g_free(argvp[i]);
			argvp[i] = g_strdup(argument_value);
		    }
		    print_debug("SPAWN ARG[%d] %s",i,argvp[i]);
		}			
		ret = g_spawn_async(NULL,argvp,NULL,G_SPAWN_SEARCH_PATH,NULL,NULL,&childpid,NULL);
		g_strfreev(argvp);
	}
	
	return ret;
}
