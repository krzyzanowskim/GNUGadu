pkgdir = $(libdir)
pkg_LTLIBRARIES = libgadu.la

libgadu_ladir = $(includedir)
libgadu_la_SOURCES = common.c \
	dcc.c \
	events.c \
	http.c \
	libgadu.c \
	obsolete.c \
	pubdir50.c \
	pubdir.c \
	compat.h \
	libgadu-config.h \
	libgadu.h


EXTRA_DIST=libgadu-config.h.in
	
#libgadu_la_HEADERS = libgadu.h
#noinst_libgadu_la_HEADERS = libgadu.h
libgadu_la_CFLAGS = @CFLAGS_LIBGADU@ -I. -I. -I$(top_srcdir)/src/plugins/gadu_gadu/libgadu/lib -I$(top_srcdir)/src/plugins/gadu_gadu/libgadu -Wall

libgadu_la_LIBADD = @LIBS_LIBGADU@
libgadu_la_LDFLAGS = -version-info 3:0:0 -static
