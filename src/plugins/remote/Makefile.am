pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libremote_plugin.la
noinst_LIBRARIES =

libremote_plugin_la_SOURCES = remote_plugin.c \
    remote_plugin.h
    
libremote_plugin_la_LIBADD = $(top_builddir)/lib/libgg2_core.la

libremote_plugin_la_LDFLAGS = -avoid-version  

