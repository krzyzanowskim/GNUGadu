pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libremote_plugin.la
noinst_LIBRARIES =

libremote_plugin_la_SOURCES = remote_plugin.c \
    remote_plugin.h
    

libremote_plugin_la_LIBADD = ../../plugins.lo \
    ../../support.lo \
    ../../signals.lo \
    ../../menu.lo \
    ../../unified-types.lo \
    ../../dialog.lo

libremote_plugin_la_LDFLAGS = -module -avoid-version  

