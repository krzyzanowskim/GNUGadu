# libgg2.so
pkgdir = $(libdir)
pkg_LTLIBRARIES = libgg2_core.la
noinst_LIBRARIES =

libgg2_core_ladir = $(includedir)
libgg2_core_la_SOURCES = \
	signals.c \
	plugins.c \
	ggadu_support.c \
	ggadu_menu.c \
	ggadu_repo.c \
	ggadu_conf.c \
	ggadu_dialog.c \
	perl_embed.c \
	ggadu_types.h \
	ggadu_types.c \
	signals.h \
	plugins.h \
	ggadu_support.h \
	ggadu_menu.h \
	ggadu_repo.h \
	ggadu_conf.h \
	ggadu_dialog.h \
	perl_embed.h 
	
libgg2_core_la_HEADERS = gg2_core.h

libgg2_core_la_LIBADD = $(PERL_OPTION_LDFLAGS) -ldl 
libgg2_core_la_LDFLAGS = -version-info 2:0:0

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gg2_core.pc

all:
	@echo "/* This file is generated automatically. Do not edit. */" > $(top_builddir)/lib/gg2_core.h
	@echo "#ifndef GG2_CORE_H" >> $(top_builddir)/lib/gg2_core.h
	@echo "#define GG2_CORE_H" >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/config.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/ggadu_types.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/ggadu_dialog.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/perl_embed.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/ggadu_repo.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/ggadu_support.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/ggadu_menu.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/plugins.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/ggadu_conf.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/signals.h >> $(top_builddir)/lib/gg2_core.h
	@echo "#endif" >> $(top_builddir)/lib/gg2_core.h
	sed -e 's/#include \"ggadu_types.h\"//g' $(top_builddir)/lib/gg2_core.h > $(top_builddir)/lib/gg2_core.h-
	mv -f $(top_builddir)/lib/gg2_core.h- $(top_builddir)/lib/gg2_core.h
