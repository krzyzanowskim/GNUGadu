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
	ggadu_conf.c \
	perl_embed.c \
	gg-types.h \
	unified-types.h \
	signals.h \
	plugins.h \
	support.h \
	menu.h \
	dialog.h \
	repo.h \
	ggadu_conf.h \
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
	@cat $(top_builddir)/lib/gg-types.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/unified-types.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/dialog.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/perl_embed.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/repo.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/support.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/menu.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/plugins.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/ggadu_conf.h >> $(top_builddir)/lib/gg2_core.h
	@cat $(top_builddir)/lib/signals.h >> $(top_builddir)/lib/gg2_core.h
	@echo "#endif" >> $(top_builddir)/lib/gg2_core.h
	sed -e 's/#include \"unified-types.h\"//g' $(top_builddir)/lib/gg2_core.h > $(top_builddir)/lib/gg2_core.h-
	sed -e 's/#include \"gg-types.h\"//g' $(top_builddir)/lib/gg2_core.h- > $(top_builddir)/lib/gg2_core.h
	rm -f $(top_builddir)/lib/gg2_core.h-
