/* $Id: unified-types.h,v 1.10 2004/02/15 17:15:38 krzyzak Exp $ */

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

#ifndef GGadu_UNIFIED_TYPES_H
#define GGadu_UNIFIED_TYPES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

/*
 *    GGaduContact
 *    struktura opisujaca kontakt z dowolnego protoko³u
 *    je¶li jakie¶ pole == NULL GUI powinno je zignorowaæ
 */

typedef struct
{
    gchar *id;			/* unikalny identyfikator: numer GG, adres z tlen'u etc. */
    gchar *first_name;		/* imiê */
    gchar *last_name;		/* nazwisko */
    gchar *nick;		/* pseudo */
    gchar *mobile;		/* tel. komórkowy */
    gchar *email;		/* adres e-mail */
    gchar *gender;		/* p³eæ */
    gchar *group;		/* grupa */
    gchar *comment;		/* komentarz */
    gchar *birthdate;		/* data urodzenia */
    gchar *status_descr;	/* opis do statusu */
    gchar *ip;			/* "IP:PORT" */
    gchar *city;		/* miasto */
    gchar *age;			/* wiek */
    gint status;		/* status w postaci liczbowej */
} GGaduContact;

void GGaduContact_free(GGaduContact * k);

/*
 *	Klasy wiadomo¶ci
 */

enum
{
    GGADU_CLASS_CHAT,
    GGADU_CLASS_MSG,
    GGADU_CLASS_CONFERENCE,
    GGADU_MSG_SEND,
    GGADU_MSG_RECV
};

enum
{
    GGADU_SEARCH_FIRSTNAME,
    GGADU_SEARCH_LASTNAME,
    GGADU_SEARCH_NICKNAME,
    GGADU_SEARCH_CITY,
    GGADU_SEARCH_BIRTHYEAR,
    GGADU_SEARCH_GENDER,
    GGADU_SEARCH_ACTIVE,
    GGADU_SEARCH_ID,
    GGADU_SEARCH_EMAIL
};


/*
 *	GGaduMsg
 *	opisuje przesylana wiadomosc
 */

typedef struct
{
    gchar *id;
    gchar *message;
    guint class;
    guint time;

    /* conference */
    GSList *recipients;

} GGaduMsg;

void GGaduMsg_free(gpointer msg);

/*
 *	GGaduNotify
 *	opisuje pojawienie sie kogos, lub zmiane stanu
 */

typedef struct
{
    gchar *id;
    unsigned long status;
    gchar *ip;			/* adres ip, nie musi byc ustawiony */
} GGaduNotify;

void GGaduNotify_free(GGaduNotify * n);

/*
 *	GGaduStatusPrototype
 *	prototyp statusu uzytownika danego protoko³u
 *
 */

typedef struct
{
    gint status;		/* identyfikator statusu */
    gchar *description;		/* wy¶wietlany opis np. "Dostêpny" */
    gchar *image;		/* nazwa pliku obrazeku statusu */
    gboolean receive_only;
} GGaduStatusPrototype;

void GGaduStatusPrototype_free(GGaduStatusPrototype * s);

typedef struct
{
    gint key;
    gpointer value;

    guint type;			/* VAR_STR, VAR_INT, VAR_BOOL */
    guint flag;			/* GGADU_INSENSITIVE, GGADU_SENSITIVE (default) */

    gchar *description;

    gpointer user_data;
} GGaduKeyValue;

void GGaduKeyValue_free(GGaduKeyValue * kv);

#endif
