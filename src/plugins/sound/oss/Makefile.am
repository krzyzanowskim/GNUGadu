pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libsound_oss_plugin.la
noinst_LIBRARIES =

libsound_oss_plugin_la_CFLAGS = $(AUDIOFILE_CFLAGS) -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"

libsound_oss_plugin_la_SOURCES = plugin_sound_oss.c

libsound_oss_plugin_la_LIBADD = $(AUDIOFILE_LIBS) $(top_builddir)/lib/libgg2_core.la

libsound_oss_plugin_la_LDFLAGS = -avoid-version  


