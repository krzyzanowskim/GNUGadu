/* $Id: menu.h,v 1.2 2003/06/09 18:24:36 shaster Exp $ */

#ifndef GGadu_MENU_H
#define GGadu_MENU_H 1

#include "gg-types.h"

void ggadu_menu_free(GGaduMenu *menu);

/*
 * tworzy nowe menu
 */
GGaduMenu *ggadu_menu_create();

/*
 * tworzy nowy item ktory pozniej pozna podlaczac
 */
GGaduMenu *ggadu_menu_new_item(gchar *label, function_ptr callback, gpointer data);

/* 
 * dodaje element (item) do menu, zwraca dodany item
 * zwraca GGaduMenu reprezentujace dodany item
 */
GGaduMenu *ggadu_menu_add_item(GGaduMenu *menu,gchar *label, function_ptr callback, gpointer user_data);


/*
 * dodaje submenu do menu a konkretnie do item'u
 */
void ggadu_menu_add_submenu(GGaduMenu *to_item, GGaduMenu *menu);

/*
 * zwraca submenu dla podanego menu
 */
GGaduMenu *ggadu_menu_get_submenu(GGaduMenu *menu);


/*
 * zwraca liste zawierajaca GGaduMenuItem dla jednego poziomu podanego menu
 */
GSList *ggadu_menu_get_items(GGaduMenu *menu);

/*
 * zwraca menu nadrzedne
 */
GGaduMenu *ggadu_menu_get_parent(GGaduMenu *children);

/*
 * wypisuje strukture zbudowanego drzewa, raczej przydatne tylko do debugu
 */
void ggadu_menu_print(GGaduMenu *node, gchar *p);

#endif

