INCLUDES =

lib_LTLIBRARIES = libmozilla_org_freedesktop_im_dbus.la

libmozilla_org_freedesktop_im_dbus_la_SOURCES = \
	mozilla-plugin.c \
	npunix.c

# this stinks	
X_CFLAGS="-I/usr/X11R6/include"
X_LIBS="-L/usr/X11R6/lib"

libmozilla_org_freedesktop_im_dbus_la_CFLAGS = $(MOZILLA_PLUGIN_CFLAGS) $(X_CFLAGS)
libmozilla_org_freedesktop_im_dbus_la_LDFLAGS = -avoid-version  $(X_LIBS)
libmozilla_org_freedesktop_im_dbus_la_LIBADD = $(MOZILLA_PLUGIN_LIBS)
