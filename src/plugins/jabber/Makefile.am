pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libjabber_plugin.la
noinst_LIBRARIES =

libjabber_plugin_la_SOURCES = jabber_plugin.c \
    jabber_plugin.h \
    jabber_protocol.c \
    jabber_protocol.h \
    jabber_login.c \
    jabber_login.h \
    jabber_cb.c \
    jabber_cb.h

libjabber_plugin_la_CFLAGS = $(JABBER_CFLAGS)
libjabber_plugin_la_LIBADD = $(JABBER_LIBS) $(top_builddir)/lib/libgg2_core.la


libjabber_plugin_la_LDFLAGS = -avoid-version  
