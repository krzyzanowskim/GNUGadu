/* $Id: repo.h,v 1.3 2004/01/17 11:57:01 krzyzak Exp $ */

#ifndef GGadu_REPO_H
#define GGadu_REPO_H 1

#include <glib.h>
#include <limits.h>

#include "gg-types.h"

typedef void (*watch_ptr) (gchar *, gpointer, gint);

typedef struct
{
    gpointer key;
    gpointer value;
    gint type;
    GSList *watches;
} GGaduRepoValue;

typedef struct
{
    gchar *name;
    GSList *values;
    GSList *watches;
} GGaduRepo;

typedef struct
{
    gint actions;
    gint types;
    watch_ptr callback;
} GGaduRepoWatch;

gboolean ggadu_repo_check_value(gchar * repo_name, gpointer key);
gpointer ggadu_repo_find_value(gchar * repo_name, gpointer key);
gboolean ggadu_repo_exists(gchar * repo_name);

enum
{
    REPO_VALUE_DC = 0,		/* don't change */
    REPO_VALUE_CONTACT = 1,
    REPO_VALUE_SETTING = 2,
    REPO_VALUE_PROTOCOL = 4,
    REPO_VALUE_OTHER = 8,

    REPO_VALUE_ANY = INT_MAX
};

void ggadu_repo_disable_notification();
void ggadu_repo_enable_notification();

gboolean ggadu_repo_add(gchar * repo_name);
gboolean ggadu_repo_add_value(gchar * repo_name, gpointer key, gpointer value, gint type);
gboolean ggadu_repo_change_value(gchar * repo_name, gpointer key, gpointer value, gint type);
gboolean ggadu_repo_del_value(gchar * repo_name, gpointer key);
gboolean ggadu_repo_del(gchar * repo_name);

gpointer ggadu_repo_value_first(gchar * repo_name, gint type, gpointer * data);
gpointer ggadu_repo_value_next(gchar * repo_name, gint type, gpointer * data, gpointer index);
GSList*  ggadu_repo_get_as_slist(gchar * repo_name, gint type);

enum
{
    REPO_ACTION_NEW = 1,	/* dodanie nowego repo */
    REPO_ACTION_DEL = 2,	/* usuni�cie danego repo */
    REPO_ACTION_CHANGE = 4,	/* zmiana w danym repo (warto�ci) */
    REPO_ACTION_VALUE_NEW = 8,	/* nowa warto�� w repo */
    REPO_ACTION_VALUE_DEL = 16,	/* usuni�cie warto�ci */
    REPO_ACTION_VALUE_CHANGE = 32	/* zmiana warto�ci */
};

extern const gint REPO_mask;
extern const gint REPO_value_mask;

/* Czym si� r�ni REPO_ACTION_VALUE_* od REPO_ACTION_CHANGE?
 * Ano tym, �e REPO_ACTION_CHANGE stwierdza tylko, czy co� si� zmieni�o w repo.
 * A to co� to dowolna warto��.
 *
 * REPO_ACTION_VALUE_* tyczy si� tylko jednej, podanej warto�ci.
 *
 * Je�li jeden callback obs�uguje np. REPO_ACTION_VALUE_NEW
 * i REPO_ACTION_CHANGE,
 * to gdy zostanie dodana nowa warto��, to callback zostanie poinformowany
 * o REPO_ACTION_VALUE_NEW.
 *
 * Je�li jest usuwane repo, to niezale�nie od podanych masek, zawsze wysy�any
 * jest REPO_ACTION_DEL.
 *
 * Informacje o usuni�ciu s� wysy�ane przed usuni�ciem danych z repo, natomiast
 * inne informacje ju� po dodaniu/modyfikacji.
 */

/* GGaduRepo_watch_add(gchar *repo_name,gpointer key,gint actions,gpointer callback)
 *
 * repo_name - nazwa repo, w kt�rym jest nas�uch
 * actions   - maska, okre�laj�ca akcje, przy kt�rych wywo�ywany jest callback
 * key       - klucz, kt�ry ma by� sprawdzany (je�li w actions siedzi jedno
 *             z *_VALUE_*)
 * callback  - wska�nik do funkcji, przyjmuj�cej trzy argumenty:
 *             callback (gchar *repo_name, gpointer key, gint actions)
 *             actions zawiera informacje o akcji na danym repo lub key
 *
 * Kolejne wywo�ania GGaduRepo_watch_add z tymi samymi danymi (repo_name, key,
 * callback)
 * uaktualnia mask� akcji (dodaje brakuj�ce elementy).
 * 
 * GGaduRepo_watch_del() usuwa podane akcje odnosz�ce si� do danego repo
 * (repo_name), klucza
 * (key) oraz callbacku (callback). Je�li actions jest r�wne actions podanym
 * funkcji
 * GGaduRepo_watch_add(), to watch jest ca�kowicie usuwany.
 */
gboolean ggadu_repo_watch_add(gchar * repo_name, gint actions, gint types, watch_ptr callback);
gboolean ggadu_repo_watch_del(gchar * repo_name, gint actions, gint types, watch_ptr callback);

gboolean ggadu_repo_watch_value_add(gchar * repo_name, gpointer key, gint actions, watch_ptr callback);
gboolean ggadu_repo_watch_value_del(gchar * repo_name, gpointer key, gint actions, watch_ptr callback);

gboolean ggadu_repo_watch_clear_callback(watch_ptr callback);

#endif
