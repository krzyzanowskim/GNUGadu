## Process this file with automake to produce Makefile.in

SUBDIRS = intl m4 lib src po pixmaps sounds themes

EXTRA_DIST = \
	autogen.sh \
	ANNOUNCE \
	AUTHORS \
	ChangeLog \
	COPYING \
	contrib/example.pl \
	contrib/gg2sms \
	contrib/mkgg2snap \
	contrib/plugin_example/build.sh \
	contrib/plugin_example/my_plugin.c \
	contrib/plugin_example/my_plugin.h \
	contrib/remote_client/Makefile \
	contrib/remote_client/main.c \
	contrib/remote_client/remote_client.c \
	contrib/remote_client/remote_client.h \
	doc/README.remote \
	doc/emoticons.def \
	doc/problems \
	gg2.desktop \
	gg2.spec \
	gg2-devel.SlackBuild \
	slack-desc
#	config.rpath
#	mkinstalldirs
		
dist-hook:
	if test -d doc/gg2-doc; then \
	  mkdir $(distdir)/doc/gg2-doc; \
	  for docsf in doc/gg2-doc/*; do \
	    if test -d $$docsf; then \
		cp -rp $$docsf $(distdir)/doc/gg2-doc; \
	    fi \
	  done \
	fi

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

ACLOCAL_AMFLAGS = -I m4

#if BUILD_GG
#all: config.h
#	(cd $(top_srcdir)/src/plugins/gadu_gadu/libgadu/; make)
#	$(MAKE) $(AM_MAKEFLAGS) all-recursive
#else

all: config.h
	$(MAKE) $(AM_MAKEFLAGS) all-recursive
	
#endif