#define DBUS_API_SUBJECT_TO_CHANGE

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include "dbus_plugin.h"

static gboolean send_ping(DBusConnection * bus);
static void get_protocols(DBusConnection *bus);
static void open_chat(DBusConnection *bus);

int main(int argc, char **argv)
{
	GMainLoop *loop;
	DBusConnection *bus;
	DBusError error;

	/* Create a new event loop to run in */
	loop = g_main_loop_new(NULL, FALSE);

	/* Get a connection to the session bus */
	dbus_error_init(&error);
	bus = dbus_bus_get(DBUS_BUS_SESSION, &error);
	if (!bus)
	{
		g_warning("Failed to connect to the D-BUS daemon: %s", error.message);
		dbus_error_free(&error);
		return 1;
	}

	/* get avaiable protocols */
	get_protocols(bus);
	/* Set up this connection to work in a GLib event loop */
	dbus_connection_setup_with_g_main(bus, NULL);
	/* Every second call send_ping() with the bus as an argument */
	g_timeout_add(2000, (GSourceFunc) send_ping, bus);

	/* Start the event loop */
	g_main_loop_run(loop);
	return 0;
}


static gboolean send_ping(DBusConnection *bus)
{
	DBusMessage *message;

	message = dbus_message_new_method_call(DBUS_ORG_FREEDESKTOP_IM_SERVICE, 
					       DBUS_ORG_FREEDESKTOP_IM_OBJECT,
					       DBUS_ORG_FREEDESKTOP_IM_INTERFACE,
					       DBUS_ORG_FREEDESKTOP_IM_GET_PRESENCE);
					       
	dbus_message_append_args(message, DBUS_TYPE_STRING, "gg://5074881", DBUS_TYPE_INVALID);
//	dbus_message_set_no_reply(message, TRUE);
	/* Send the signal */
	dbus_connection_send(bus, message, NULL);
	/* Free the signal now we have finished with it */
	dbus_message_unref(message);
	open_chat(bus);
	/* Tell the user we send a signal */
	/* Return TRUE to tell the event loop we want to be called again */
	return TRUE;
}

static void get_protocols(DBusConnection *bus)
{
	DBusMessage *message;

	message = dbus_message_new_method_call(DBUS_ORG_FREEDESKTOP_IM_SERVICE, 
					       DBUS_ORG_FREEDESKTOP_IM_OBJECT,
					       DBUS_ORG_FREEDESKTOP_IM_INTERFACE,
					       DBUS_ORG_FREEDESKTOP_IM_GET_PROTOCOLS);
	
	dbus_connection_send(bus, message, NULL);

	dbus_message_unref(message);
}

static void open_chat(DBusConnection *bus)
{
	DBusMessage *message;

	message = dbus_message_new_method_call(DBUS_ORG_FREEDESKTOP_IM_SERVICE, 
					       DBUS_ORG_FREEDESKTOP_IM_OBJECT,
					       DBUS_ORG_FREEDESKTOP_IM_INTERFACE,
					       DBUS_ORG_FREEDESKTOP_IM_OPEN_CHAT);
	
	dbus_message_append_args(message, DBUS_TYPE_STRING, "gg://5074881", DBUS_TYPE_INVALID);
	dbus_connection_send(bus, message, NULL);

	dbus_message_unref(message);
}
