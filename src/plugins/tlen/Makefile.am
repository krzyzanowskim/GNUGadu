pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libtlen_plugin.la
noinst_LIBRARIES =

libtlen_plugin_la_SOURCES = tlen_plugin.c \
    tlen_plugin.h

libtlen_plugin_la_LIBADD = $(TLEN_LIBS) $(top_builddir)/lib/libgg2_core.la
    
libtlen_plugin_la_CFLAGS = -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"
libtlen_plugin_la_LDFLAGS = -avoid-version  
