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
libgg2_core_la_LDFLAGS = -version-info 2:2:0
libgg2_core_la_CFLAGS = -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gg2_core.pc
