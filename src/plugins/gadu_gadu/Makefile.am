#SUBDIRS=libgadu

pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libgadu_gadu_plugin.la
noinst_LIBRARIES =

if EXTERNAL_LIBGADU
libgadu_gadu_plugin_la_SOURCES = \
    gadu_gadu_plugin.c \
    gadu_gadu_plugin.h
else
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
endif

libgadu_gadu_plugin_la_LIBADD =  \
	    $(top_srcdir)/lib/libgg2_core.la 
    
if EXTERNAL_LIBGADU
libgadu_gadu_plugin_la_LDFLAGS = -avoid-version -pthread $(OPENSSL_LIBS) $(LIBGADU_LIBS)
else
libgadu_gadu_plugin_la_LDFLAGS = -avoid-version -pthread $(OPENSSL_LIBS)
endif

if EXTERNAL_LIBGADU
libgadu_gadu_plugin_la_CFLAGS =  -I/usr/kerberos/include/ $(CFLAGS) $(OPENSSL_INCLUDES) $(LIBGADU_CFLAGS)
else
libgadu_gadu_plugin_la_CFLAGS =  -I./libgadu/lib -I/usr/kerberos/include/ $(CFLAGS) $(OPENSSL_INCLUDES)
endif

EXTRA_DIST = libgadu/autogen.sh \
	libgadu/compat \
	libgadu/m4 \
	libgadu/configure \
	libgadu/configure.in \
	libgadu/aclocal.m4 \
	libgadu/Makefile.am \
	libgadu/Makefile.in \
	libgadu/Makefile \
	libgadu/config.status \
	libgadu/install-sh \
	libgadu/missing \
	libgadu/lib/*.c \
	libgadu/lib/libgadu.h \
	libgadu/lib/compat.h \
	libgadu/lib/*.h.in
#	libgadu/ltmain.sh \
#	libgadu/libtool \
#	libgadu/depcomp \
#	libgadu/config.sub \
#	libgadu/config.guess \
#	libgadu/intl
