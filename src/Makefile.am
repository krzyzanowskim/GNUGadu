SUBDIRS = plugins

# bin
bin_PROGRAMS = gg2

gg2_SOURCES = main.c
gg2_DEPENDENCIES = libgg2_core.la

gg2_CFLAGS = -DPACKAGE_LIB_DIR=\"$(libdir)/$(PACKAGE)\" 
gg2_LDADD = $(PERL_OPTION_LDFLAGS) -ldl libgg2_core.la

# libgg2.so
pkgdir = $(libdir)
pkg_LTLIBRARIES = libgg2_core.la
noinst_LIBRARIES =

libgg2_core_la_SOURCES = gg-types.h \
	unified-types.h \
	unified-types.c \
	signals.c \
	signals.h \
	plugins.c \
	plugins.h \
	support.c \
	support.h \
	menu.c \
	menu.h \
	dialog.c \
	dialog.h \
	repo.c \
	repo.h \
	perl_embed.c \
	perl_embed.h

libgg2_core_la_LIBADD = $(PERL_OPTION_LDFLAGS) -ldl 
libgg2_core_la_LDFLAGS = -version-info 2:0:0

all:
	@cat $(top_builddir)/src/gg-types.h > $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/dialog.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/perl_embed.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/repo.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/support.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/menu.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/plugins.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/signals.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/unified-types.h >> $(top_builddir)/src/libgg2_core.h
