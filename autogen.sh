#!/bin/bash

: ${AUTOCONF=autoconf}
: ${AUTOHEADER=autoheader}
: ${AUTOMAKE=automake-1.9}
: ${ACLOCAL=aclocal-1.9}
: ${LIBTOOLIZE=libtoolize}
: ${INTLTOOLIZE=intltoolize}
: ${LIBTOOL=libtool}

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

# chujowe, ale ja jestem lamer i nie wiem jak to teraz zrobic ladniej
CONFIGURE=configure.in
ACPATH1="/usr/share/aclocal";
ACPATH2="/opt/gnome2/share/aclocal";
DIE=0

if [ -d $ACPATH1 ]; then
    ACPATH="-I $ACPATH1"
fi

if [ -d $ACPATH2 ]; then
    ACPATH="$ACPATH -I $ACPATH2"
fi


(grep "^AC_PROG_INTLTOOL" $srcdir/$CONFIGURE >/dev/null) && {
  ($INTLTOOLIZE --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "You must have \`intltoolize' installed to compile $PROJECT."
    echo "Get ftp://ftp.gnome.org/pub/GNOME/stable/sources/intltool/intltool-0.22.tar.gz"
    echo "(or a newer version if it is available)"
    DIE=1
  }
}


if test "$DIE" -eq 1; then 
        exit 1
fi

echo "gettextize"
#gettextize --force --copy --no-changelog || exit 1
# comment the next line when you have the system without NLS
#if ! gettextize --version | grep -q '0\.10\.' ; then
#    gettextize -c -f --intl --no-changelog
#else
#    gettextize -c -f
#fi
if grep "^AM_[A-Z0-9_]\{1,\}_GETTEXT" "$CONFIGURE" >/dev/null; then
  if grep "sed.*POTFILES" "$CONFIGURE" >/dev/null; then
    GETTEXTIZE=""
  else
    if grep "^AM_GLIB_GNU_GETTEXT" "$CONFIGURE" >/dev/null; then
      GETTEXTIZE="glib-gettextize"
      GETTEXTIZE_URL="ftp://ftp.gtk.org/pub/gtk/v2.0/glib-2.0.0.tar.gz"
    else
      GETTEXTIZE="gettextize"
      GETTEXTIZE_URL="ftp://alpha.gnu.org/gnu/gettext-0.10.35.tar.gz"
    fi
                                                                                                          
    $GETTEXTIZE --version < /dev/null > /dev/null 2>&1
    if test $? -ne 0; then
      echo
      echo "**Error**: You must have \`$GETTEXTIZE' installed to compile $PKG_NAME."
      echo "Get $GETTEXTIZE_URL"
      echo "(or a newer version if it is available)"
      DIE=1
    fi
  fi
fi

if test "$DIE" -eq 1; then
	exit 1
fi

      if grep "^AM_GLIB_GNU_GETTEXT" $CONFIGURE >/dev/null; then
	if grep "sed.*POTFILES" $CONFIGURE >/dev/null; then
	  : do nothing -- we still have an old unmodified $CONFIGURE
	else
	  echo "Creating $dr/aclocal.m4 ..."
	  test -r $dr/aclocal.m4 || touch $dr/aclocal.m4
	  echo "Running glib-gettextize...  Ignore non-fatal messages."
	  echo "no" | glib-gettextize --force --copy
	  echo "Making $dr/aclocal.m4 writable ..."
	  test -r $dr/aclocal.m4 && chmod u+w $dr/aclocal.m4
        fi
      fi
      if grep "^IT_PROG_INTLTOOL" $CONFIGURE >/dev/null; then
        echo "Running intltoolize..."
	intltoolize --copy --force --automake
      fi
      if grep "^AM_PROG_LIBTOOL" $CONFIGURE >/dev/null; then
	echo "Running $LIBTOOLIZE..."
	$LIBTOOLIZE --force --copy
      fi
      echo "Running $ACLOCAL $aclocalinclude ..."
      $ACLOCAL $aclocalinclude
      if grep "^AM_CONFIG_HEADER" $CONFIGURE >/dev/null; then
	echo "Running $AUTOHEADER..."
	$AUTOHEADER
      fi
      echo "Running $AUTOMAKE --gnu $am_opt ..."
      $AUTOMAKE --add-missing --gnu $am_opt
      echo "Running $AUTOCONF ..."
      $AUTOCONF


# I don't like when this file gets automatically changed
if [ -f configure.in~ ]; then
    mv configure.in~ configure.in
fi

if [ -f Makefile.am~ ]; then
    mv Makefile.am~ Makefile.am
fi

#echo "libtoolize"
#libtoolize --force --copy --automake || exit 1

#echo "aclocal"
#$ACLOCAL $ACPATH -I . -I src/plugins/gadu_gadu/libgadu/m4 || exit 1

#echo "automake"
#$AUTOMAKE --no-force --copy --add-missing || exit 1

prev=`pwd`
cd src/plugins/gadu_gadu/libgadu
./autogen.sh

cd $prev
#./configure $*

#cd po
#make update-po
#cd ..
