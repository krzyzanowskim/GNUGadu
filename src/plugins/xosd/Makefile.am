pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libxosd_plugin.la
noinst_LIBRARIES =

libxosd_plugin_la_CFLAGS = $(XOSD_CFLAGS)

libxosd_plugin_la_SOURCES = plugin_xosd.c\
    plugin_xosd.h

libxosd_plugin_la_LIBADD = $(XOSD_LIBS) $(top_builddir)/lib/libgg2_core.la
    
libxosd_plugin_la_LDFLAGS = -avoid-version  
