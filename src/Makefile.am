SUBDIRS = plugins

# bin
bin_PROGRAMS = gg2

gg2_SOURCES = main.c

gg2_CFLAGS = $(CFLAGS) -DPACKAGE_LIB_DIR=\"$(libdir)/$(PACKAGE)\" -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\" -DPACKAGE_LOCALE_DIR=\"$(localedirCOMMON)\"
gg2_LDADD = $(PERL_OPTION_LDFLAGS) $(top_builddir)/lib/libgg2_core.la

