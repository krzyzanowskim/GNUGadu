pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libdocklet_plugin.la
noinst_LIBRARIES =

libdocklet_plugin_la_CFLAGS = $(GTK_CFLAGS)

libdocklet_plugin_la_SOURCES = docklet_plugin.c \
    docklet_plugin.h \
    eggtrayicon.c \
    eggtrayicon.h
    

libdocklet_plugin_la_LIBADD = $(GTK_LIBS) $(top_builddir)/lib/libgg2_core.la
libdocklet_plugin_la_LDFLAGS = -avoid-version  

