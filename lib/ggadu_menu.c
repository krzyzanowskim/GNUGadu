/* $Id: ggadu_menu.c,v 1.2 2004/05/04 21:39:08 krzyzak Exp $ */

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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "ggadu_support.h"
#include "ggadu_types.h"


void ggadu_menu_free(GGaduMenu * menu)
{
    g_node_destroy((GNode *) menu);
}

/*
 * tworzy nowe menu
 */
GGaduMenu *ggadu_menu_create()
{
    return g_node_new(NULL);
}

/* 
 * dodaje element (item) do menu, zwraca dodany item
 * zwraca GGaduMenu reprezentujace dodany item
 */
GGaduMenu *ggadu_menu_add_item(GGaduMenu * menu, gchar * label, function_ptr callback, gpointer user_data)
{
    GGaduMenuItem *ggadumenuitem = NULL;
    GGaduMenu *gmenu = NULL;

    ggadumenuitem = g_new0(GGaduMenuItem, 1);

    ggadumenuitem->label = g_strdup(label);	/* ZONK */
    ggadumenuitem->data = user_data;
    ggadumenuitem->callback = callback;

    gmenu = g_node_new(ggadumenuitem);

    if (menu != NULL)
	gmenu = g_node_append(menu, gmenu);

    return gmenu;
}

/*
 * tworzy nowy item ktory pozniej pozna podlaczac
 */
GGaduMenu *ggadu_menu_new_item(gchar * label, function_ptr callback, gpointer data)
{
    return ggadu_menu_add_item(NULL, label, callback, data);
}

/*
 * dodaje submenu do menu a konkretnie do item'u
 */
void ggadu_menu_add_submenu(GGaduMenu * to_item, GGaduMenu * menu)
{
    g_node_append(to_item, menu);
}

/*
 * zwraca submenu dla podanego menu
 */
GGaduMenu *ggadu_menu_get_submenu(GGaduMenu * menu)
{

    if (menu == NULL)
    {
	g_warning(_("!!! ggadu_menu_get_sumbenu : menu == NULL !!!\n"));
	return NULL;
    }

    return (GGaduMenu *) menu->children;
}


/*
 * zwraca liste GGaduMenuItem z podanego menu na jednym poziomie
 */
GSList *ggadu_menu_get_items(GGaduMenu * menu)
{
    GSList *list = NULL;
    GGaduMenu *node = g_node_first_sibling(menu);

    while (node)
    {
	GGaduMenuItem *g = node->data;
	list = g_slist_append(list, g);
	node = node->next;
    }

    return list;
}

/*
 * zwraca menu nadrzedne
 */
GGaduMenu *ggadu_menu_get_parent(GGaduMenu * children)
{

    if (children == NULL)
    {
	g_warning(_("!!! ggadu_menu_get_parent : children == NULL !!!\n"));
	return NULL;
    }

    return (GGaduMenu *) children->parent;
}


/*
 * wypisuje strukture zbudowanego drzewa, raczej przydatne tylko do debugu
 */
void ggadu_menu_print(GGaduMenu * node, gchar * p)
{
    GGaduMenu *child = NULL;
    static int dep = 0;

    if (!p)
	p = g_strdup_printf("->");

    if (G_NODE_IS_ROOT(node))
	node = g_node_first_child(node);
    else
	node = g_node_first_sibling(node);

    while (node)
    {
	GGaduMenuItem *item = node->data;

	print_debug("%d %s %s\n", dep, p, item->label);

	if ((child = g_node_first_child(node)) != NULL)
	{
	    char *s = p;
	    p = g_strdup_printf("->%s", s);
	    g_free(s);
	    dep++;

	    ggadu_menu_print(child, p);	/* rekurencja */

	    dep--;
	    s = p;
	    p = g_strndup(s, strlen(p) - 2);
	    g_free(s);

	}

	node = g_node_next_sibling(node);
    }
}

/*
void test() {
    GGaduMenu *root_menu;
    GGaduMenu *item1,*item2,*item3,*item4,*item5,*item6,*item7,*item8;

    root_menu = ggadu_menu_create();
    item1 = ggadu_menu_new_item("jeden",NULL,NULL);
    item2 = ggadu_menu_new_item("dwa",NULL,NULL);
    item3 = ggadu_menu_new_item("trzy",NULL,NULL);
    item4 = ggadu_menu_new_item("cztery",NULL,NULL);
    item5 = ggadu_menu_new_item("piec",NULL,NULL);
    item6 = ggadu_menu_new_item("szesc",NULL,NULL);
    item7 = ggadu_menu_new_item("siedem",NULL,NULL);
    item8 = ggadu_menu_new_item("osiem",NULL,NULL);
    
    ggadu_menu_add_submenu(root_menu,item1);
    ggadu_menu_add_submenu(root_menu,item2);
    ggadu_menu_add_submenu(root_menu,item3);
    
    ggadu_menu_add_submenu(item3,item4);
    ggadu_menu_add_submenu(item3,item5);
    ggadu_menu_add_submenu(item3,item6);

    ggadu_menu_add_submenu(item6,item7);
    ggadu_menu_add_submenu(item5,item8);
    
    ggadu_menu_print(root_menu);
}
*/
