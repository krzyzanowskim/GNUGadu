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

