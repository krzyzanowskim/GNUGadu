/* $Id: unified-types.h,v 1.4 2003/04/04 15:17:31 thrulliq Exp $ */
#ifndef GGadu_UNIFIED_TYPES_H
#define GGadu_UNIFIED_TYPES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


/*
 *    GGaduContact
 *    struktura opisujaca kontakt z dowolnego protoko³u
 *    je¶li jakie¶ pole == NULL GUI powinno je zignorowaæ
 */
 
typedef struct {
    gchar *id;		// unikalny identyfikator: numer GG, adres z tlen'u etc.
    gchar *first_name;	// imiê
    gchar *last_name;	// nazwisko
    gchar *nick;	// pseudo
    gchar *mobile;	// tel. komórkowy
    gchar *email;	// adres e-mail
    gchar *gender;	// p³eæ
    gchar *group;	// grupa
    gchar *comment;	// komentarz
    gchar *birthdate;	// data urodzenia
    gchar *status_descr;// opis do statusu
    gint status;	// status w postaci liczbowej
} GGaduContact;

void GGaduContact_free(GGaduContact *k);

/*
 *	Klasy wiadomo¶ci
 */
 
enum {
    GGADU_CLASS_CHAT,
    GGADU_CLASS_MSG,
    GGADU_CLASS_CONFERENCE
};

enum {
    GGADU_SEARCH_FIRSTNAME,
    GGADU_SEARCH_LASTNAME,
    GGADU_SEARCH_NICKNAME,
    GGADU_SEARCH_CITY,
    GGADU_SEARCH_ACTIVE,
    GGADU_SEARCH_ID
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
//    guint recipients_count;
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
} GGaduNotify;

void GGaduNotify_free(GGaduNotify *n);

/*
 *	GGaduStatusPrototype
 *	prototyp statusu uzytownika danego protoko³u
 *
 */ 
 
typedef struct {
    guint status;		// identyfikator statusu
    gchar *description;		// wy¶wietlany opis np. "Dostêpny"
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

typedef struct {
    gchar *title;
    gchar *callback_signal;
    gint response;
    GSList *optlist; // lista elementów GGaduKeyValue
    gpointer user_data;
    gint type;
} GGaduDialog;

void GGaduDialog_free(GGaduDialog *d);

#endif

