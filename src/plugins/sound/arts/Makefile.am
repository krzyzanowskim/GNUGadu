pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libsound_arts_plugin.la
noinst_LIBRARIES =

libsound_arts_plugin_la_CFLAGS = $(ARTS_CFLAGS)
libsound_arts_plugin_la_CXXFLAGS = $(ARTS_CXXFLAGS)

libsound_arts_plugin_la_SOURCES = plugin_sound_arts.c sound-arts.cpp sound-arts.h

libsound_arts_plugin_la_LIBADD = $(ARTS_LIBS) $(top_builddir)/lib/libgg2_core.la

libsound_arts_plugin_la_LDFLAGS = -avoid-version  


