pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libtlen_plugin.la
noinst_LIBRARIES =

libtlen_plugin_la_SOURCES = tlen_plugin.c \
    tlen_plugin.h

libtlen_plugin_la_LIBADD = ../../plugins.lo \
    ../../support.lo \
    ../../signals.lo \
    ../../menu.lo \
    ../../unified-types.lo \
    ../../dialog.lo \
    $(TLEN_LIBS)
    
libtlen_plugin_la_LDFLAGS = -module -avoid-version  
