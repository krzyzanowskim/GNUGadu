#SUFFIXES = .c .h .so

pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libGUI_plugin.la
noinst_LIBRARIES = 

if USE_GTKSPELL
libGUI_plugin_la_CFLAGS = $(GTK_CFLAGS) $(GTKSPELL_CFLAGS) -DUSE_GTKSPELL -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"
else
libGUI_plugin_la_CFLAGS = $(GTK_CFLAGS) -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"
endif

libGUI_plugin_la_SOURCES = gui_main.c \
    gui_main.h \
    GUI_plugin.c \
    GUI_plugin.h \
    gui_chat.c \
    gui_chat.h \
    gui_support.c \
    gui_support.h \
    gui_search.c \
    gui_search.h \
    gui_dialogs.c \
    gui_dialogs.h \
    gui_userview.c \
    gui_userview.h \
    gui_handlers.c \
    gui_handlers.h \
    gui_preferences.c \
    gui_preferences.h \
    gtkanimlabel.c \
    gtkanimlabel.h \
    gui_chat_session.c \
    gui_chat_session.h \
    gtksourceiter.c \
    gtksourceiter.h \
    gtkimhtml.c \
    gtkimhtml.h

    
if USE_GTKSPELL
libGUI_plugin_la_LIBADD = $(GTK_LIBS) $(GTKSPELL_LIBS) $(top_builddir)/lib/libgg2_core.la
else
libGUI_plugin_la_LIBADD = $(GTK_LIBS) $(top_builddir)/lib/libgg2_core.la
endif
    
libGUI_plugin_la_LDFLAGS = -avoid-version
