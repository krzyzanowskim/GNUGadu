pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libdocklet_plugin.la
noinst_LIBRARIES =

libdocklet_plugin_la_CFLAGS = $(GTK_CFLAGS)

libdocklet_plugin_la_SOURCES = docklet_plugin.c \
    docklet_plugin.h \
    eggtrayicon.c \
    eggtrayicon.h
    

libdocklet_plugin_la_LIBADD = ../../plugins.lo \
    ../../support.lo \
    ../../signals.lo \
    $(GTK_LIBS)

libdocklet_plugin_la_LDFLAGS = -module -avoid-version  

