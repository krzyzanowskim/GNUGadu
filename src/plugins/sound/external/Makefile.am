pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libsound_external_plugin.la
noinst_LIBRARIES =

libsound_external_plugin_la_SOURCES = plugin_sound_external.c plugin_sound_external.h

libsound_external_plugin_la_LIBADD = ../../../plugins.lo \
    ../../../support.lo \
    ../../../signals.lo \
    ../../../menu.lo \
    ../../../dialog.lo \
    ../../../unified-types.lo

libsound_external_plugin_la_LDFLAGS = -module -avoid-version  


