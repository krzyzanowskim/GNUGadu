## Process this file with automake to produce Makefile.in

SUBDIRS = lib src po

EXTRA_DIST = autogen.sh ANNOUNCE AUTHORS ChangeLog COPYING \
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
		intltool-extract.in \
		intltool-merge.in \
		intltool-update.in
		

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	    fi \
	  done \
	  fi
	if test -d $(srcdir)/pixmaps/emoticons; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps/emoticons; \
	  for pixmapemot in $(srcdir)/pixmaps/emoticons/*; do \
	    if test -f $$pixmapemot; then \
	      $(INSTALL_DATA) $$pixmapemot $(DESTDIR)$(pkgdatadir)/pixmaps/emoticons; \
	    fi \
	  done \
	  fi
	if test -d $(srcdir)/sounds; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/sounds; \
	  for sound in $(srcdir)/sounds/*; do \
	    if test -f $$sound; then \
	      $(INSTALL_DATA) $$sound $(DESTDIR)$(pkgdatadir)/sounds; \
	    fi \
	  done \
	  fi
	if test -d $(srcdir)/themes; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/themes; \
	  for theme in $(srcdir)/themes/*; do \
	    if test -f $$theme; then \
	      $(INSTALL_DATA) $$theme $(DESTDIR)$(pkgdatadir)/themes; \
	    fi \
	  done \
	  fi
	if test -d $(srcdir)/pixmaps/icons; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps/icons; \
	  for icons in $(srcdir)/pixmaps/icons/*; do \
	    if test -d $$icons && test $$icons != "$(srcdir)/pixmaps/icons/CVS" ; then \
	      cp -rp $$icons $(DESTDIR)$(pkgdatadir)/pixmaps/icons; \
	    fi \
	  done \
	  fi

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi
	if test -d pixmaps/emoticons; then \
	  mkdir $(distdir)/pixmaps/emoticons; \
	  for pixmapemot in pixmaps/emoticons/*; do \
	    if test -f $$pixmapemot; then \
	      cp -p $$pixmapemot $(distdir)/pixmaps/emoticons; \
	    fi \
	  done \
	fi
	if test -d sounds; then \
	  mkdir $(distdir)/sounds; \
	  for sound in sounds/*; do \
	    if test -f $$sound; then \
	      cp -p $$sound $(distdir)/sounds; \
	    fi \
	  done \
	fi
	if test -d themes; then \
	  mkdir $(distdir)/themes; \
	  for theme in themes/*; do \
	    if test -f $$theme; then \
	      cp -p $$theme $(distdir)/themes; \
	    fi \
	  done \
	fi
	if test -d pixmaps/icons; then \
	  mkdir $(distdir)/pixmaps/icons; \
	  for icons in pixmaps/icons/*; do \
	    if test -d $$icons; then \
	      cp -rp $$icons $(distdir)/pixmaps/icons; \
	    fi \
	  done \
	fi
	if test -d doc/gg2-doc; then \
	  mkdir $(distdir)/doc/gg2-doc; \
	  for docsf in doc/gg2-doc/*; do \
	    if test -d $$docsf; then \
		cp -rp $$docsf $(distdir)/doc/gg2-doc; \
	    fi \
	  done \
	fi
#	find ./$(distdir) -true -name "CVS" -type d -exec rm -rf {} \;

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
