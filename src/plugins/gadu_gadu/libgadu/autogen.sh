#!/bin/sh
if test "$*"; then
	ARGS="$*"
else
	test -f config.log && ARGS=`grep '^  \$ \./configure ' config.log | sed 's/^  \$ \.\/configure //' 2> /dev/null`
fi

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

libtoolize --force --copy --automake || exit 1

aclocal -I m4 || exit 1
autoheader || exit 1

$AUTOMAKE --no-force --copy --add-missing --foreign || exit 1

autoconf || exit 1
#test x$NOCONFIGURE = x && echo "Running ./configure $ARGS" && ./configure $ARGS

