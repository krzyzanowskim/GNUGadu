/* $Id: repo.c,v 1.2 2003/04/12 16:38:53 zapal Exp $ */

#include <glib.h>

#include "repo.h"
#include "gg-types.h"
#include "signals.h"
#include "support.h"

extern GGaduConfig *config;

const gint REPO_mask       = REPO_ACTION_NEW | REPO_ACTION_DEL | REPO_ACTION_CHANGE;
const gint REPO_value_mask = REPO_ACTION_VALUE_NEW | REPO_ACTION_VALUE_DEL | REPO_ACTION_VALUE_CHANGE;

/********************************************************************
 *                         PRIVATE STUFF                            *
 ********************************************************************
 */

typedef struct {
  gpointer key;
  gpointer value;
  gint     type;
  GSList  *watches;
} GGaduRepoValue;

typedef struct {
  gchar  *name;
  GSList *values;
  GSList *watches;
} GGaduRepo;

typedef struct {
  gint      actions;
  gint      types;
  watch_ptr callback;
} GGaduRepoWatch;

static GGaduRepo repos;

GGaduRepo *ggadu_repo_find (gchar *repo_name)
{
  GGaduRepo      *tmp;
  GSList         *list;

  list = repos.values;
  while (list)
  {
    tmp = (GGaduRepo *) list->data;

    if (!ggadu_strcasecmp (repo_name, (gchar *)tmp->name))
      return tmp;
    
    list = list->next;
  }

  return NULL;
}

gpointer ggadu_repo_ptr_value (gchar *repo_name, gpointer key)
{
  GGaduRepo      *tmp;
  GGaduRepoValue *tmpv;
  GSList         *list;

  tmp = ggadu_repo_find (repo_name);
  
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


void ggadu_repo_watch_notify (gchar *repo_name, gpointer key, gint actions, gint types)
{
  GGaduRepo      *tmp;
  GGaduRepoValue *tmpv;
  GSList         *list;
  GGaduRepoWatch *watch;
  GSList         *completed = NULL;

  /* Wysy�amy ��dania w kolejno�ci od najbardziej szczeg�owych do
   * najbardziej og�lnych. Na pierwszy rzut id� patrza�ki na poszczeg�lne
   * warto�ci.
   */
  
  /* najpierw przelatujemy przez wszystkie warto�ci */
  if ((actions & REPO_value_mask) && ggadu_repo_check_value (repo_name, key))
  {
    tmpv = ggadu_repo_ptr_value (repo_name, key);
    
    list = tmpv->watches;
    while (list)
    {
      watch = (GGaduRepoWatch *)list->data;
      if ((watch->actions & actions & REPO_value_mask) && (watch->types & types))
      {
       	print_debug ("watch->actions: %d\n", watch->actions);
	print_debug ("       actions: %d\n", actions);
	print_debug ("          mask: %d\n", REPO_mask);
	print_debug ("    value mask: %d\n", REPO_value_mask);
	watch->callback (repo_name, key, watch->actions & actions & REPO_value_mask);
	if (!g_slist_find (completed, watch->callback))
	  completed = g_slist_append (completed, watch->callback);
      }
      list = list->next;
    }
  }

  /* teraz patrza�ki na konkretne repo */
  if ((actions & REPO_mask & (~REPO_ACTION_NEW)) && (tmp = ggadu_repo_find (repo_name)))
  {
    list = tmp->watches;
    while (list)
    {
      watch = (GGaduRepoWatch *) list->data;
      if ((watch->types & types) && !g_slist_find (completed, watch->callback))
      {
       	print_debug ("WATCH->actions: %d\n", watch->actions);
	print_debug ("       actions: %d\n", actions);
	print_debug ("          mask: %d\n", REPO_mask);
	print_debug ("    value mask: %d\n", REPO_value_mask);
	watch->callback (repo_name, key, watch->actions & actions & REPO_mask);
	completed = g_slist_append (completed, watch->callback);
      }
      list = list->next;
    }
  }
  
  /* i w ko�cu kierujemy patrza�ki na najbardziej og�lne - czyli na list� rep�w */
  if (actions & REPO_mask)
  {
    list = repos.watches;
    while (list)
    {
      watch = (GGaduRepoWatch *) list->data;
      if ((watch->types & types) && !g_slist_find (completed, watch->callback))
      {
       	print_debug ("WaTcH->actions: %d\n", watch->actions);
	print_debug ("       actions: %d\n", actions);
	print_debug ("          mask: %d\n", REPO_mask);
	print_debug ("    value mask: %d\n", REPO_value_mask);
	watch->callback (repo_name, key, watch->actions & actions & REPO_mask);
      }
      list = list->next;
    }
  }

  return;
}

/********************************************************************
 *                         PUBLIC  STUFF                            *
 ********************************************************************
 */

gboolean ggadu_repo_exists (gchar *repo_name)
{
  return ggadu_repo_find (repo_name) ? TRUE:FALSE;
}

gboolean ggadu_repo_check_value (gchar *repo_name, gpointer key)
{
  GGaduRepo      *tmp;
  GGaduRepoValue *tmpv;
  GSList         *list;

  tmp = ggadu_repo_find (repo_name);
  
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

gpointer ggadu_repo_find_value (gchar *repo_name, gpointer key)
{
  GGaduRepo      *tmp;
  GGaduRepoValue *tmpv;
  GSList         *list;

  tmp = ggadu_repo_find (repo_name);
  
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

gboolean ggadu_repo_add (gchar *repo_name)
{
  GGaduRepo *tmp;

  if (ggadu_repo_find (repo_name))
  {
    print_debug ("repo: repo %s exists\n", repo_name);
    return FALSE;
  }
  
  tmp          = g_new0 (GGaduRepo, 1);
  tmp->name    = g_strdup (repo_name);
  tmp->values  = NULL;
  tmp->watches = NULL;

  repos.values = g_slist_append (repos.values, tmp);

  ggadu_repo_watch_notify (repo_name, NULL, REPO_ACTION_NEW, REPO_VALUE_ANY);
  return TRUE;
}

gboolean ggadu_repo_add_value (gchar *repo_name, gpointer key, gpointer value, gint type)
{
  GGaduRepoValue *tmpv;
  GGaduRepo      *tmp;

  if (ggadu_repo_check_value (repo_name, key))
  {
    print_debug ("wartosc jest\n");
    return FALSE;
  }
  
  tmp = ggadu_repo_find (repo_name);
  
  tmpv          = g_new0 (GGaduRepoValue, 1);
  tmpv->key     = key;
  tmpv->value   = value;
  if (type == REPO_VALUE_DC)
    tmpv->type  = REPO_VALUE_OTHER;
  else
    tmpv->type    = type;
  tmpv->watches = NULL;

  tmp->values = g_slist_append (tmp->values, tmpv);

  ggadu_repo_watch_notify (repo_name, key, REPO_ACTION_CHANGE | REPO_ACTION_VALUE_NEW, type);
  return TRUE;
}

gboolean ggadu_repo_change_value (gchar *repo_name, gpointer key, gpointer value, gint type)
{
  GGaduRepo      *tmp;
  GGaduRepoValue *tmpv;

  tmp = ggadu_repo_find (repo_name);
  if (!tmp)
    return FALSE;

  tmpv = ggadu_repo_ptr_value (repo_name, key);
  if (!tmpv)
  {
    print_debug ("wartosci nie ma\n");
    return FALSE;
  }
  
  tmpv->value = value;

  if (type != REPO_VALUE_DC)
    tmpv->type  = type;
  
  ggadu_repo_watch_notify (repo_name, key, REPO_ACTION_CHANGE | REPO_ACTION_VALUE_CHANGE, type);
  return TRUE;
}

gboolean ggadu_repo_del_value (gchar *repo_name, gpointer key)
{
  GGaduRepo      *tmp;
  GGaduRepoValue *tmpv;

  tmp = ggadu_repo_find (repo_name);
  if (!tmp)
    return FALSE;

  tmpv = ggadu_repo_ptr_value (repo_name, key);
  if (!tmpv)
    return FALSE;
  
  ggadu_repo_watch_notify (repo_name, key, REPO_ACTION_CHANGE | REPO_ACTION_VALUE_DEL, tmpv->type);

  tmp->values = g_slist_remove (tmp->values, tmpv);
  g_slist_free (tmp->watches);
  g_free (tmpv);
  
  return TRUE;
}

gboolean ggadu_repo_del (gchar *repo_name)
{
  GGaduRepo      *tmp;
  GGaduRepoValue *tmpv;
  GSList         *list;

  tmp = ggadu_repo_find (repo_name);
  if (!tmp)
    return FALSE;

  ggadu_repo_watch_notify (repo_name, NULL, REPO_ACTION_DEL | REPO_ACTION_VALUE_DEL, REPO_VALUE_ANY);

  list = tmp->values;
  while (list)
  {
    tmpv = (GGaduRepoValue *) list->data;
    g_slist_free (tmp->watches);
    g_free (tmpv);
    list = list->next;
  }
  g_slist_free (tmp->values);

  repos.values = g_slist_remove (repos.values, tmp);
 
  g_slist_free (tmp->watches);
  g_free (tmp->name);
  g_free (tmp);

  return TRUE;
}

gboolean ggadu_repo_watch_add (gchar *repo_name, gint actions, gint types, watch_ptr callback)
{
  GGaduRepo      *tmp;
  GSList         *list;
  GGaduRepoWatch *watch;

  if (!repo_name)
    tmp = &repos;
  else
  {
    tmp = ggadu_repo_find (repo_name);
    if (!tmp)
      return FALSE;
  }

  list = tmp->watches;
  while (list)
  {
    watch = (GGaduRepoWatch *)list->data;
    if (watch->callback == callback) /* mamy skuba�ca */
    {
      watch->actions = watch->actions | actions;
      watch->types   = watch->types   | types;
      print_debug ("repo: %s on '%s' is now %d for callback %p\n",
	  repo_name ? "Watch":"Core watch",
	  repo_name ? repo_name:"(everything)",
	  watch->actions, watch->callback);
      return TRUE;
    }
    list = list->next;
  }
  
  /* ojej, musimy ca�kiem od nowa doda� watcha */
  watch = g_new0 (GGaduRepoWatch, 1);
  watch->callback = callback;
  watch->actions  = actions;
  watch->types    = types;
  print_debug ("repo: %s on '%s' is now %d for callback %p\n",
      repo_name ? "Watch":"Core watch",
      repo_name ? repo_name:"(everything)",
      watch->actions, watch->callback);
  tmp->watches = g_slist_append (tmp->watches, watch);
  return TRUE;
}

gboolean ggadu_repo_watch_del (gchar *repo_name, gint actions, gint types, watch_ptr callback)
{
  GGaduRepo      *tmp;
  GSList         *list;
  GGaduRepoWatch *watch;

  if (!repo_name)
    tmp = &repos;
  else
  {
    tmp = ggadu_repo_find (repo_name);
    if (!tmp)
      return FALSE;
  }
  
  list = tmp->watches;
  while (list)
  {
    watch = (GGaduRepoWatch *)list->data;
    if (watch->callback == callback) /* mamy skuba�ca */
    {
      watch->actions = watch->actions & ~actions;
      watch->types   = watch->types   & ~types;
      print_debug ("repo: %s on '%s' is now %d for callback %p\n",
	  repo_name ? "Watch":"Core watch",
	  repo_name ? repo_name:"(everything)",
	  watch->actions, watch->callback);
      if (watch->actions == 0)
      {
	tmp->watches = g_slist_remove (tmp->watches, watch);
	g_free (watch);
      }
      return TRUE;
    }
    list = list->next;
  }

  return FALSE;
}

gboolean ggadu_repo_watch_value_add (gchar *repo_name, gpointer key, gint actions, watch_ptr callback)
{
  GGaduRepoValue *tmpv;
  GSList         *list;
  GGaduRepoWatch *watch;

  if (repo_name && (actions & REPO_value_mask))
  {
    if (!ggadu_repo_check_value (repo_name, key))
      return FALSE;

    tmpv = ggadu_repo_ptr_value (repo_name, key);

    list = tmpv->watches;
    while (list)
    {
      watch = (GGaduRepoWatch *)list->data;
      if (watch->callback == callback) /* mamy skuba�ca */
      {
	watch->actions = watch->actions | (actions & REPO_value_mask);
	print_debug ("repo: Watch on %p in '%s' is now %d for callback %p\n",
	    key, repo_name, watch->actions, watch->callback);
	return TRUE;
      }
      list = list->next;
    }

    /* ojej, musimy ca�kiem od nowa doda� watcha */
    watch = g_new0 (GGaduRepoWatch, 1);
    watch->callback = callback;
    watch->actions  = actions & REPO_value_mask;
    print_debug ("repo: Watch on %p in '%s' is now %d for callback %p\n",
	key, repo_name, watch->actions, watch->callback);
    tmpv->watches = g_slist_append (tmpv->watches, watch);
    return TRUE;
  }
  
  return FALSE;
}

gboolean ggadu_repo_watch_value_del (gchar *repo_name, gpointer key, gint actions, watch_ptr callback)
{
  GGaduRepo      *tmp;
  GGaduRepoValue *tmpv;
  GSList         *list;
  GGaduRepoWatch *watch;

  if (repo_name && (actions & REPO_value_mask))
  {
    tmpv = ggadu_repo_find_value (repo_name, key);
    if (!tmpv)
      return FALSE;

    list = tmp->watches;
    while (list)
    {
      watch = (GGaduRepoWatch *)list->data;
      if (watch->callback == callback) /* mamy skuba�ca */
      {
	watch->actions = watch->actions | ~(actions & REPO_value_mask);
	print_debug ("repo: Watch on %p in '%s' is now %d for callback %p\n",
	    key, repo_name, watch->actions, watch->callback);
	if (watch->actions == 0)
	{
	  tmp->watches = g_slist_remove (tmp->watches, watch);
	  g_free (watch);
	}
	return TRUE;
      }
      list = list->next;
    }

    return TRUE;
  }

  return FALSE;
}

void drop_callback (GSList **watches, watch_ptr callback)
{
  GSList         *list;
  GGaduRepoWatch *watch;
  
  list = *watches;
  while (list)
  {
    watch = (GGaduRepoWatch *)list->data;
    if (watch->callback == callback)
    {
      *watches = g_slist_remove (*watches, watch);
      g_free(watch);
      return;
    }
    list = list->next;
  }
}

gboolean ggadu_repo_watch_clear_callback (watch_ptr callback)
{
  GGaduRepo      *tmp;
  GGaduRepo      *tmpr;
  GGaduRepoValue *tmpv;
  GSList         *list;
  GSList         *listv;

  tmp = &repos;
  
  /* najpierw id� patrza�ki na repos */
  drop_callback (&tmp->watches, callback);

  /* teraz przelatujemy przez ka�de repo */
  list = tmp->values;
  while (list)
  {
    tmpr = (GGaduRepo *)list->data;
    drop_callback (&tmpr->watches, callback);

    listv = tmpr->values;
    while (listv)
    {
      tmpv = (GGaduRepoValue *)list->data;
      drop_callback (&tmpv->watches, callback);
      listv = listv->next;
    }
    
    list = list->next;
  }

  return FALSE;
}

