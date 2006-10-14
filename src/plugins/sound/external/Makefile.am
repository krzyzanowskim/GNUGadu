pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libsound_external_plugin.la
noinst_LIBRARIES =

libsound_external_plugin_la_SOURCES = plugin_sound_external.c plugin_sound_external.h

libsound_external_plugin_la_LIBADD = $(top_builddir)/lib/libgg2_core.la
libsound_external_plugin_la_CFLAGS = -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"
libsound_external_plugin_la_LDFLAGS = -avoid-version  


