/* $Id: ggadu_types.h,v 1.11 2004/10/19 10:51:24 krzyzak Exp $ */

/* 
 * GNU Gadu 2 
 * 
 * Copyright (C) 2001-2002 Igor Popik
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

#ifndef GGadu_TYPES_H
#define GGadu_TYPES_H

#include <glib.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

typedef gpointer(*function_ptr) (gpointer);
typedef void (*signal_func_ptr) (gpointer, gpointer);
typedef GQuark GGaduSigID;

typedef enum
{
    GGADU_PLUGIN_TYPE_UI = 1,
    GGADU_PLUGIN_TYPE_PROTOCOL,
    GGADU_PLUGIN_TYPE_MISC
} GGaduPluginType;

enum
{
    GGADU_ID = 1,
    GGADU_NICK,
    GGADU_FIRST_NAME,
    GGADU_LAST_NAME,
    GGADU_MOBILE,
    GGADU_PASSWORD
};

typedef enum
{
    VAR_STR = 1,
    VAR_INT,
    VAR_INT_WITH_NEGATIVE,
    VAR_BOOL,
    VAR_IMG,
    VAR_FILE_CHOOSER,
    VAR_FONT_CHOOSER,
    VAR_COLOUR_CHOOSER,
    VAR_LIST,
    VAR_NULL
} GGaduVarType;

typedef enum
{
    GGADU_PLUGIN_EXTENSION_USER_MENU_TYPE = 1
} GGaduPluginExtensionType;

typedef enum
{
    VAR_FLAG_NONE = 1,
    VAR_FLAG_SENSITIVE = 1 << 2,
    VAR_FLAG_INSENSITIVE = 1 << 4,
    VAR_FLAG_PASSWORD = 1 << 5,
    VAR_FLAG_SELECTED = 1 << 6,
    VAR_FLAG_FOCUS = 1 << 8
} GGaduKeyValueFlag;

enum
{
    GGADU_NONE,
    GGADU_OK,
    GGADU_CANCEL,
    GGADU_YES,
    GGADU_NO
};

typedef enum
{
    GGADU_HISTORY_TYPE_RECEIVE,
    GGADU_HISTORY_TYPE_SEND    
} GGaduHistoryType;


/*
 * GGaduMenu
 */
typedef GNode GGaduMenu;
typedef struct
{
    gchar *label;
    gpointer data;
    gpointer callback;		/* function_ptr callback; */
} GGaduMenuItem;

/* GGaduPluginExtension */
typedef struct
{
    const gchar *txt;
    guint type;
    gpointer(*callback) (gpointer user_data);
} GGaduPluginExtension;

/*
 * GGaduVar
 * Zmienne dla protokolu
 */
typedef struct
{
    gchar *name;		/* nazwa zmiennej */
    GGaduVarType type;		/* typ */
    gpointer ptr;		/* wskaznik gdzie bedzie przechowywana */
    gpointer def;		/* default value */
} GGaduVar;

/* 
 * GGaduProtocol
 * specyficzne dla protokolu 
 */
typedef struct
{
    gchar *display_name;
    gchar *img_filename;	/* ¶cie¿ka do obrazka z etykiet± na zak³adkê */
    gchar *status_description;	/* current description - description of current status */
    GSList *statuslist;		/* lista dostêpnych statusów danego protoko³u */
    GSList *offline_status;	/* ktory status oznacza offline */
    GSList *away_status;	/* ktory status oznacza away (NULL = brak) */
    GSList *online_status;	/* ktory status oznacza online */
} GGaduProtocol;

/* 
 * GGaduSignal 
 */
typedef struct
{
    GQuark name;		/* GQuark */
    gpointer source_plugin_name;
    gpointer destination_plugin_name;
    gpointer data;
    gpointer data_return;	/* mozna to uzupelnic o jakies cos co ma byc zwrocone przez emit_signal */
    gint error;			/* okresla czy jest jakis blad podczas wykonywania, -1 znaczy ze wszystko ok. */
    gboolean free_me;
    void (*free) (gpointer signal);
} GGaduSignal;

/* 
 * GGaduSignalinfo
 */
typedef struct
{
    GQuark name;
} GGaduSignalinfo;

/*
 * GGaduPlugin
 * Ta struktura jest przechowuje wszystkie informacje o pluginie
 */
typedef struct
{
    guint type;
    gchar *name;		/* nazwa zeby mozna bylo po niej znajdowac plugin */
    gchar *description;		/* descriptive text about plugin */
    gpointer ptr;		/* wskaznik na strukture charakterystyczna dla pluginu (np. protokolu) */
    void *plugin_so_handler;

    gchar *config_file;		/* plik konfiguracyjny dla tego pluginu */
    GSList *variables;		/* zmienne czytane z pliku */
    GSList *signals;		/* lista zarejestrowanych signali */

    GGaduProtocol *protocol;	/* stuff specyficzna dla kazdego typu */
    GSList *extensions;		/* GGaduPluginExtension's */

    void (*signal_receive_func) (gpointer, gpointer);	/* wskaznik na receiver signali */
    void (*destroy_plugin) ();	/* wskaznik na zwalniacza plugina */
    void (*start_plugin) ();	/* wskaznik na funkcjê startuj±c± plugina */
} GGaduPlugin;


/*
 * GGaduPluginFile
 */
typedef struct
{
    gchar *name;
    gchar *path;
} GGaduPluginFile;

/*
 * GGaduConfig
 */
typedef struct
{
    /* globalnie niezalezne od protokolu */
    gboolean send_on_enter;
    guint main_on_start;	/* pokaÅ¼ g³ówne okno po uruchomieniu programu */
    guint width;
    guint height;
    gint pos_x;
    gint pos_y;

    gboolean all_plugins_loaded;	/* TRUE if all plugins are loaded */
    GSList *all_available_plugins;	/* wszystkie dostepne, zainstalowane w systemie pluginy */
    GSList *plugins;		/* lista pluginow */

    gchar *configdir;		/* katalog z plikami konfiguracyjnymi programu do dowolnego wykorzystania przez plugin (.gg2) */

    GSList *waiting_signals;
    GSList *signal_hooks;

    GMainLoop *main_loop;

    gpointer repos;
} GGaduConfig;

extern GGaduConfig *config;

/*
 *  GGaduSignalHook
 */
typedef struct
{
    GQuark name;
    GSList *hooks;
    void (*perl_handler) (GGaduSignal *, gchar *, void *);
} GGaduSignalHook;

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
    gchar *resource;		/* zasob */
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

    GGaduVarType type;		/* GGaduVarType      */
    guint flag;			/* GGaduKeyValueFlag */

    gchar *description;

    gpointer user_data;
} GGaduKeyValue;

void GGaduKeyValue_free(GGaduKeyValue * kv);


#endif
