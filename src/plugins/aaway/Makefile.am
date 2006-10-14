pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libaaway_plugin.la
noinst_LIBRARIES =

libaaway_plugin_la_SOURCES = aaway.c \
	aaway.h

libaaway_plugin_la_CFLAGS = $(CFLAGS) $(AAWAY_CFLAGS) -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"
libaaway_plugin_la_LIBADD = $(AAWAY_LIBS) $(top_builddir)/lib/libgg2_core.la
libaaway_plugin_la_LDFLAGS = -avoid-version

