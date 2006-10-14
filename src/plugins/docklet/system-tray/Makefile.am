pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libdocklet_system_tray_plugin.la
noinst_LIBRARIES =

libdocklet_system_tray_plugin_la_CFLAGS = $(GTK_CFLAGS) -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"

libdocklet_system_tray_plugin_la_SOURCES = docklet_plugin.c \
    docklet_plugin.h \
    eggtrayicon.c \
    eggtrayicon.h
    

libdocklet_system_tray_plugin_la_LIBADD = $(GTK_LIBS) $(top_builddir)/lib/libgg2_core.la
libdocklet_system_tray_plugin_la_LDFLAGS = -avoid-version  

