/* $Id: ggadu_repo.c,v 1.4 2005/01/19 19:39:01 krzyzak Exp $ */

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

#include <glib.h>

#include "ggadu_repo.h"
#include "ggadu_types.h"
#include "signals.h"
#include "ggadu_support.h"

const gint REPO_mask = REPO_ACTION_NEW | REPO_ACTION_DEL | REPO_ACTION_CHANGE;
const gint REPO_value_mask = REPO_ACTION_VALUE_NEW | REPO_ACTION_VALUE_DEL | REPO_ACTION_VALUE_CHANGE;

gint silent_job = 0;

void ggadu_repo_disable_notification()
{
	silent_job = 1;
}

void ggadu_repo_enable_notification()
{
	silent_job = 0;
}

/********************************************************************
 *                         PRIVATE STUFF                            *
 ********************************************************************
 */

#define REPOS ((GGaduRepo *)(config->repos))

GGaduRepoKey ggadu_repo_key_from_string(gchar * key)
{
	return (GGaduRepoKey)g_quark_from_string(key);
}

GGaduRepo *ggadu_repo_find(gchar * repo_name)
{
	GGaduRepo *tmp;
	GSList *list;

	list = REPOS->values;
/*
    print_debug ("%p\n", REPOS->values);
*/
	while (list)
	{
		tmp = (GGaduRepo *) list->data;

		if (tmp && !ggadu_strcasecmp(repo_name, (gchar *) tmp->name))
			return tmp;

		list = list->next;
	}

	return NULL;
}

gpointer ggadu_repo_ptr_value(gchar * repo_name, gpointer key)
{
	GGaduRepo *tmp;
	GGaduRepoValue *tmpv;
	GSList *list;

	tmp = ggadu_repo_find(repo_name);

	list = tmp->values;
	while (list)
	{
		tmpv = (GGaduRepoValue *) list->data;

		if (tmpv->key == key)
			return tmpv;

		list = list->next;
	}

	return NULL;
}


void ggadu_repo_watch_notify(gchar * repo_name, gpointer key, gint actions, gint types)
{
	GGaduRepo *tmp;
	GGaduRepoValue *tmpv;
	GSList *list;
	GGaduRepoWatch *watch;
	GSList *completed = NULL;

	print_debug("repo: repo_name = '%s', key = '%p', actions = '%d', types = '%d'\n", repo_name, key, actions,
		    types);

	if (silent_job)
	{
		print_debug("repo: I was told to shut up.\n");
		return;
	}

	/* Wysy³amy ¿±dania w kolejno¶ci od najbardziej szczegó³owych do
	 * najbardziej ogólnych. Na pierwszy rzut id± patrza³ki na poszczególne
	 * warto¶ci.
	 */

	/* najpierw przelatujemy przez wszystkie warto¶ci */
	if ((actions & REPO_value_mask) && ggadu_repo_check_value(repo_name, key))
	{
		tmpv = ggadu_repo_ptr_value(repo_name, key);

		list = tmpv->watches;
		while (list)
		{
			watch = (GGaduRepoWatch *) list->data;
			if ((watch->actions & actions & REPO_value_mask) && (watch->types & types))
			{
/*
       	print_debug ("watch->actions: %d\n", watch->actions);
	print_debug ("       actions: %d\n", actions);
	print_debug ("          mask: %d\n", REPO_mask);
	print_debug ("    value mask: %d\n", REPO_value_mask);
*/
				watch->callback(repo_name, key, watch->actions & actions & REPO_value_mask);
				if (!g_slist_find(completed, watch->callback))
					completed = g_slist_append(completed, watch->callback);
			}
			list = list->next;
		}
	}

	/* teraz patrza³ki na konkretne repo */
	if ((actions & REPO_mask & (~REPO_ACTION_NEW)) && (tmp = ggadu_repo_find(repo_name)))
	{
		list = tmp->watches;
		while (list)
		{
			watch = (GGaduRepoWatch *) list->data;
			if ((watch->types & types) && !g_slist_find(completed, watch->callback))
			{
/*
       	print_debug ("WATCH->actions: %d\n", watch->actions);
	print_debug ("       actions: %d\n", actions);
	print_debug ("          mask: %d\n", REPO_mask);
	print_debug ("    value mask: %d\n", REPO_value_mask);
*/
				watch->callback(repo_name, key, watch->actions & actions);
				completed = g_slist_append(completed, watch->callback);
			}
			list = list->next;
		}
	}

	/* i w koñcu kierujemy patrza³ki na najbardziej ogólne - czyli na listê repów */
	if (actions & REPO_mask)
	{
		list = REPOS->watches;
		while (list)
		{
			watch = (GGaduRepoWatch *) list->data;
			if ((watch->types & types) && !g_slist_find(completed, watch->callback) &&
			    ((actions & watch->actions) || (watch->actions & REPO_mask && actions & REPO_value_mask)))
			{
/*
       	print_debug ("WaTcH->actions: %d\n", watch->actions);
	print_debug ("       actions: %d\n", actions);
	print_debug ("          mask: %d\n", REPO_mask);
	print_debug ("    value mask: %d\n", REPO_value_mask);
*/
				watch->callback(repo_name, key, watch->actions & actions);
			}
			list = list->next;
		}
	}

	if (completed)
		g_slist_free(completed);

	return;
}

/********************************************************************
 *                         PUBLIC  STUFF                            *
 ********************************************************************
 */

gboolean ggadu_repo_exists(gchar * repo_name)
{
/*
    print_debug ("repo: repo_name = '%s'\n", repo_name);
*/
	return ggadu_repo_find(repo_name) ? TRUE : FALSE;
}

gboolean ggadu_repo_check_value(gchar * repo_name, gpointer key)
{
	GGaduRepo *tmp;
	GGaduRepoValue *tmpv;
	GSList *list;
/*
    print_debug ("repo: repo_name = '%s', key = '%p'\n", repo_name, key);
*/
	tmp = ggadu_repo_find(repo_name);

	list = tmp->values;
	while (list)
	{
		tmpv = (GGaduRepoValue *) list->data;

		if (tmpv->key == key)
			return TRUE;

		list = list->next;
	}

	return FALSE;
}

gpointer ggadu_repo_find_value(gchar * repo_name, gpointer key)
{
	GGaduRepo *tmp;
	GGaduRepoValue *tmpv;
	GSList *list;
/*
    print_debug ("repo: repo_name = '%s', key = '%p'\n", repo_name, key);
*/
	tmp = ggadu_repo_find(repo_name);

	list = tmp->values;
	while (list)
	{
		tmpv = (GGaduRepoValue *) list->data;

		if (tmpv->key == key)
			return tmpv->value;

		list = list->next;
	}

	return NULL;
}

gboolean ggadu_repo_add(gchar * repo_name)
{
	GGaduRepo *tmp;
/*
    print_debug ("repo: repo_name = '%s'\n", repo_name);
*/
	if (ggadu_repo_find(repo_name))
	{
		return FALSE;
	}

	tmp = g_new0(GGaduRepo, 1);
	tmp->name = g_strdup(repo_name);
	tmp->values = NULL;
	tmp->watches = NULL;

	REPOS->values = g_slist_append(REPOS->values, tmp);

	ggadu_repo_watch_notify(repo_name, NULL, REPO_ACTION_NEW, REPO_VALUE_ANY);
	return TRUE;
}

gboolean ggadu_repo_add_value(gchar * repo_name, gpointer key, gpointer value, gint type)
{
	GGaduRepoValue *tmpv;
	GGaduRepo *tmp;
/*
    print_debug ("repo: repo_name = '%s', key = '%p', value = '%p', type = '%d'\n", repo_name, key, value, type);
*/
	if (ggadu_repo_check_value(repo_name, key))
	{
		return FALSE;
	}

	tmp = ggadu_repo_find(repo_name);

	tmpv = g_new0(GGaduRepoValue, 1);
	tmpv->key = key;
	tmpv->value = value;
	if (type == REPO_VALUE_DC)
		tmpv->type = REPO_VALUE_OTHER;
	else
		tmpv->type = type;
	tmpv->watches = NULL;

	tmp->values = g_slist_append(tmp->values, tmpv);

	ggadu_repo_watch_notify(repo_name, key, REPO_ACTION_CHANGE | REPO_ACTION_VALUE_NEW, type);
	return TRUE;
}

gboolean ggadu_repo_change_value(gchar * repo_name, gpointer key, gpointer value, gint type)
{
	GGaduRepoValue *tmpv;
/*
    print_debug ("repo: repo_name = '%s', key = '%p', value = '%p', type = '%d'\n", repo_name, key, value, type);
*/
	tmpv = ggadu_repo_ptr_value(repo_name, key);
	if (!tmpv)
		return FALSE;

	tmpv->value = value;

	if (type != REPO_VALUE_DC)
		tmpv->type = type;

	ggadu_repo_watch_notify(repo_name, key, REPO_ACTION_CHANGE | REPO_ACTION_VALUE_CHANGE, tmpv->type);
	return TRUE;
}

gboolean ggadu_repo_del_value(gchar * repo_name, gpointer key)
{
	GGaduRepo *tmp;
	GGaduRepoValue *tmpv;
/*
    print_debug ("repo: repo_name = '%s', key = '%p'\n", repo_name, key);
*/
	tmp = ggadu_repo_find(repo_name);

	if (!tmp)
		return FALSE;

	tmpv = ggadu_repo_ptr_value(repo_name, key);

	if (!tmpv)
		return FALSE;

	ggadu_repo_watch_notify(repo_name, key, REPO_ACTION_CHANGE | REPO_ACTION_VALUE_DEL, tmpv->type);

	tmp->values = g_slist_remove(tmp->values, tmpv);
	g_slist_free(tmp->watches);
	g_free(tmpv);

	return TRUE;
}

gboolean ggadu_repo_del(gchar * repo_name)
{
	GGaduRepo *tmp;
	GGaduRepoValue *tmpv;
	GSList *list;
/*
    print_debug ("repo: repo_name = '%s'\n", repo_name);
*/
	tmp = ggadu_repo_find(repo_name);
	if (!tmp)
		return FALSE;

	ggadu_repo_watch_notify(repo_name, NULL, REPO_ACTION_DEL | REPO_ACTION_VALUE_DEL, REPO_VALUE_ANY);

	list = tmp->values;
	while (list)
	{
		tmpv = (GGaduRepoValue *) list->data;
		g_slist_free(tmp->watches);
		g_free(tmpv);
		list = list->next;
	}

	g_slist_free(tmp->values);

	REPOS->values = g_slist_remove(REPOS->values, tmp);

	g_slist_free(tmp->watches);
	g_free(tmp->name);
	g_free(tmp);

	return TRUE;
}

gpointer ggadu_repo_value_first(gchar * repo_name, gint type, gpointer * data)
{
	GGaduRepo *tmpr;
/*
    print_debug ("repo: repo_name = '%s', type = '%d'\n", repo_name, type);
*/
	if (!(tmpr = ggadu_repo_find(repo_name)))
		return NULL;
	if (!tmpr->values || !(tmpr->values->data))
		return NULL;

	*data = ((GGaduRepoValue *) (tmpr->values->data))->key;
	return tmpr->values;
}

gpointer ggadu_repo_value_next(gchar * repo_name, gint type, gpointer * data, gpointer index)
{
	GSList *tmp;
/*
    print_debug ("repo: repo_name = '%s', type = '%d'\n", repo_name, type);
*/
	tmp = ((GSList *) index)->next;

	if (!tmp || !tmp->data)
		return NULL;

	*data = ((GGaduRepoValue *) (tmp->data))->key;
	return tmp;
}

GSList *ggadu_repo_get_as_slist(gchar * repo_name, gint type)
{
	gpointer index, key, val;
	GSList *list_from_repo = NULL;

	index = ggadu_repo_value_first(repo_name, type, (gpointer *) & key);
	while (index)
	{
		val = ggadu_repo_find_value(repo_name, key);
		list_from_repo = g_slist_append(list_from_repo, val);
		index = ggadu_repo_value_next(repo_name, type, (gpointer *) & key, index);
	}

	return list_from_repo;
}

gboolean ggadu_repo_watch_add(gchar * repo_name, gint actions, gint types, watch_ptr callback)
{
	GGaduRepo *tmp;
	GSList *list;
	GGaduRepoWatch *watch;
/*
    print_debug ("repo: repo_name = '%s', actions = '%d', types = '%d', callback = '%p'\n", repo_name, actions, types, callback);
*/
	if (!repo_name)
		tmp = REPOS;
	else
	{
		tmp = ggadu_repo_find(repo_name);
		if (!tmp)
			return FALSE;
	}

	list = tmp->watches;
	while (list)
	{
		watch = (GGaduRepoWatch *) list->data;
		if (watch->callback == callback)	/* mamy skubañca */
		{
			watch->actions = watch->actions | actions;
			watch->types = watch->types | types;
/*
      print_debug ("repo: %s on '%s' is now %d for callback %p\n",
	  repo_name ? "Watch":"Core watch",
	  repo_name ? repo_name:"(everything)",
	  watch->actions, watch->callback);
*/
			return TRUE;
		}
		list = list->next;
	}

	/* ojej, musimy ca³kiem od nowa dodaæ watcha */
	watch = g_new0(GGaduRepoWatch, 1);
	watch->callback = callback;
	watch->actions = actions;
	watch->types = types;
/*
  print_debug ("repo: %s on '%s' is now %d for callback %p\n",
      repo_name ? "Watch":"Core watch",
      repo_name ? repo_name:"(everything)",
      watch->actions, watch->callback);
*/
	tmp->watches = g_slist_append(tmp->watches, watch);
	return TRUE;
}

gboolean ggadu_repo_watch_del(gchar * repo_name, gint actions, gint types, watch_ptr callback)
{
	GGaduRepo *tmp;
	GSList *list;
	GGaduRepoWatch *watch;
/*
    print_debug ("repo: repo_name = '%s', actions = '%d', types = '%d', callback = '%p'\n", repo_name, actions, types, callback);
*/
	if (!repo_name)
		tmp = REPOS;
	else
	{
		tmp = ggadu_repo_find(repo_name);
		if (!tmp)
			return FALSE;
	}

	list = tmp->watches;
	while (list)
	{
		watch = (GGaduRepoWatch *) list->data;
		if (watch->callback == callback)	/* mamy skubañca */
		{
			watch->actions = watch->actions & ~actions;
			watch->types = watch->types & ~types;
/*
      print_debug ("repo: %s on '%s' is now %d for callback %p\n",
	  repo_name ? "Watch":"Core watch",
	  repo_name ? repo_name:"(everything)",
	  watch->actions, watch->callback);
*/
			if (watch->actions == 0)
			{
				tmp->watches = g_slist_remove(tmp->watches, watch);
				g_free(watch);
			}
			return TRUE;
		}
		list = list->next;
	}

	return FALSE;
}

gboolean ggadu_repo_watch_value_add(gchar * repo_name, gpointer key, gint actions, watch_ptr callback)
{
	GGaduRepoValue *tmpv;
	GSList *list;
	GGaduRepoWatch *watch;
/*
    print_debug ("repo: repo_name = '%s', actions = '%d', callback = '%p'\n", repo_name, actions, callback);
*/
	if (repo_name && (actions & REPO_value_mask))
	{
		if (!ggadu_repo_check_value(repo_name, key))
			return FALSE;

		tmpv = ggadu_repo_ptr_value(repo_name, key);

		list = tmpv->watches;
		while (list)
		{
			watch = (GGaduRepoWatch *) list->data;
			if (watch->callback == callback)	/* mamy skubañca */
			{
				watch->actions = watch->actions | (actions & REPO_value_mask);
/*
	print_debug ("repo: Watch on %p in '%s' is now %d for callback %p\n",
	    key, repo_name, watch->actions, watch->callback);
*/
				return TRUE;
			}
			list = list->next;
		}

		/* ojej, musimy ca³kiem od nowa dodaæ watcha */
		watch = g_new0(GGaduRepoWatch, 1);
		watch->callback = callback;
		watch->actions = actions & REPO_value_mask;
/*
    print_debug ("repo: Watch on %p in '%s' is now %d for callback %p\n",
	key, repo_name, watch->actions, watch->callback);
*/
		tmpv->watches = g_slist_append(tmpv->watches, watch);
		return TRUE;
	}

	return FALSE;
}

gboolean ggadu_repo_watch_value_del(gchar * repo_name, gpointer key, gint actions, watch_ptr callback)
{
	GGaduRepo *tmp;
	GGaduRepoValue *tmpv;
	GSList *list;
	GGaduRepoWatch *watch;
/*
    print_debug ("repo: repo_name = '%s', actions = '%d', callback = '%p'\n", repo_name, actions, callback);
*/
	if (repo_name && (actions & REPO_value_mask))
	{
		tmpv = ggadu_repo_find_value(repo_name, key);
		if (!tmpv)
			return FALSE;

		list = tmp->watches;
		while (list)
		{
			watch = (GGaduRepoWatch *) list->data;
			if (watch->callback == callback)	/* mamy skubañca */
			{
				watch->actions = watch->actions | ~(actions & REPO_value_mask);
/*
	print_debug ("repo: Watch on %p in '%s' is now %d for callback %p\n",
	    key, repo_name, watch->actions, watch->callback);
*/
				if (watch->actions == 0)
				{
					tmp->watches = g_slist_remove(tmp->watches, watch);
					g_free(watch);
				}
				return TRUE;
			}
			list = list->next;
		}

		return TRUE;
	}

	return FALSE;
}

void drop_callback(GSList ** watches, watch_ptr callback)
{
	GSList *list;
	GGaduRepoWatch *watch;

	list = *watches;
	while (list)
	{
		watch = (GGaduRepoWatch *) list->data;
		if (watch->callback == callback)
		{
			*watches = g_slist_remove(*watches, watch);
			g_free(watch);
			return;
		}
		list = list->next;
	}
}

gboolean ggadu_repo_watch_clear_callback(watch_ptr callback)
{
	GGaduRepo *tmp;
	GGaduRepo *tmpr;
	GGaduRepoValue *tmpv;
	GSList *list;
	GSList *listv;

	tmp = REPOS;

	/* najpierw id± patrza³ki na repos */
	drop_callback(&tmp->watches, callback);

	/* teraz przelatujemy przez ka¿de repo */
	list = tmp->values;
	while (list)
	{
		tmpr = (GGaduRepo *) list->data;
		drop_callback(&tmpr->watches, callback);

		listv = tmpr->values;
		while (listv)
		{
			tmpv = (GGaduRepoValue *) list->data;
			drop_callback(&tmpv->watches, callback);
			listv = listv->next;
		}

		list = list->next;
	}

	return FALSE;
}
