/* $Id: unified-types.h,v 1.6 2003/10/27 21:46:17 krzyzak Exp $ */
#ifndef GGadu_UNIFIED_TYPES_H
#define GGadu_UNIFIED_TYPES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

/*
 *    GGaduContact
 *    struktura opisujaca kontakt z dowolnego protokołu
 *    jeśli jakieś pole == NULL GUI powinno je zignorować
 */
 
typedef struct {
    gchar *id;		// unikalny identyfikator: numer GG, adres z tlen'u etc.
    gchar *first_name;	// imię
    gchar *last_name;	// nazwisko
    gchar *nick;	// pseudo
    gchar *mobile;	// tel. komórkowy
    gchar *email;	// adres e-mail
    gchar *gender;	// płeć
    gchar *group;	// grupa
    gchar *comment;	// komentarz
    gchar *birthdate;	// data urodzenia
    gchar *status_descr;// opis do statusu
    gchar *ip; 	// "IP:PORT"
    gchar *city;	// miasto
    gchar *age;	//wiek
    gint status;	// status w postaci liczbowej
} GGaduContact;

void GGaduContact_free(GGaduContact *k);

/*
 *	Klasy wiadomości
 */
 
enum {
    GGADU_CLASS_CHAT,
    GGADU_CLASS_MSG,
    GGADU_CLASS_CONFERENCE,
    GGADU_MSG_SEND,
    GGADU_MSG_RECV
};

enum {
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

typedef struct {
    gchar *id;
    gchar *message;
    guint class;
    guint time;
   
    /* conference */
    GSList *recipients;
    
} GGaduMsg;

void GGaduMsg_free(GGaduMsg *m);

/*
 *	GGaduNotify
 *	opisuje pojawienie sie kogos, lub zmiane stanu
 */

typedef struct {
    gchar *id;
    unsigned long status;
		gchar *ip; /* adres ip, nie musi byc ustawiony */
} GGaduNotify;

void GGaduNotify_free(GGaduNotify *n);

/*
 *	GGaduStatusPrototype
 *	prototyp statusu uzytownika danego protokołu
 *
 */ 
 
typedef struct {
    gint status;		// identyfikator statusu
    gchar *description;		// wyświetlany opis np. "Dostępny"
    gchar *image;		// nazwa pliku obrazeku statusu 
    gboolean receive_only;
} GGaduStatusPrototype;

void GGaduStatusPrototype_free(GGaduStatusPrototype *s);

typedef struct {
    gint	key;
    gpointer	value;

    guint	type; // VAR_STR, VAR_INT, VAR_BOOL
    guint	flag; // GGADU_INSENSITIVE, GGADU_SENSITIVE (default)
    
    gchar	*description;
    
    gpointer 	user_data;
} GGaduKeyValue;

void GGaduKeyValue_free(GGaduKeyValue *kv);

#endif
