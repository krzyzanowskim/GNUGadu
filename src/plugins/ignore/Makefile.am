pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libignore_main_plugin.la
noinst_LIBRARIES =

libignore_main_plugin_la_CFLAGS = -DBINDIR=\"$(bindir)\" $(CFLAGS) $(IGNORE_CFLAGS) -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"
libignore_main_plugin_la_SOURCES = ignore.c ignore.h
libignore_main_plugin_la_LIBADD = $(IGNORE_LIBS) $(top_builddir)/lib/libgg2_core.la
libignore_main_plugin_la_LDFLAGS = -avoid-version  


