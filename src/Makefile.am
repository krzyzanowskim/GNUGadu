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
	dialog.h

gg2_CFLAGS = -DPACKAGE_LIB_DIR=\"$(libdir)/$(PACKAGE)\"

gg2_LDADD = -ldl