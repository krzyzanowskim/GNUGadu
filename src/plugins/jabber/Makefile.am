pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libjabber_plugin.la
noinst_LIBRARIES =

libjabber_plugin_la_SOURCES = jabber_plugin.c \
    jabber_plugin.h \
    jabber_plugin_roster.c \
    jabber_plugin_roster.h \
    jabber_plugin_protocol.c \
    jabber_plugin_protocol.h

libjabber_plugin_la_LIBADD = ../../plugins.lo \
    ../../support.lo \
    ../../signals.lo \
    ../../menu.lo \
    ../../unified-types.lo \
    ../../dialog.lo \
    $(JABBER_LIBS)
    
libjabber_plugin_la_LDFLAGS = -module -avoid-version  
