pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libicq_plugin.la
noinst_LIBRARIES =

libicq_plugin_la_SOURCES = icq_plugin.c \
	icq_plugin.h \
	chatsession.c \
	chatsession.h \
	contacts.c \
	contacts.h \
	cyrillic.c \
	eventhandle.c \
	eventhandle.h \
	filesession.c \
	filesession.h \
	icq.h \
	icqbyteorder.c \
	icqbyteorder.h \
	icqevent.c \
	icqevent.h \
	icqlib.c \
	icqlib.h \
	icqpacket.c \
	icqpacket.h \
	icqtypes.h \
	list.c \
	list.h \
	proxy.c \
	queue.c \
	queue.h \
	socketmanager.c \
	socketmanager.h \
	stdpackets.c \
	stdpackets.h \
	tcp.c \
	tcp.h \
	tcpchathandle.c \
	tcpfilehandle.c \
	tcphandle.c \
	tcplink.c \
	tcplink.h \
	timeout.c \
	timeout.h \
	udp.c \
	udp.h \
	udphandle.c \
	util.c \
	util.h

libicq_plugin_la_LIBADD = ../../plugins.lo \
    ../../support.lo \
    ../../signals.lo \
    ../../menu.lo \
    ../../unified-types.lo \
    ../../dialog.lo \
    $(ICQ_LIBS)

libicq_plugin_la_LDFLAGS = -module -avoid-version  
EXTRA_DIST = \
	AUTHORS \
	COPYING \
	ChangeLog \
	INSTALL \
	README \
	VERSION
