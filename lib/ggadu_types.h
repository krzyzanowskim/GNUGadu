/* $Id: ggadu_types.h,v 1.20 2004/12/26 22:23:16 shaster Exp $ */

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

/*! \file ggadu_types.h */

#ifndef GGadu_TYPES_H
#define GGadu_TYPES_H

#include <glib.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

typedef gpointer(*function_ptr) (gpointer);
typedef void (*signal_func_ptr) (gpointer, gpointer);

/*! \brief Signal ID */
typedef GQuark GGaduSigID;

/*! \brief Plugin type used in GGadu_PLUGIN_INIT(x,type) 
    @see GGaduPlugin
    @see GGadu_PLUGIN_INIT
*/
typedef enum
{
    GGADU_PLUGIN_TYPE_UI = 1,   /*!< User Interface type plugin */
    GGADU_PLUGIN_TYPE_PROTOCOL, /*!< Protocol type plugin */
    GGADU_PLUGIN_TYPE_MISC,     /*!< Misc. type plugin */
    GGADU_PLUGIN_TYPE_ANY       /*!< for internal use only */
} GGaduPluginType;

/*! \brief Some kind of enum, don't know why it is here */
enum
{
    GGADU_ID = 1,
    GGADU_NICK,
    GGADU_FIRST_NAME,
    GGADU_LAST_NAME,
    GGADU_MOBILE,
    GGADU_PASSWORD
} GGaduContactFieldType;

/*! \brief Type of variable
    @see GGaduKeyValue
*/
typedef enum
{
    VAR_STR = 1,		/*!< string  */
    VAR_INT,			/*!< integer */
    VAR_INT_WITH_NEGATIVE,	/*!< integer with negative values */
    VAR_BOOL,			/*!< boolean value */
    VAR_IMG,			/*!< image type */
    VAR_FILE_CHOOSER,		/*!< file chooser type */
    VAR_FONT_CHOOSER,		/*!< font chooser type */
    VAR_COLOUR_CHOOSER,		/*!< colour chooser type */
    VAR_LIST,			/*!< list type */
    VAR_NULL			/*!< null - means nothing */
} GGaduVarType;


/*! \brief Flags for GGaduKeyValue structure 
    @see GGadyKeyValue
*/
typedef enum
{
    VAR_FLAG_NONE = 1,			/*!< Empty, no flag */
    VAR_FLAG_SENSITIVE = 2,	/*!< Widget is sensitive */
    VAR_FLAG_INSENSITIVE = 4,	/*!< Widget is insensitive */
    VAR_FLAG_PASSWORD = 8,		/*!< Password type input */
    VAR_FLAG_SELECTED = 16,		/*!< Mark selected by default */
    VAR_FLAG_FOCUS = 32,		/*!< Grab focus */
    VAR_FLAG_ADVANCED = 64		/*!< Put option into advanced box */
} GGaduKeyValueFlag;

/*! common enums */
enum
{
    GGADU_NONE,
    GGADU_OK,
    GGADU_CANCEL,
    GGADU_YES,
    GGADU_NO
};

/*! \brief History entries */
typedef enum
{
    GGADU_HISTORY_TYPE_RECEIVE,	/*!< Input message */
    GGADU_HISTORY_TYPE_SEND    	/*!< Output message */
} GGaduHistoryType;


/*! \brief Root menu item 
    @see GGaduMenuItem
*/
typedef GNode GGaduMenu;

/*! \brief Menu item
    @see GGaduMenu
*/
typedef struct
{
    gchar *label;		/*!< Label for menu item */
    gpointer data;		/*!< data */
    gpointer callback;		/*!< function_ptr callback for this menu item */
} GGaduMenuItem;

/*! \brief Types of extensions 
    @see GGaduPluginExtension
*/
typedef enum
{
    GGADU_PLUGIN_EXTENSION_USER_MENU_TYPE = 1
} GGaduPluginExtensionType;


/*! \brief Plugins extensions
    @see GGaduPluginExtensionType
*/
typedef struct
{
    const gchar *txt;				/*!< extension label */
    GGaduPluginExtensionType type;		/*!< extension type  */
    gpointer(*callback) (gpointer user_data);	/*!< extension callback */
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

/*! \brief Protocol description data
    GGaduPlugin ->plugin_data where type is  \a GGADU_PLUGIN_TYPE_PROTOCOL should point to this structure
    @see GGaduPluginType
    @see GGaduPlugin
*/
typedef struct
{
    gchar *display_name;	/*!< displayable name of protocol */
    gchar *protocol_uri; 	/*!< URI for protocols eg.: gg://  icq:// tlen://  */
    gchar *img_filename;	/*!< Path to picture with protocol label */
    GSList *statuslist;		/*!< List of available status codes */
    GSList *offline_status;	/*!< List of "offline" status codes */
    GSList *away_status;	/*!< List of "away" status codes */
    GSList *online_status;	/*!< List of "online" status codes */
} GGaduProtocol;

/*! Internal SIGNAL structure used by core */
typedef struct
{
    GGaduSigID name;		/*!< integer representation of signal name */
    gchar *source_plugin_name;	/*!< source plugin name */
    gchar *destination_plugin_name; /*!< destination plugin name */
    gpointer data;		/*!< data send with this signal */
    gpointer data_return;	/*!< data that should be returned by emit_sianal function */
    gint error;			/*!< error flag, -1 mean no error */
    gboolean free_me;		/*!< mark if signal sould be freed with given function, TRUE by default */
    void (*free) (gpointer signal); /*!< callback to function which can free data send with signal */
} GGaduSignal;

/*! GGaduSignalInfo */
typedef struct
{
    GGaduSigID name; 		/*!< integer representation of signal name */
} GGaduSignalInfo;

/*! Structure of data specific for plugins */
typedef struct
{
    GGaduPluginType type;	/*!< type of plugin */
    gchar *name;		/*!< plugin name */
    gchar *description;		/*!< descriptive text about plugin */
    gpointer plugin_data;	/*!< data specific for types of plugins, ex. protocols have GGaduProtocol* */
    void *plugin_so_handler;

    gchar *config_file;		/* plik konfiguracyjny dla tego pluginu */
    GSList *variables;		/* zmienne czytane z pliku */
    GSList *signals;		/* lista zarejestrowanych signali */

    GSList *extensions;		/* GGaduPluginExtension's */

    void (*signal_receive_func) (gpointer, gpointer);	/* wskaznik na receiver signali */
    void (*destroy_plugin) ();	/* wskaznik na zwalniacza plugina */
    void (*start_plugin) ();	/* wskaznik na funkcjê startuj±c± plugina */
} GGaduPlugin;


/*! \brief Represent data about plugin file */
typedef struct
{
    gchar *name;	/*!< plugin name */
    gchar *path;	/*!< plugin file path */
} GGaduPluginFile;

/*! \brief Global configuration data accessible from any plugin */
typedef struct
{
    gboolean send_on_enter;	/*!< shuldn't be here */
    guint main_on_start;	/*!< show main window on start ? (shouldn't be here) */
    guint width;		/*!< shuldn't be here */
    guint height;		/*!< shuldn't be here */
    gint pos_x;			/*!< shuldn't be here */
    gint pos_y;			/*!< shuldn't be here */
    gboolean all_plugins_loaded;	/*!< TRUE if all plugins are loaded */
    GSList *all_available_plugins;	/*!< All available plugins */
    GSList *loaded_plugins;		/*!< All loaded plugins */
    gchar *configdir;			/*!< Main configuration data directory reflecting HOME_ETC etc. (~/.gg2) */
    gpointer repos;			/*!< Pointer to repos */
    GMainLoop *main_loop;		/*!< GMainLoop used by GNU Gadu */
    GSList *waiting_signals;		/*!< private used by internal core */
    GSList *signal_hooks;		/*!< private used by core */
    int argc;
    char **argv;
} GGaduConfig;

/*! \brief Main config available for any plugin */
extern GGaduConfig *config;

/*
 *  GGaduSignalHook
 */
typedef struct
{
    GGaduSigID name;
    GSList *hooks;
    void (*perl_handler) (GGaduSignal *, gchar *, void *);
} GGaduSignalHook;

/*! \brief Universal contact data structure
    if some value is NULL then GUI should ignore such entry
    @see GGaduContact_free
*/
typedef struct
{
    gchar *id;			/*!< unique id: GG number, Jabber jid, etc.. */
    gchar *first_name;		/*!< Name */
    gchar *last_name;		/*!< Surname */
    gchar *nick;		/*!< Nick */
    gchar *mobile;		/*!< Mobile */
    gchar *email;		/*!< Email */
    gchar *gender;		/*!< Gender */
    gchar *group;		/*!< Group */
    gchar *comment;		/*!< Comment */
    gchar *birthdate;		/*!< Birth Date */
    gchar *ip;			/*!< IP address "IP:PORT" */
    gchar *city;		/*!< City */
    gchar *age;			/*!< Age */
    gchar *resource;		/*!< Resource (ex. Jabber) */
    gint status;		/*!< Status code */
    gchar *status_descr;	/*!< Status Description */
} GGaduContact;

void 	      GGaduContact_free(GGaduContact * k);
GGaduContact *GGaduContact_copy(GGaduContact * k);


/*! \brief Classes of message */
typedef enum
{
    GGADU_CLASS_CHAT,
    GGADU_CLASS_MSG,
    GGADU_CLASS_CONFERENCE,
    GGADU_MSG_SEND,
    GGADU_MSG_RECV
} GGaduMessageClass;

/*! \brief some useful constants to search engine */
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


/*! \brief Structure to send message */
typedef struct
{
    gchar *id;		/*!< Recipient ID */
    gchar *message;	/*!< Message in UTF-8 */
    guint time;		/*!< Time of message */
    GGaduMessageClass class;	/*!< Kind of message */
    GSList *recipients;	/*!< Holds recipients of message if GGADU_CLASS_CONFERENCE */
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

/*! \brief Status prototype structure
*/
typedef struct
{
    gint status;		 /*!< Status code */
    gchar *status_description;   /*!< Status description */
    gchar *description;		 /*!< Status label np. "Available" */
    gchar *image;		 /*!< Image name  */
    gboolean receive_only;	 /*!< Identify if status is only received by other contacts */
} GGaduStatusPrototype;

void GGaduStatusPrototype_free(GGaduStatusPrototype * s);

/*! \brief Key<->Value pair of data 
    @see GGaduKeyValueFlag
*/
typedef struct
{
    gint key;			/*!< Key */
    gpointer value;		/*!< Value for this key */
    GGaduVarType type;		/*!< GGaduVarType      */
    guint flag;			/*!< GGaduKeyValueFlag */
    gchar *description;		/*!< Description assigned to this key */
    gpointer user_data;		/*!< User data assigned to this structure */
} GGaduKeyValue;

void GGaduKeyValue_free(GGaduKeyValue * kv);


#endif
