/* $Id: gg-types.h,v 1.4 2003/04/03 11:07:46 krzyzak Exp $ */

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
    VAR_BOOL,
    VAR_IMG,
    VAR_FILE_CHOOSER,
    VAR_LIST
};

enum {
    VAR_FLAG_NONE         = 1,
    VAR_FLAG_SENSITIVE    = 1 << 2,
    VAR_FLAG_INSENSITIVE  = 1 << 4,
    VAR_FLAG_PASSWORD     = 1 << 5
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

/*
 * GGaduVar
 * Zmienne dla protokolu
 */
typedef struct {
    gchar *name; 			// nazwa zmiennej
    gint type;				// typ
    gpointer ptr;			// wska¼nik gdzie bêdzie przechowywana 
} GGaduVar;

/* 
 * GGaduProtocol
 * specyficzne dla protokolu 
 */ 
typedef struct {
    gchar *display_name;
    gchar *img_filename;	//¶cie¿ka do obrazka z etykiet± na zak³adkê
    GSList *statuslist;		// lista dostêpnych statusów danego protoko³u
    gint   offline_status;		// ktory status oznacza offline
} GGaduProtocol;

/* 
 * GGaduSignal 
 */
typedef struct {
    gpointer name;
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
    gpointer name;
//    void (*signal_free)(gpointer signal);
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
    GSList *variables;  		// zmienne czyane z pliku
    GSList *signals;			// lista zarejestrowanych signali
    
    GGaduProtocol *protocol; 		/* stuff specyficzna dla kazdego typu */
    
    void (*signal_receive_func)(gpointer,gpointer); // wskaznik na receiver signali
    void (*destroy_plugin)();			// wskaznik na zwalniacza plugina
    void (*start_plugin)();			// wskaznik na funkcjê startuj±c± plugina
} GGaduPlugin;


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
	guint main_on_start;		// poka¿ g³ówne okno po uruchomieniu programu
	guint width;
	guint height;
	gint pos_x;
	gint pos_y;
//	GSList *all_available_plugins;	// wszystkie dostepne, zainstalowane w systemie pluginy
	GSList *all_available_plugins;	// wszystkie dostepne, zainstalowane w systemie pluginy
	GSList *plugins;		// lista protokolow 

	gchar  *configdir;		// katalog z plikami konfiguracyjnymi programu do dowolnego wykorzystania przez plugin (.gg2)
	GSList *waiting_signals;
	gboolean all_plugins_loaded;
	GMainLoop *main_loop;
	
	GSource *signals_source;
} GGaduConfig;


enum {
    GGADU_OK,
    GGADU_CANCEL,
    GGADU_YES,
    GGADU_NO
};

#endif
