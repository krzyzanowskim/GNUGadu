#SUFFIXES = .c .h .so

pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libGUI_plugin.la
noinst_LIBRARIES =

libGUI_plugin_la_CFLAGS = $(GTK_CFLAGS)

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
    gui_preferences.h

    
libGUI_plugin_la_LIBADD = $(GTK_LIBS) $(top_builddir)/lib/libgg2_core.la
#libGUI_plugin_la_LIBADD = $(GTK_LIBS)
    
libGUI_plugin_la_LDFLAGS = -avoid-version
