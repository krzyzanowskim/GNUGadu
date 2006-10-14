pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libsound_arts_plugin.la
noinst_LIBRARIES =

libsound_arts_plugin_la_CFLAGS = $(ARTSC_CFLAGS) $(AUDIOFILE_CFLAGS) -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"

libsound_arts_plugin_la_SOURCES = plugin_sound_arts.c sound-arts.c sound-arts.h

libsound_arts_plugin_la_LIBADD = $(ARTSC_LIBS) $(AUDIOFILE_LIBS) $(top_builddir)/lib/libgg2_core.la

libsound_arts_plugin_la_LDFLAGS = -avoid-version  


