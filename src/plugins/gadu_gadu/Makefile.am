SUBDIRS=libgadu

GG_LIBS=-lgadu -lssl -lcrypto -ldl 

pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libgadu_gadu_plugin.la
noinst_LIBRARIES =

libgadu_gadu_plugin_la_SOURCES = gadu_gadu_plugin.c \
    gadu_gadu_plugin.h

libgadu_gadu_plugin_la_LIBADD = $(GG_LIBS) $(top_builddir)/lib/libgg2_core.la
    
libgadu_gadu_plugin_la_LDFLAGS = -avoid-version -L$(top_builddir)/src/plugins/gadu_gadu/libgadu/lib
libgadu_gadu_plugin_la_CFLAGS = -pthread -I$(top_builddir)/src/plugins/gadu_gadu/libgadu/lib
