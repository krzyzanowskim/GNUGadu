pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libdocklet_dockapp_plugin.la
noinst_LIBRARIES =

libdocklet_dockapp_plugin_la_CFLAGS = $(GTK_CFLAGS) -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"

libdocklet_dockapp_plugin_la_SOURCES = dockapp_plugin.c \
    dockapp_plugin.h \
    dockapp.xpm
    

libdocklet_dockapp_plugin_la_LIBADD = $(GTK_LIBS) $(top_builddir)/lib/libgg2_core.la
libdocklet_dockapp_plugin_la_LDFLAGS = -avoid-version  

