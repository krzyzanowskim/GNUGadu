SUBDIRS = plugins

# bin
bin_PROGRAMS = gg2

gg2_SOURCES = main.c
#gg2_DEPENDENCIES = libgg2_core.la

gg2_CFLAGS = $(CFLAGS) -DPACKAGE_LIB_DIR=\"$(libdir)/$(PACKAGE)\" 
#gg2_LDFLAGS = -export-dynamic
#gg2_LDADD = $(PERL_OPTION_LDFLAGS) -ldl $(top_builddir)/lib/libgg2_core.la
gg2_LDADD = $(PERL_OPTION_LDFLAGS) $(top_builddir)/lib/libgg2_core.la

