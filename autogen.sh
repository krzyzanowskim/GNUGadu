#!/bin/bash

# chujowe, ale ja jestem lamer i nie wiem jak to teraz zrobic ladniej
ACPATH1="/usr/share/aclocal";
ACPATH2="/opt/gnome2/share/aclocal";
DIE=0

if [ -d $ACPATH1 ]; then
    ACPATH="-I $ACPATH1"
fi

if [ -d $ACPATH2 ]; then
    ACPATH="$ACPATH -I $ACPATH2"
fi


have_libtool=false
if libtoolize --version < /dev/null > /dev/null 2>&1 ; then
    libtool_version=`libtoolize --version | sed 's/^[^0-9]*\([0-9.][0-9.]*\).*/\1/'`
    case $libtool_version in
	1.4*)
	    have_libtool=true
	    ;;
	1.5*)
	    have_libtool=true
	    ;;
    esac
fi
if $have_libtool ; then : ; else
        echo
        echo "You must have libtool 1.4 installed to compile $PROJECT."
        echo "Install the appropriate package for your distribution,"
        echo "or get the source tarball at http://ftp.gnu.org/gnu/libtool/"
        DIE=1
fi

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have autoconf installed to compile $PROJECT."
        echo "Install the appropriate package for your distribution,"
        echo "or get the source tarball at http://ftp.gnu.org/gnu/autoconf/"
        DIE=1
}

if automake-1.7 --version < /dev/null > /dev/null 2>&1 ; then
    AUTOMAKE=automake-1.7
    ACLOCAL=aclocal-1.7
else
        echo
        echo "You must have automake 1.7.x installed to compile $PROJECT."
        echo "Install the appropriate package for your distribution,"
        echo "or get the source tarball at http://ftp.gnu.org/gnu/automake/"
	echo "\n"
	echo "trying to automake anyway but YOU WERE WARNED"
	AUTOMAKE=automake
    	ACLOCAL=aclocal
fi


if test "$DIE" -eq 1; then 
        exit 1
fi

echo "intltoolize"
intltoolize --force --automake

echo "aclocal"
$ACLOCAL $ACPATH || exit 1

echo "libtoolize"
libtoolize --force --copy --automake || exit 1

echo "automake"
$AUTOMAKE --add-missing || exit 1

echo "autoconf"
autoconf || exit 1

./configure $*

#cd po
#make update-po
#cd ..
