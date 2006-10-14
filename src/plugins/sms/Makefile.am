pkgdir = $(libdir)/gg2

pkg_LTLIBRARIES = libsms_plugin.la
noinst_LIBRARIES =

libsms_plugin_la_CFLAGS = -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"

libsms_plugin_la_SOURCES = sms_gui.c \
    sms_gui.h \
    sms_core.c \
    sms_core.h

libsms_plugin_la_LIBADD = $(top_builddir)/lib/libgg2_core.la
    
libsms_plugin_la_LDFLAGS = -avoid-version  
