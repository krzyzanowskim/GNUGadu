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

libgg2_core_ladir = $(includedir)
libgg2_core_la_SOURCES = unified-types.c \
	signals.c \
	plugins.c \
	support.c \
	menu.c \
	dialog.c \
	repo.c \
	perl_embed.c \
	gg-types.h \
	unified-types.h \
	signals.h \
	plugins.h \
	support.h \
	menu.h \
	dialog.h \
	repo.h \
	perl_embed.h
	
libgg2_core_la_HEADERS = libgg2_core.h

libgg2_core_la_LIBADD = $(PERL_OPTION_LDFLAGS) -ldl 
libgg2_core_la_LDFLAGS = -version-info 2:0:0

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgg2_core.pc

all:
	@echo "/* This file is generated automatically. Do not edit. */\n" > $(top_builddir)/src/libgg2_core.h
	@echo "#ifndef LIBGG2_CORE_H\n" >> $(top_builddir)/src/libgg2_core.h
	@echo "#define LIBGG2_CORE_H\n" >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/config.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/gg-types.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/dialog.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/perl_embed.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/repo.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/support.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/menu.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/plugins.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/signals.h >> $(top_builddir)/src/libgg2_core.h
	@cat $(top_builddir)/src/unified-types.h >> $(top_builddir)/src/libgg2_core.h
	@echo "#endif\n" >> $(top_builddir)/src/libgg2_core.h
