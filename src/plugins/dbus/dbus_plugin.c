/* $Id: dbus_plugin.c,v 1.5 2004/10/26 16:26:05 krzyzak Exp $ */

/* 
 * DBUS plugin code for GNU Gadu 2 
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

#define DBUS_API_SUBJECT_TO_CHANGE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "gg2_core.h"
#include "dbus_plugin.h"

GGaduPlugin *plugin_handler = NULL;
GGadu_PLUGIN_INIT("dbus", GGADU_PLUGIN_TYPE_MISC);

static void dbus_plugin_unregistered_func(DBusConnection * connection, gpointer user_data)
{
	print_debug("plumcium");
}


static DBusHandlerResult dbus_plugin_message_func(DBusConnection * connection, DBusMessage * message,
						  gpointer user_data)
{
	DBusError error;
	dbus_error_init(&error);

	print_debug("DBUS: member=%s path=%s interface=%s type=%d", dbus_message_get_member(message),
		    dbus_message_get_path(message), 
		    dbus_message_get_interface(message),
		    dbus_message_get_type(message));


	if (dbus_message_is_signal(message, DBUS_INTERFACE_ORG_FREEDESKTOP_LOCAL, "Disconnected"))
	{
		print_debug("dbus signal: Disconnected");
		dbus_error_free(&error);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	/* getPresence */
	if (dbus_message_is_method_call
	    (message, DBUS_ORG_FREEDESKTOP_IM_INTERFACE, DBUS_ORG_FREEDESKTOP_IM_GET_PRESENCE))
	{
		/* URI of the user which we have to return presence. ex.  gg://13245  */
		gchar *contactURI = NULL;
		if (dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &contactURI, DBUS_TYPE_INVALID))
		{
			print_debug("DBUS getPresence: search %s", contactURI);
			dbus_free(contactURI);
		}
		dbus_error_free(&error);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	dbus_error_free(&error);
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


static DBusObjectPathVTable vtable = {
	dbus_plugin_unregistered_func,
	dbus_plugin_message_func,
	NULL,
};


GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
	print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);
	GGadu_PLUGIN_ACTIVATE(conf_ptr);
	plugin_handler = register_plugin(GGadu_PLUGIN_NAME, _("DBUS interface plugin"));
	return plugin_handler;
}

void start_plugin()
{
	/* initialize dbus */
	DBusConnection *bus = NULL;
	GError *error = NULL;
	DBusError derror;

	dbus_g_thread_init();

	bus = dbus_g_connection_get_connection(dbus_g_bus_get(DBUS_BUS_SESSION, &error));
	if (!bus)
	{
		g_warning("Failed to connect to the D-BUS daemon: %s\n", error->message);
		g_error_free(error);
		return;
	}

	dbus_connection_setup_with_g_main(bus, g_main_loop_get_context(config->main_loop));

	dbus_error_init(&derror);
	dbus_bus_acquire_service(bus, DBUS_ORG_FREEDESKTOP_IM_SERVICE, 0, &derror);
	if (dbus_error_is_set(&derror))
	{
		g_warning("DBUS: Failed to acquire IM service. %s",derror.message);
		dbus_error_free(&derror);
		return;
	}

	if (!dbus_connection_register_object_path(bus, DBUS_ORG_FREEDESKTOP_IM_OBJECT, &vtable, NULL))
	{
		g_warning("DBUS: Failed to register object path.");
		return;
	}

	print_debug("dbus stared");
	return;
}

void destroy_plugin()
{
	print_debug("destroy_plugin %s", GGadu_PLUGIN_NAME);
}



/*
	dbus_bus_add_match (bus, "type='signal',interface='"DBUS_IM_FREEDESKTOP_ORG_SIGNAL_INTERFACE"'",&dbuserror);
	dbus_connection_add_filter (bus,dbus_signal_filter, NULL,NULL);
*/
/*static DBusHandlerResult dbus_signal_filter(DBusConnection * connection, DBusMessage * message, void *user_data)
{
	if (dbus_message_is_signal(message, DBUS_IM_FREEDESKTOP_ORG_SIGNAL_INTERFACE, "Ping"))
	{
		DBusError error;
		char *s;
		dbus_error_init(&error);
		if (dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID))
		{
			print_debug("DBUS: Ping received: %s\n", s);
			dbus_free(s);
		}
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
*/
