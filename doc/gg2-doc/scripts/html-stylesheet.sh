#!/bin/bash


if [ -d $1 ]; then
	cd $1

	for file in *.html; do 
		sed -e 's:><META:><link rel="STYLESHEET" href="docbook.css"><meta http-equiv="Content-type" content="text/html; charset=utf-8"><META:g' $file > tmp
		sed -e 's:SRC="/usr/share/sgml.*/:SRC="../images/:g' tmp > tmp2
		rm tmp
		mv tmp2 $file 
	done
else
	sed -e 's:><META:><link rel="STYLESHEET" href="docbook.css"><meta http-equiv="Content-type" content="text/html; charset=utf-8"><META:g' $1 > tmp
	sed -e 's:SRC="/usr/share/sgml.*/:SRC="../images/:g' tmp > tmp2
	rm tmp
	mv tmp2 $1
fi

