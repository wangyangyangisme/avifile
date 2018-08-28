#!/bin/sh
#
#set -x
acdir=""

# can't be made default - some automake will fail here - using only privately
# for snapshot creation
#use_copy="--copy"
use_copy=""
forceon=""
removeconf=""

while [ $# -gt 0 ]; do
	case "$1" in
		--acdir)
			acdir=$2
                        shift
			;;
		--force)
			forceon="force"
                        shift
			;;
		--copy)
			use_copy="--copy"
                        shift
			;;
		--linkac)
			test -h configure.ac || ln -sf configure.in configure.ac
                        ;;
		--clean)
			removeconf="yes"
			rm -f config.cache
                        ;;
		--clean-am)
                        find . -name autom4te.cache -print0 | xargs -0 rm -rf core
                        exit
                        ;;
                -h|--help)
                        echo "Usage: $0 [OPTIONS]"
                        echo
	                echo "Run all necesary programs to create configure files"
                        echo
	                echo "Options:"
	                echo "  --help    print this help, then exit"
	                echo "  --acdir   pass this parameter to aclocal (see aclocal --help for more details)"
	                echo "  --force   ignore FATAL error message"
	                echo "  --linkac  create symbolic link configure.in ->configure.ac"
	                echo "  --copy    use option --copy with automake and libtoolize"
	                echo "  --clean   remove configure script & config.cache (forcing its recreation)"
                        exit 0
                        ;;
		*)
			echo "WARNING: unrecognized option: $1 (see --help)"
			;;
	esac
	shift
done

echo "Generating build information..."

# Touch the timestamps on all the files since CVS messes them up
#directory=`dirname $0`
#touch $directory/configure.in
#touch configure.in


# Debian unstable allows to have several different versions of autoconf
# in the one system

use_automake=automake
use_aclocal=aclocal
use_libtool=libtool
use_autoconf=autoconf
use_autoheader=autoheader

# some freebsd compatibility lookups for prefered versions
findver=14
export findver
( which automake${findver} 2>/dev/null | grep -q "^/" ) && use_automake=`which automake${findver}`
( which aclocal${findver} 2>/dev/null | grep -q "^/" ) && use_aclocal=`which aclocal${findver}`
findver=213
( which autoconf${findver} 2>/dev/null | grep -q "^/" ) && use_autoconf=`which autoconf${findver}`
( which autoheader${findver} 2>/dev/null | grep -q "^/" ) && use_autoheader=`which autoheader${findver}`
unset findver

# I think links are now OK
# Debian will use 2.50 for configure.ac files automaticaly
#( which autoconf2.50 2>/dev/null | grep -q "^/" ) && use_autoconf=autoconf2.50
#( which autoheader2.50 2>/dev/null | grep -q "^/" ) && use_autoheader=autoheader2.50

DIE=0

($use_autoconf --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autoconf' installed ."
  DIE=1
}

($use_automake --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`automake' installed."
  DIE=1
  NO_AUTOMAKE=yes
}

($use_aclocal --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: Missing \`aclocal'."
  DIE=1
}

($use_libtool --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`libtool' installed."
  DIE=1
}

if test -z "$acdir" -a -d /usr/local/share/aclocal ; then
  if !(ls -ld /usr/local/share | grep /usr/share > /dev/null 2>/dev/null ); then
  if test "`$use_aclocal --print-ac-dir`" != "/usr/local/share/aclocal" ; then
    INCLUDESTRING="-I /usr/local/share/aclocal"
#    ACLOCAL_FLAGS=`echo $ACLOCAL_FLAGS | sed s/-I\ \/z//`
    ACLOCAL_FLAGS="-I /usr/local/share/aclocal $ACLOCAL_FLAGS"
    ACLOCAL_MAINDIR=`$use_aclocal --print-ac-dir`
    for FILENAME in `ls $ACLOCAL_MAINDIR`; do
    if test -e /usr/local/share/aclocal/$FILENAME ; then
	echo "FATAL: both $ACLOCAL_MAINDIR/$FILENAME and /usr/local/share/aclocal/$FILENAME exist; you must manually delete one of them"
	echo "or use: autogen.sh --acdir /usr/share/aclocal"
        if test -z "$forceon" ; then
                echo "if you still want to continue - use force option"
                echo "but do not expect it will work! - fix the problem"
                exit 0;
        fi
    fi
    done
  fi
  else
    echo /usr/local/share is link to /usr/share
  fi
fi

# Regenerate configuration files
ok=0

rm -f aclocal.m4 configure config.guess config.log config.sub ltmain.sh libtool ltconfig missing mkinstalldirs depcomp install-sh

# test if configure.in is newer the created configure script
testfornewer()
{
  test $1.in -nt $1 -o -n "$removeconf" && rm -f $1;
}

#testfornewer libmmxnow/configure
#testfornewer plugins/libmad/libmad/configure

echo "Running libtoolize..."
libtoolize --force $use_copy > /dev/null || ok=1

if test -d m4 ; then
	rm -f acinclude.m4; for i in m4/*.m4 ; do cat $i >> acinclude.m4 ; done
fi
if test -z "$acdir" ; then
	echo "Running aclocal $ACLOCAL_FLAGS..."
	$use_aclocal $ACLOCAL_FLAGS || ok=1
else
	echo "Running aclocal with forced acdir: $acdir"
	$use_aclocal --acdir=$acdir || ok=1
fi

# echo "Running autoupdate..."
# autoupdate || ok=1
echo "Running autoheader..."
$use_autoheader || ok=1
echo "Running autoconf..."
$use_autoconf || ok=1
echo "Running automake..."
$use_automake $use_copy --add-missing --foreign || ok=1

# as the automake is unable to pass make distcheck with
# am_edit perl script - it's disabled for now
#
# only invoke in the toplevel dir
if test -r admin/am_edit_XXX ; then
	# disable - XXX doesn't exists
	echo "Postprocessing automake generated Makefiles"
        # use only for dirs with Qt programs
	perl admin/am_edit --foreign-libtool --no-final \
	     libavqt/Makefile.in \
	     player/Makefile.in \
	     samples/artsplug/Makefile.in \
	     samples/qtrecompress/Makefile.in \
	     samples/qtvidcap/Makefile.in || ok=1
fi

if [ "${ok}" -eq 0 ]; then
	echo "Now you are ready to run ./configure"
	echo "  Please remove the file config.cache after changing your setup"
	echo "  so that configure will find the changes next time."
	echo "  To speedup configure process use option cache-file i.e.:"
        echo "      configure --cache-file=config.cache"
        if test -n "$acdir" ; then
		echo
		echo "  Note: you have to specify this configure option:"
		echo "        --with-acdir=$acdir"
		echo "        when configuring avifile"
        fi
    else
	echo "Problems detected. Please investigate."
	echo "Suggested version of used programs (try to use them)"
	echo "   libtool  1.4.1 or better"
	echo "   automake 1.4 or better (automake 1.6.0 is buggy!)"
	echo "   autoconf 2.52 or better"
	echo "Your installed version:"
        $use_libtool --version | head -1
        $use_automake --version | head -1
        $use_autoconf --version | head -1
        echo "Report aclocal = "
        $use_aclocal --print-ac-dir
        echo "Please report your problem on avifile-admin@prak.org"
        echo "with this log of build process together with system description."
fi
