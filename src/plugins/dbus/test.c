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
	/* open chat window */
	open_chat(bus);
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
	/* Send the signal */
	dbus_connection_send(bus, message, NULL);
	/* Free the signal now we have finished with it */
	dbus_message_unref(message);
	/* Tell the user we send a signal */
	/* Return TRUE to tell the event loop we want to be called again */
	return TRUE;
}

static void get_protocols(DBusConnection *bus)
{
	DBusMessage *message = NULL;
	DBusMessage *reply = NULL;
	DBusPendingCall *pending_call;
	DBusMessageIter iter;

	message = dbus_message_new_method_call(DBUS_ORG_FREEDESKTOP_IM_SERVICE, 
					       DBUS_ORG_FREEDESKTOP_IM_OBJECT,
					       DBUS_ORG_FREEDESKTOP_IM_INTERFACE,
					       DBUS_ORG_FREEDESKTOP_IM_GET_PROTOCOLS);
	
	/*dbus_connection_send(bus, message, NULL);*/
	dbus_connection_send_with_reply(bus,message,&pending_call,-1);
	dbus_pending_call_block (pending_call);
	reply = dbus_pending_call_get_reply (pending_call);
	
	/* process reply */
	dbus_message_iter_init(reply,&iter);
	do {
	    gchar *protocol = NULL;
	    if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING)
	          break;
		  
	    protocol = g_strdup (dbus_message_iter_get_string (&iter));
	    g_print("return getProtocols: %s\n",protocol);
	    g_free(protocol);
	    
	} while (dbus_message_iter_next (&iter));

	dbus_message_unref(message);
	dbus_message_unref(reply);
	dbus_pending_call_unref (pending_call);
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
