pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libxosd_plugin.la
noinst_LIBRARIES =

libxosd_plugin_la_CFLAGS = $(XOSD_CFLAGS)

libxosd_plugin_la_SOURCES = plugin_xosd.c\
    plugin_xosd.h

libxosd_plugin_la_LIBADD = ../../plugins.lo \
    ../../support.lo \
    ../../signals.lo \
    ../../menu.lo \
    ../../dialog.lo \
    ../../unified-types.lo \
    $(XOSD_LIBS)
    
libxosd_plugin_la_LDFLAGS = -module -avoid-version  
