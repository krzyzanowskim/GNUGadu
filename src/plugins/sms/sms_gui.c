/*
** Sms gui plugin for GNU Gadu 2
** Bartlomiej Pawlak Copyright (c) 2003
**
**
** This code is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License.
**
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>


#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "menu.h"
#include "support.h"
#include "dialog.h"
#include "sms_gui.h"
#include "sms_core.h"

GGaduPlugin *sms_handler;
GSList *smslist = NULL;
gchar *this_configdir = NULL;
gchar *nick = NULL;
gchar *mobile = NULL;

GGaduProtocol *p;
GGaduMenu *menu_smsmenu;

GGadu_PLUGIN_INIT("sms", GGADU_PLUGIN_TYPE_PROTOCOL);


/* okienko z ustawieniami */
gpointer sms_preferences (gpointer user_data)
{
    GGaduDialog *d;

    print_debug("%s : Preferences\n",GGadu_PLUGIN_NAME);

    d = ggadu_dialog_new();
    ggadu_dialog_set_title(d, _("SMS Preferences"));
    ggadu_dialog_callback_signal(d, "update config");
    ggadu_dialog_set_type(d, GGADU_DIALOG_CONFIG);
    
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SMS_CONFIG_EXTERNAL, _("Use \"SMS\" program to send"), VAR_BOOL, (gpointer)config_var_get(sms_handler, "external"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SMS_CONFIG_SENDER, _("Sender"), VAR_STR, (gpointer)config_var_get(sms_handler, "sender"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SMS_CONFIG_SHOW_IN_STATUS, _("Show plugin in status list (needs reload)"), VAR_BOOL, (gpointer)config_var_get(sms_handler, "show_in_status"), VAR_FLAG_NONE);
    
    signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");

    return NULL;
}

/* edycja usera */
gpointer sms_edit_contact (gpointer user_data)
{
    GSList *optlist = NULL;
    GSList *users = (GSList *)user_data;
    GGaduContact *k = (GGaduContact *)users->data;

    print_debug("%s : Add Contact\n", GGadu_PLUGIN_NAME);
    
    ggadu_dialog_add_entry(&optlist, GGADU_SMS_CONTACT_ID, _("Id"), VAR_STR, g_strconcat(k->nick, "@", k->mobile, NULL), VAR_FLAG_INSENSITIVE);
    ggadu_dialog_add_entry(&optlist, GGADU_SMS_CONTACT_NICK, _("Nick"), VAR_STR, k->nick, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&optlist, GGADU_SMS_CONTACT_NUMBER, _("Number"), VAR_STR, k->mobile, VAR_FLAG_NONE);
    
    signal_emit(GGadu_PLUGIN_NAME, "gui change user window", optlist, "main-gui");

    return NULL;
}

/* okienko do dodawania usera */
gpointer sms_add_contact (gpointer user_data)
{
    GSList *optlist = NULL;

    print_debug("%s : Add Contact\n", GGadu_PLUGIN_NAME);
    
    ggadu_dialog_add_entry(&optlist, GGADU_SMS_CONTACT_NICK, _("Nick"), VAR_STR, NULL, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&optlist, GGADU_SMS_CONTACT_NUMBER, _("Number"), VAR_STR, NULL, VAR_FLAG_NONE);
    
    signal_emit(GGadu_PLUGIN_NAME, "gui add user window", optlist, "main-gui");

    return NULL;
}

/* wywalenie usera */
gpointer sms_remove_contact (gpointer user_data)
{
    GSList *users = (GSList *)user_data;

    while (users)
    {
	GGaduContact *k = (GGaduContact *)users->data;
	smslist = g_slist_remove(smslist, k);
	users = users->next;
    }
    signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", smslist, "main-gui");
    save_smslist();

    return NULL;
}

/* wyslanie smsa po kliknieciu prawego przycisku */
gpointer sms_send_sms (gpointer user_data)
{
    GSList *users = (GSList *)user_data;
    GGaduContact *k = (GGaduContact *)users->data;
    GGaduDialog *d;

    d = ggadu_dialog_new();
    ggadu_dialog_set_title(d, g_strconcat(_("Send to : "), k->nick, " (", k->mobile, ")", NULL));
    ggadu_dialog_callback_signal(d, "sms send");
    config_var_set(sms_handler, "number", k->mobile);

    ggadu_dialog_add_entry(&(d->optlist), GGADU_SMS_CONFIG_EXTERNAL, _("Use external program"), VAR_BOOL, (gpointer)config_var_get(sms_handler, "external"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SMS_CONFIG_SENDER, _("Sender"), VAR_STR, (gpointer)config_var_get(sms_handler, "sender"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), GGADU_SMS_CONFIG_BODY, _("Message"), VAR_STR, (gpointer)config_var_get(sms_handler, "body"), VAR_FLAG_FOCUS);    
    signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");

    return NULL;
}

/* tworzenie menu pod prawym klawiszem */
void sms_userlist_menu()
{
    GGaduMenu *umenu = ggadu_menu_create();
    GGaduMenu *listmenu = NULL;
    
    ggadu_menu_add_submenu(umenu, ggadu_menu_new_item(_("Send SMS"), sms_send_sms, NULL) );

    listmenu = ggadu_menu_new_item(_("Contact"), NULL, NULL);
    ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Add"), sms_add_contact, NULL) );
    ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Remove"), sms_remove_contact, NULL) );
    ggadu_menu_add_submenu(listmenu, ggadu_menu_new_item(_("Edit"), sms_edit_contact, NULL) );
    ggadu_menu_add_submenu(umenu, listmenu);

    ggadu_menu_print(umenu, NULL);    

    signal_emit(GGadu_PLUGIN_NAME, "gui register userlist menu", umenu, "main-gui");
}

/* tworzenie menu w "MENU" */
GGaduMenu *sms_menu()
{
    GGaduMenu *root = ggadu_menu_create();
    GGaduMenu *item = ggadu_menu_add_item(root, "SMS", NULL, NULL);

    ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Add Contact"), sms_add_contact, NULL));
    ggadu_menu_add_submenu(item, ggadu_menu_new_item("", NULL, NULL));
    ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Preferences"), sms_preferences, NULL));

    return root;
}

/* wiadomo */
GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
    print_debug("%s : initialize\n",GGadu_PLUGIN_NAME);

    GGadu_PLUGIN_ACTIVATE(conf_ptr);
    sms_handler = (GGaduPlugin *)register_plugin(GGadu_PLUGIN_NAME,_("SMS sender"));
    
    print_debug("%s : read configuration\n",GGadu_PLUGIN_NAME);
    if (g_getenv("CONFIG_DIR"))
	this_configdir = g_build_filename(g_get_home_dir(), g_getenv("CONFIG_DIR"), "gg2", NULL);
    else
	this_configdir = g_build_filename(g_get_home_dir(), ".gg2", NULL);
	
    set_config_file_name((GGaduPlugin *)sms_handler, g_build_filename(this_configdir, "sms", NULL));
    config_var_add(sms_handler, "sender", VAR_STR);
    config_var_add(sms_handler,"number", VAR_STR);
    config_var_add(sms_handler,"body", VAR_STR);
    config_var_add(sms_handler,"external", VAR_BOOL);
    config_var_add(sms_handler, "show_in_status", VAR_BOOL);

    if (!config_read(sms_handler))
	g_warning(_("Unable to read config file for plugin sms"));
	
    register_signal_receiver((GGaduPlugin *)sms_handler, (signal_func_ptr)signal_receive);
    
    return sms_handler;
}

/* odczyt sygnalow i podjecie odpowiednich czynnosci zwiazanych z tym */
void signal_receive(gpointer name, gpointer signal_ptr)
{
    GGaduSignal *signal = (GGaduSignal *)signal_ptr;
    
    print_debug("%s : received signal %d\n", GGadu_PLUGIN_NAME, signal->name);

    if (signal->name == g_quark_from_static_string("update config"))
    {
        GGaduDialog *d = signal->data;
	GSList *tmplist = d->optlist;
	
	if (d->response == GGADU_OK)
	{
	    while (tmplist)
	    {
		GGaduKeyValue *kv = (GGaduKeyValue *)tmplist->data;
	        switch (kv->key)
		{
		    case GGADU_SMS_CONFIG_SENDER:
		        print_debug("change default sender to %s\n",kv->value);
		        config_var_set(sms_handler, "sender", kv->value);
		        break;
		    case GGADU_SMS_CONFIG_EXTERNAL:
		        print_debug("change external program to %d\n",kv->value);
		        config_var_set(sms_handler, "external", kv->value);
		        break;
		    case GGADU_SMS_CONFIG_SHOW_IN_STATUS:
			print_debug("change show_in_status to %d\n", kv->value);
			config_var_set (sms_handler, "show_in_status", kv->value);
			break;
		}
		tmplist = tmplist->next;
	    }
	    config_save(sms_handler);
	}
        GGaduDialog_free(d);
	
	return;
    }

    if (signal->name == g_quark_from_static_string("change status"))
    {
	GGaduStatusPrototype *sp = signal->data;
	
	if (sp->status == 1 || sp->status == 2)
	{
	    GGaduDialog *d;

	    if (sp->status == 1)
		config_var_set(sms_handler, "external", (gpointer)FALSE);
	    else 
		config_var_set(sms_handler, "external", (gpointer)TRUE);
        
	    print_debug("%s : Send sms\n",GGadu_PLUGIN_NAME);

	    d = ggadu_dialog_new();
	    ggadu_dialog_set_title(d, _("SMS Send"));
	    ggadu_dialog_callback_signal(d, "sms send");

	    ggadu_dialog_add_entry(&(d->optlist), GGADU_SMS_CONFIG_SENDER, _("Sender"), VAR_STR, (gpointer)config_var_get(sms_handler, "sender"), VAR_FLAG_NONE);
	    ggadu_dialog_add_entry(&(d->optlist), GGADU_SMS_CONFIG_NUMBER, _("Number"), VAR_STR, (gpointer)config_var_get(sms_handler, "number"), VAR_FLAG_NONE);
	    ggadu_dialog_add_entry(&(d->optlist), GGADU_SMS_CONFIG_BODY, _("Message"), VAR_STR, (gpointer)config_var_get(sms_handler, "body"), VAR_FLAG_FOCUS);
    
	    signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");
	}
    }

    if  (signal->name == g_quark_from_static_string("send message"))
    {
	GGaduMsg *msg = signal->data;
	
	if (msg)
	{
	    method = GGADU_SMS_METHOD_CHAT;
	    if (config_var_get(sms_handler, "sender") != NULL)
		send_sms((gboolean)config_var_get(sms_handler, "external"),
			  config_var_get(sms_handler, "sender"),
			  msg->id,
			  msg->message);
	    else 
		sms_preferences(0);    
	}
    }

    if (signal->name == g_quark_from_static_string("sms send"))
    {
        GGaduDialog *d = signal->data;
	GSList *tmplist = d->optlist;
	gboolean old_external = (gboolean)config_var_get(sms_handler, "external");
	gchar *sender_temp;
	
	if (d->response == GGADU_OK)
	{
	    while (tmplist)
	    {
		GGaduKeyValue *kv = (GGaduKeyValue *)tmplist->data;
		switch (kv->key)
	        {
	    	    case GGADU_SMS_CONFIG_SENDER:
	    	        sender_temp = g_strdup(kv->value);
			break;
		    case GGADU_SMS_CONFIG_EXTERNAL:
		        config_var_set(sms_handler, "external", kv->value);
		        break;
		    case GGADU_SMS_CONFIG_NUMBER:
		        config_var_set(sms_handler, "number", kv->value);
		        break;
		    case GGADU_SMS_CONFIG_BODY:
		        config_var_set(sms_handler, "body", kv->value);
		        break;
		}
		tmplist = tmplist->next;
	    }
	    method = GGADU_SMS_METHOD_POPUP;
	    send_sms((gboolean)config_var_get(sms_handler, "external"),
		 sender_temp,
		 config_var_get(sms_handler, "number"),
		 config_var_get(sms_handler, "body"));

//	    g_free(sender_temp);
	    config_var_set(sms_handler, "external", (gpointer)old_external);
	    config_save(sms_handler);
	}

        GGaduDialog_free(d);
	return;
    }

    if (signal->name == g_quark_from_static_string("add user"))
    {
	GSList *tmplist = (GSList *)signal->data;
	GGaduContact *k = g_new0(GGaduContact,1);
	
	while (tmplist)
	{
	    GGaduKeyValue *kv = (GGaduKeyValue *)tmplist->data;
	    switch (kv->key)
	    {
		case GGADU_SMS_CONTACT_NICK:
		    k->nick = g_strdup(kv->value);
		    break;
		case GGADU_SMS_CONTACT_NUMBER:
		    k->mobile = g_strdup(kv->value);
		    k->id = k->mobile;
		    k->status = 1;
		    break;
	    }
	    tmplist=tmplist->next;
	}
	g_slist_free(tmplist);

	smslist = g_slist_append(smslist,k);
	signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", smslist, "main-gui");

	save_smslist();    

	return;
    }

    if (signal->name == g_quark_from_static_string("change user"))
    {
	GSList *tmplist = (GSList *)signal->data;
	GSList *uslist = smslist;
	GGaduContact *k = g_new0(GGaduContact,1);
	
	while (tmplist)
	{
	    GGaduKeyValue *kv = (GGaduKeyValue *)tmplist->data;
	    switch (kv->key)
	    {
		case GGADU_SMS_CONTACT_ID:
		    k->id = g_strdup(kv->value);
		    break;
		case GGADU_SMS_CONTACT_NICK:
		    k->nick = g_strdup(kv->value);
		    break;
		case GGADU_SMS_CONTACT_NUMBER:
		    k->mobile = g_strdup(kv->value);
		    k->status = 1;
		    break;
	    }
	    tmplist=tmplist->next;
	}
	g_slist_free(tmplist);
	
	while (uslist)
	{
	    GGaduContact *kvtmp = (GGaduContact *)uslist->data;
	    
	    if (!ggadu_strcasecmp(g_strconcat(kvtmp->nick, "@", kvtmp->mobile, NULL), k->id))
	    {
		kvtmp->mobile = k->mobile;
		kvtmp->id = k->mobile;
		kvtmp->nick = k->nick;
		break;
	    }
	    uslist = uslist->next;
	}
	//smslist = g_slist_append(smslist,k);
        signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", smslist, "main-gui");
        save_smslist();    

	return;
    }

    if (signal->name == g_quark_from_static_string("get token"))
    {
        GGaduDialog *d = signal->data;
	GSList *tmplist = d->optlist;
	gchar *temp = NULL;

	if (d->response == GGADU_OK)
	{
	    while (tmplist)
	    {
		GGaduKeyValue *kv = (GGaduKeyValue *)tmplist->data;
		if (kv->key == 1)
		    temp = kv->value;
		tmplist = tmplist->next;
	    }
	    send_IDEA_stage2(temp, d->user_data);
	}
        GGaduDialog_free(d);
    }
}

/* guziczki na dole, podpiete pod zmiane statusu */
GSList *button_send()
{
    GSList *list = NULL;
    GGaduStatusPrototype *sp;
    
    sp = g_new0(GGaduStatusPrototype, 3);
    
    sp->status = 1;
    sp->description = g_strdup(_("Send SMS (internal)"));
    sp->image = g_strdup("sms_i.png");
    list = g_slist_append(list,sp);
    sp++;

    sp->status = 2;
    sp->description = g_strdup(_("Send SMS (external)"));
    sp->image = g_strdup("sms_i.png");
    list = g_slist_append(list,sp);
    sp++;

    return list;
}

/* gdzies to musi sie zaczac */
void start_plugin()
{
    p = g_new0(GGaduProtocol,1);
    p->display_name = g_strdup("SMS");
    p->img_filename = g_strdup("sms.png");
    
    p->statuslist = button_send();
    p->offline_status = config_var_get (sms_handler, "show_in_status") ? 2:3;

    print_debug("%s : start_plugin\n",GGadu_PLUGIN_NAME);
    register_signal(sms_handler, "update config");
    register_signal(sms_handler, "change status");
    register_signal(sms_handler, "sms send");
    register_signal(sms_handler, "add user");
    register_signal(sms_handler, "change user");
    register_signal(sms_handler, "send message");
    register_signal(sms_handler, "get token");

    signal_emit(GGadu_PLUGIN_NAME, "gui register protocol", p, "main-gui");

    print_debug("%s : create menu\n",GGadu_PLUGIN_NAME);
    menu_smsmenu = sms_menu ();
    signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_smsmenu, "main-gui");

    load_smslist();

    signal_emit(GGadu_PLUGIN_NAME, "gui send userlist", smslist, "main-gui");

    sms_userlist_menu();
}

/* i skonczyc :-p */
void destroy_plugin()
{
    print_debug("destroy_plugin%s\n",GGadu_PLUGIN_NAME);
    if (menu_smsmenu)
    {
      signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", menu_smsmenu, "main-gui");
      ggadu_menu_free (menu_smsmenu);
    }
    signal_emit(GGadu_PLUGIN_NAME, "gui unregister userlist menu", NULL, "main-gui");
    signal_emit(GGadu_PLUGIN_NAME, "gui unregister protocol", p, "main-gui"); 
}

/* zapis listy numerow do pliku */
void save_smslist()
{
    GSList *smu = smslist;
    FILE *fp;
    gchar *path;

    path = g_build_filename(this_configdir, "smslist", NULL);

    fp = fopen(path, "w");

    g_free(path);

    if (!fp)
    {
	print_debug("cannot create smslist!\n");
	return;
    }

    nick = g_malloc0(100);
    mobile = g_malloc0(20);
    
    while (smu)
    {
	GGaduContact *k = (GGaduContact *)smu->data;
	char *t;
    
	/* Podmiana srednikow na przecinki */
	for (t = k->nick; *t; t++)
	    if (*t == ';')
		*t = ',';

	fprintf(fp,"%s;%s\n",k->nick,k->mobile);
	smu = smu->next;
    }

    fclose(fp);

    g_free(nick);
    g_free(mobile);
}

/* wczytanie listy numerow z pliku */
void load_smslist()
{
    FILE *fp;
    GGaduContact *k = NULL;
    gchar *path;
    
    path = g_build_filename(this_configdir, "smslist", NULL);
    
    fp = fopen(path, "r");
    
    g_free(path);
    if (!fp)
    {
	print_debug("smslist not found\n");
	return;
    }

    nick = g_malloc0(100);
    mobile = g_malloc0(20);
    
    while (fscanf(fp,"%[^;];%[^\n]\n",nick,mobile) != EOF)
    {
	k = g_new0(GGaduContact, 1);
	k->nick = g_strdup(nick);
	k->mobile = g_strdup(mobile);
	k->id = k->mobile;
	k->status = 1;
	print_debug("%s %s\n",k->nick,k->mobile);
	smslist = g_slist_append(smslist, k);
    }

    fclose(fp);

    g_free(nick);
    g_free(mobile);
}
