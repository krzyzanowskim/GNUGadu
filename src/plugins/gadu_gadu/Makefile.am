pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libgadu_gadu_plugin.la
noinst_LIBRARIES =

libgadu_gadu_plugin_la_CFLAGS = $(PERL_OPTION_CFLAGS)


libgadu_gadu_plugin_la_SOURCES = gadu_gadu_plugin.c \
    gadu_gadu_plugin.h

libgadu_gadu_plugin_la_LIBADD = ../../plugins.lo \
    ../../support.lo \
    ../../signals.lo \
    ../../menu.lo \
    ../../dialog.lo \
    ../../unified-types.lo \
    ../../repo.lo \
    $(GG_LIBS)
    
libgadu_gadu_plugin_la_LDFLAGS = -module -avoid-version  
