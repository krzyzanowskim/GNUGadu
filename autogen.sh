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
    if automake-1.8 --version < /dev/null > /dev/null 2>&1 ; then
	AUTOMAKE=automake-1.8
        ACLOCAL=aclocal-1.8
    else
        echo
        echo "You must have at least automake 1.7.x installed to compile $PROJECT."
        echo "Install the appropriate package for your distribution,"
        echo "or get the source tarball at http://ftp.gnu.org/gnu/automake/"
	echo "\n"
	echo "trying to automake anyway but YOU WERE WARNED"
	AUTOMAKE=automake
    	ACLOCAL=aclocal
    fi
fi

if test "$DIE" -eq 1; then 
        exit 1
fi

echo "gettextize"
#gettextize --force --copy --no-changelog || exit 1
# comment the next line when you have the system without NLS
if ! gettextize --version | grep -q '0\.10\.' ; then
    gettextize -c -f --intl --no-changelog
else
    gettextize -c -f
fi
# I don't like when this file gets automatically changed
if [ -f configure.in~ ]; then
    mv configure.in~ configure.in
fi

if [ -f Makefile.am~ ]; then
    mv Makefile.am~ Makefile.am
fi

echo "libtoolize"
libtoolize --force --copy --automake || exit 1

echo "aclocal"
$ACLOCAL $ACPATH -I m4 || exit 1

echo "automake"
$AUTOMAKE --copy --add-missing || exit 1

echo "autoconf"
autoconf || exit 1

./configure $*

#cd po
#make update-po
#cd ..
