pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libsound_esd_plugin.la
noinst_LIBRARIES =

libsound_esd_plugin_la_CFLAGS = $(ESD_CFLAGS)

libsound_esd_plugin_la_SOURCES = plugin_sound_esd.c

libsound_esd_plugin_la_LIBADD = $(ESD_LIBS) $(top_builddir)/lib/libgg2_core.la

libsound_esd_plugin_la_LDFLAGS = -avoid-version  


