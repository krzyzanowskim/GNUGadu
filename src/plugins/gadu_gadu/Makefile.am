SUBDIRS=libgadu

#OSSL_LIBS=-lssl -lcrypto -ldl 

pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libgadu_gadu_plugin.la
noinst_LIBRARIES =

libgadu_gadu_plugin_la_SOURCES = gadu_gadu_plugin.c \
    gadu_gadu_plugin.h

libgadu_gadu_plugin_la_LIBADD = $(OSSL_LIBS) $(top_builddir)/lib/libgg2_core.la $(top_builddir)/src/plugins/gadu_gadu/libgadu/lib/libgadu.a
    
libgadu_gadu_plugin_la_LDFLAGS = -avoid-version
#libgadu_gadu_plugin_la_LDFLAGS = -avoid-version -L$(top_builddir)/src/plugins/gadu_gadu/libgadu/lib
libgadu_gadu_plugin_la_CFLAGS = -pthread -I$(top_builddir)/src/plugins/gadu_gadu/libgadu/lib
