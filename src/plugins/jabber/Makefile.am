pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libjabber_plugin.la
noinst_LIBRARIES =

libjabber_plugin_la_SOURCES = jabber_plugin.c \
    jabber_plugin.h \
    jabber_plugin_roster.c \
    jabber_plugin_roster.h \
    jabber_plugin_protocol.c \
    jabber_plugin_protocol.h

libjabber_plugin_la_LIBADD = $(JABBER_LIBS) $(top_builddir)/lib/libgg2_core.la


libjabber_plugin_la_LDFLAGS = -avoid-version  
