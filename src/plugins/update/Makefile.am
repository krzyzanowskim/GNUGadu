pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libupdate_plugin.la
noinst_LIBRARIES =

libupdate_plugin_la_CFLAGS = $(GTK_CFLAGS)

libupdate_plugin_la_SOURCES = update_plugin.c \
    update_plugin.h
    

libupdate_plugin_la_LIBADD = ../../plugins.lo \
    ../../support.lo \
    ../../signals.lo \
    ../../menu.lo \
    ../../unified-types.lo \
    ../../dialog.lo \
    $(GTK_LIBS)

libupdate_plugin_la_LDFLAGS = -module -avoid-version  
