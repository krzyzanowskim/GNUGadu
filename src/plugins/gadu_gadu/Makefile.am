#SUBDIRS=libgadu

pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libgadu_gadu_plugin.la
noinst_LIBRARIES =

libgadu_gadu_plugin_la_SOURCES = \
    gadu_gadu_plugin.c \
    gadu_gadu_plugin.h \
    libgadu/lib/common.c \
    libgadu/lib/dcc.c \
    libgadu/lib/events.c \
    libgadu/lib/http.c \
    libgadu/lib/libgadu.c \
    libgadu/lib/obsolete.c \
    libgadu/lib/pubdir.c \
    libgadu/lib/pubdir50.c 

libgadu_gadu_plugin_la_LIBADD =  \
	    $(top_srcdir)/lib/libgg2_core.la 
    
libgadu_gadu_plugin_la_LDFLAGS = -avoid-version -pthread $(OSSL_LIBS)
libgadu_gadu_plugin_la_CFLAGS =  -I./libgadu/lib -I/usr/kerberos/include/ $(CFLAGS) $(OSSL_CFLAGS)

EXTRA_DIST = libgadu/autogen.sh \
	libgadu/compat \
	libgadu/m4 \
	libgadu/configure \
	libgadu/configure.in \
	libgadu/aclocal.m4 \
	libgadu/Makefile.am \
	libgadu/Makefile.in \
	libgadu/Makefile \
	libgadu/config.sub \
	libgadu/config.status \
	libgadu/config.guess \
	libgadu/install-sh \
	libgadu/depcomp \
	libgadu/libtool \
	libgadu/ltmain.sh \
	libgadu/missing \
	libgadu/lib/*.c \
	libgadu/lib/libgadu.h \
	libgadu/lib/compat.h \
	libgadu/lib/*.h.in
#	libgadu/intl
