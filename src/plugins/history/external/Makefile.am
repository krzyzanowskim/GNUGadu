pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libhistory_external_plugin.la
noinst_LIBRARIES =

libhistory_external_plugin_la_CFLAGS = -DBINDIR=\"$(bindir)\" $(HISTORY_EXTERNAL_VIEWER_CFLAGS) 
libhistory_external_plugin_la_SOURCES = history_viewer.c history_viewer.h
libhistory_external_plugin_la_LIBADD = $(HISTORY_EXTERNAL_VIEWER_LIBS) $(top_builddir)/lib/libgg2_core.la
libhistory_external_plugin_la_LDFLAGS = -avoid-version  


