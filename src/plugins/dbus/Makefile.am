pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libdbus_plugin.la
noinst_LIBRARIES =

libdbus_plugin_la_SOURCES = dbus_plugin.c \
    dbus_plugin.h \
    libofi.h

libdbus_plugin_la_CFLAGS = $(DBUS_CFLAGS)
libdbus_plugin_la_LIBADD = $(DBUS_LIBS) $(top_builddir)/lib/libgg2_core.la
libdbus_plugin_la_LDFLAGS = -avoid-version  

servicesdir = $(DBUS_DIR)
services_DATA = org.freedesktop.im.GG.service

EXTRA_DIST = $(services_DATA)

