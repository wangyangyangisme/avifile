#!/bin/sh

# last_cvs_update=`date -r CVS/Entries +%y%m%d-%H:%M 2>/dev/null`

# this works in zsh
# ls -atr **/CVS/Entries | tail -1 | xargs date -r

# !!!!
# This file is no longer used by avifile project
# !!!!

AVIFILE_MAJOR_VERSION=0
AVIFILE_MINOR_VERSION=7
AVIFILE_MICRO_VERSION=16

if test "$1" != "NOHEADER" ; then
	last_cvs_update=`find . -name Entries -printf '%Ty%Tm%Td-%TH:%TM\n' | sort | tail -1`
	if test $? -ne 0 -o -z "$last_cvs_update" ; then
		# probably no gnu date installed(?), use current date
		last_cvs_update=`date +%y%m%d-%H:%M`
	fi

	VFL=/tmp/version$RANDOM
	BUILD="CVS-${last_cvs_update}-$1"

	echo "#ifndef AVIFILE_VERSION
#define AVIFILE_MAJOR_VERSION $AVIFILE_MAJOR_VERSION
#define AVIFILE_MINOR_VERSION $AVIFILE_MINOR_VERSION
#define AVIFILE_PATCHLEVEL $AVIFILE_MICRO_VERSION
#define AVIFILE_VERSION  ((AVIFILE_MAJOR_VERSION << 8 + AVIFILE_MINOR_VERSION) << 8 + AVIFILE_PATCHLEVEL)
#define AVIFILE_BUILD \"$BUILD\"
#endif" >$VFL

	# avoid unnecessary timestamp modification
	diff -q $VFL include/version.h >/dev/null 2>&1
	if test $? -ne 0 ; then
		cp -p $VFL include/version.h
	fi
	rm -f $VFL
	echo $BUILD
fi
