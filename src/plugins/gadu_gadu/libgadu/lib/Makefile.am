pkgdir = $(libdir)
pkg_LTLIBRARIES = libgadu.la
noinst_LIBRARIES =

libgadu_ladir = $(includedir)
libgadu_la_SOURCES = common.c \
	dcc.c \
	events.c \
	http.c \
	libgadu.c \
	obsolete.c \
	pubdir50.c \
	pubdir.c
	
	
libgadu_la_HEADERS = compat.h libgadu-config.h libgadu.h
libgadu_la_CFLAGS = @CFLAGS_LIBGADU@ -I. -I.. -Wall

libgadu_la_LIBADD = @LIBS_LIBGADU@
libgadu_la_LDFLAGS = -version-info 3:0:0
