#SUBDIRS=libgadu

pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libgadu_gadu_plugin.la
noinst_LIBRARIES =

libgadu_gadu_plugin_la_SOURCES = gadu_gadu_plugin.c \
    gadu_gadu_plugin.h

libgadu_gadu_plugin_la_LIBADD = $(OSSL_LIBS) $(top_srcdir)/lib/libgg2_core.la $(top_srcdir)/src/plugins/gadu_gadu/libgadu/lib/.libs/libgadu.a
    
libgadu_gadu_plugin_la_LDFLAGS = -avoid-version
#libgadu_gadu_plugin_la_LDFLAGS = -avoid-version -L$(top_builddir)/src/plugins/gadu_gadu/libgadu/lib
libgadu_gadu_plugin_la_CFLAGS = -pthread -I$(top_srcdir)/src/plugins/gadu_gadu/libgadu/lib -I$(top_srcdir)/src/plugins/gadu_gadu/libgadu $(CFLAGS)

EXTRA_DIST = libgadu/compat \
	libgadu/compat \
	libgadu/m4 \
	libgadu/intl \
	libgadu/configure \
	libgadu/configure.in \
	libgadu/aclocal.m4 \
	libgadu/Makefile.am \
	libgadu/Makefile.in \
	libgadu/Makefile \
	libgadu/config.h.in \
	libgadu/config.sub \
	libgadu/config.status \
	libgadu/config.guess \
	libgadu/install-sh \
	libgadu/depcomp \
	libgadu/libtool \
	libgadu/ltmain.sh \
	libgadu/missing \
	libgadu/lib/Makefile.am \
	libgadu/lib/Makefile.in \
	libgadu/lib/Makefile \
	libgadu/lib/*.c \
	libgadu/lib/*.h \
	libgadu/lib/*.h.in