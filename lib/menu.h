/* $Id: menu.h,v 1.4 2004/01/28 23:39:26 shaster Exp $ */

/* 
 * GNU Gadu 2 
 * 
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

#ifndef GGadu_MENU_H
#define GGadu_MENU_H 1

#include "gg-types.h"

void ggadu_menu_free(GGaduMenu * menu);

/*
 * tworzy nowe menu
 */
GGaduMenu *ggadu_menu_create();

/*
 * tworzy nowy item ktory pozniej pozna podlaczac
 */
GGaduMenu *ggadu_menu_new_item(gchar * label, function_ptr callback, gpointer data);

/* 
 * dodaje element (item) do menu, zwraca dodany item
 * zwraca GGaduMenu reprezentujace dodany item
 */
GGaduMenu *ggadu_menu_add_item(GGaduMenu * menu, gchar * label, function_ptr callback, gpointer user_data);


/*
 * dodaje submenu do menu a konkretnie do item'u
 */
void ggadu_menu_add_submenu(GGaduMenu * to_item, GGaduMenu * menu);

/*
 * zwraca submenu dla podanego menu
 */
GGaduMenu *ggadu_menu_get_submenu(GGaduMenu * menu);


/*
 * zwraca liste zawierajaca GGaduMenuItem dla jednego poziomu podanego menu
 */
GSList *ggadu_menu_get_items(GGaduMenu * menu);

/*
 * zwraca menu nadrzedne
 */
GGaduMenu *ggadu_menu_get_parent(GGaduMenu * children);

/*
 * wypisuje strukture zbudowanego drzewa, raczej przydatne tylko do debugu
 */
void ggadu_menu_print(GGaduMenu * node, gchar * p);

#endif
