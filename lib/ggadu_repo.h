/* $Id: ggadu_repo.h,v 1.4 2004/12/26 22:23:16 shaster Exp $ */

/* 
 * GNU Gadu 2 
 * 
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

#ifndef GGadu_REPO_H
#define GGadu_REPO_H 1

#include <glib.h>
#include <limits.h>

#ifndef GG2_CORE_H
#include "ggadu_types.h"
#endif

typedef void (*watch_ptr) (gchar *, gpointer, gint);
typedef gpointer GGaduRepoKey;

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

GGaduRepoKey ggadu_repo_key_from_string(gchar * key);
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
    REPO_ACTION_DEL = 2,	/* usuniêcie danego repo */
    REPO_ACTION_CHANGE = 4,	/* zmiana w danym repo (warto¶ci) */
    REPO_ACTION_VALUE_NEW = 8,	/* nowa warto¶æ w repo */
    REPO_ACTION_VALUE_DEL = 16,	/* usuniêcie warto¶ci */
    REPO_ACTION_VALUE_CHANGE = 32	/* zmiana warto¶ci */
};

extern const gint REPO_mask;
extern const gint REPO_value_mask;

/* Czym siê ró¿ni REPO_ACTION_VALUE_* od REPO_ACTION_CHANGE?
 * Ano tym, ¿e REPO_ACTION_CHANGE stwierdza tylko, czy co¶ siê zmieni³o w repo.
 * A to co¶ to dowolna warto¶æ.
 *
 * REPO_ACTION_VALUE_* tyczy siê tylko jednej, podanej warto¶ci.
 *
 * Je¶li jeden callback obs³uguje np. REPO_ACTION_VALUE_NEW
 * i REPO_ACTION_CHANGE,
 * to gdy zostanie dodana nowa warto¶æ, to callback zostanie poinformowany
 * o REPO_ACTION_VALUE_NEW.
 *
 * Je¶li jest usuwane repo, to niezale¿nie od podanych masek, zawsze wysy³any
 * jest REPO_ACTION_DEL.
 *
 * Informacje o usuniêciu s± wysy³ane przed usuniêciem danych z repo, natomiast
 * inne informacje ju¿ po dodaniu/modyfikacji.
 */

/* GGaduRepo_watch_add(gchar *repo_name,gpointer key,gint actions,gpointer callback)
 *
 * repo_name - nazwa repo, w którym jest nas³uch
 * actions   - maska, okre¶laj±ca akcje, przy których wywo³ywany jest callback
 * key       - klucz, który ma byæ sprawdzany (je¶li w actions siedzi jedno
 *             z *_VALUE_*)
 * callback  - wska¼nik do funkcji, przyjmuj±cej trzy argumenty:
 *             callback (gchar *repo_name, gpointer key, gint actions)
 *             actions zawiera informacje o akcji na danym repo lub key
 *
 * Kolejne wywo³ania GGaduRepo_watch_add z tymi samymi danymi (repo_name, key,
 * callback)
 * uaktualnia maskê akcji (dodaje brakuj±ce elementy).
 * 
 * GGaduRepo_watch_del() usuwa podane akcje odnosz±ce siê do danego repo
 * (repo_name), klucza
 * (key) oraz callbacku (callback). Je¶li actions jest równe actions podanym
 * funkcji
 * GGaduRepo_watch_add(), to watch jest ca³kowicie usuwany.
 */
gboolean ggadu_repo_watch_add(gchar * repo_name, gint actions, gint types, watch_ptr callback);
gboolean ggadu_repo_watch_del(gchar * repo_name, gint actions, gint types, watch_ptr callback);

gboolean ggadu_repo_watch_value_add(gchar * repo_name, gpointer key, gint actions, watch_ptr callback);
gboolean ggadu_repo_watch_value_del(gchar * repo_name, gpointer key, gint actions, watch_ptr callback);

gboolean ggadu_repo_watch_clear_callback(watch_ptr callback);

#endif
