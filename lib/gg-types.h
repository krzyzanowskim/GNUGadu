/* $Id: gg-types.h,v 1.12 2004/01/09 23:40:48 krzyzak Exp $ */

/*
 * (C) Copyright 2001-2002 Igor Popik. Released under terms of GPL license.
 */

#ifndef GGadu_TYPES_H
#define GGadu_TYPES_H

#include <glib.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

typedef gpointer (*function_ptr)(gpointer);
typedef void (*signal_func_ptr)(gpointer,gpointer);
typedef GQuark GGaduSigID;

enum {
    GGADU_PLUGIN_TYPE_UI = 1,
    GGADU_PLUGIN_TYPE_PROTOCOL,
    GGADU_PLUGIN_TYPE_MISC
};

enum {
    GGADU_ID = 1,
    GGADU_NICK,
    GGADU_FIRST_NAME,
    GGADU_LAST_NAME,
    GGADU_MOBILE,
    GGADU_PASSWORD
};


enum {
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
};

enum {
    GGADU_PLUGIN_EXTENSION_USER_MENU_TYPE = 1
};

enum {
    VAR_FLAG_NONE         = 1,
    VAR_FLAG_SENSITIVE    = 1 << 2,
    VAR_FLAG_INSENSITIVE  = 1 << 4,
    VAR_FLAG_PASSWORD     = 1 << 5,
    VAR_FLAG_FOCUS        = 1 << 8
};

enum {
    GGADU_NONE,
    GGADU_OK,
    GGADU_CANCEL,
    GGADU_YES,
    GGADU_NO
};


/*
 * GGaduMenu
 */
typedef GNode GGaduMenu;
typedef struct {
    gchar	*label;
    gpointer 	 data;
    gpointer     callback; //    function_ptr callback;
} GGaduMenuItem;

/* GGaduPluginExtension */
typedef struct {
    const gchar *txt;
    guint type;
    gpointer (*callback)(gpointer user_data);
} GGaduPluginExtension;

/*
 * GGaduVar
 * Zmienne dla protokolu
 */
typedef struct {
    gchar *name; 			// nazwa zmiennej
    gint type;				// typ
    gpointer ptr;			// wskaźnik gdzie będzie przechowywana 
} GGaduVar;

/* 
 * GGaduProtocol
 * specyficzne dla protokolu 
 */ 
typedef struct {
    gchar *display_name;
    gchar *img_filename;	//ścieżka do obrazka z etykietą na zakładkę
    GSList *statuslist;		// lista dostępnych statusów danego protokołu
    GSList *offline_status;	// ktory status oznacza offline
    GSList *away_status;	// ktory status oznacza away (NULL = brak)
    GSList *online_status;	// ktory status oznacza online
} GGaduProtocol;

/* 
 * GGaduSignal 
 */
typedef struct {
    GQuark name;	/* GQuark */
    gpointer source_plugin_name;
    gpointer destination_plugin_name;
    gpointer data;
    gpointer data_return;	// mozna to uzupelnic o jakies cos co ma byc zwrocone przez emit_signal
    gint     error;		// okresla czy jest jakis blad podczas wykonywania, -1 znaczy ze wszystko ok.
    gboolean free_me;
    void (*free)(gpointer signal);
} GGaduSignal;

/* 
 * GGaduSignalinfo
 */
typedef struct {
    GQuark name;
} GGaduSignalinfo;

/*
 * GGaduPlugin
 * Ta struktura jest przechowuje wszystkie informacje o pluginie
 */
typedef struct {
    guint  type;
    gchar  *name;			// nazwa zeby mozna bylo po niej znajdowac plugin
    gchar  *description;
    gpointer ptr;			// wskaznik na strukture charakterystyczna dla pluginu (np. protokolu)
    void *plugin_so_handler;

    gchar  *config_file;		// plik konfiguracyjny dla tego pluginu
    GSList *variables;  		// zmienne czytane z pliku
    GSList *signals;			// lista zarejestrowanych signali
    
    GGaduProtocol *protocol; 		/* stuff specyficzna dla kazdego typu */
    GSList *extensions;			/* GGaduPluginExtension's */
    
    void (*signal_receive_func)(gpointer,gpointer); // wskaznik na receiver signali
    void (*destroy_plugin)();			// wskaznik na zwalniacza plugina
    void (*start_plugin)();			// wskaznik na funkcję startującą plugina
} GGaduPlugin;


/*
 * GGaduPluginFile
 */
typedef struct {
    gchar *name;
    gchar *path;
} GGaduPluginFile;

/*
 * GGaduConfig
 */
typedef struct {
	/* globalnie niezalezne od protokolu */
	gboolean send_on_enter;
	guint main_on_start;		// pokaż główne okno po uruchomieniu programu
	guint width;
	guint height;
	gint pos_x;
	gint pos_y;
	
	gboolean all_plugins_loaded;	/* TRUE if all plugins are loaded */
	GSList *all_available_plugins;	// wszystkie dostepne, zainstalowane w systemie pluginy
	GSList *plugins;		// lista pluginow

	gchar  *configdir;		// katalog z plikami konfiguracyjnymi programu do dowolnego wykorzystania przez plugin (.gg2)
	
	GSList *waiting_signals;
	GSList *signal_hooks;
	
	GMainLoop *main_loop;

	gpointer repos;
} GGaduConfig;

/*
 *  GGaduSignalHook
 */
typedef struct {
  GQuark name;
  GSList *hooks;
  void (*perl_handler) (GGaduSignal *, gchar *, void *);
} GGaduSignalHook;

#endif
