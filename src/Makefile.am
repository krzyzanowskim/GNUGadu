SUBDIRS = plugins


bin_PROGRAMS = gg2


gg2_SOURCES = main.c \
	gg-types.h \
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

gg2_CFLAGS = $(PERL_OPTION_CFLAGS) -DPACKAGE_LIB_DIR=\"$(libdir)/$(PACKAGE)\" 

gg2_LDADD = $(PERL_OPTION_LDFLAGS) -ldl 
#      AC_SUBST(PERL_OPTION_LIBS)

