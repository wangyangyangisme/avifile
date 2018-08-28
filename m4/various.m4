dnl    -*- shell-script -*-

dnl    This file is part of the Avifile packages
dnl    and has been heavily modified for its purposes
dnl    Copyright (C) 2002 Zdenek Kabelac (kabi@users.sourceforge.net)
dnl
dnl    Originaly this file was part of the KDE libraries/packages
dnl    Copyright (C) 1997 Janos Farkas (chexum@shadow.banki.hu)
dnl              (C) 1997 Stephan Kulow (coolo@kde.org)

dnl    This file is free software; you can redistribute it and/or
dnl    modify it under the terms of the GNU Library General Public
dnl    License as published by the Free Software Foundation; either
dnl    version 2 of the License, or (at your option) any later version.

dnl    This library is distributed in the hope that it will be useful,
dnl    but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl    Library General Public License for more details.

dnl    You should have received a copy of the GNU Library General Public License
dnl    along with this library; see the file COPYING.LIB.  If not, write to
dnl    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
dnl    Boston, MA 02111-1307, USA.


dnl ------------------------------------------------------------------------
dnl Find a file (or one of more files in a list of dirs)
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_FIND_FILE],
[
$3=NO
for i in $2;
do
  for j in $1;
  do
    if test -r "$i/$j"; then
      $3=$i
      break 2
    fi
  done
done
])

AC_DEFUN([KDE_MOC_ERROR_MESSAGE],
[
    AC_MSG_ERROR([No valid Qt meta object compiler (moc) found!
Please check whether you installed Qt correctly.
You need to have a running moc binary.
configure tried to run $ac_cv_path_moc and the test did not
succeed. If configure should not have tried this one, set
the environment variable MOC to the right one before running
configure.])
])

AC_DEFUN([KDE_UIC_ERROR_MESSAGE],
[
    AC_MSG_WARN([No valid Qt ui compiler (uic) found!
Please check whether you installed Qt correctly.
You need to have a running uic binary.
configure tried to run $ac_cv_path_uic and the test did not
succeed. If configure should not have tried this one, set
the environment variable UIC to the right one before running
configure.])
])

dnl ------------------------------------------------------------------------
dnl Find the meta object compiler in the PATH, in $QTDIR/bin, and some
dnl more usual places
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_PATH_QT_MOC_UIC],
[
dnl usually qt is installed in such a way it might help to
dnl try this binpath first
    cutlib_binpath=`echo "$ac_qt_libraries" | sed 's%/[[^/]]*$%%'`
    lc_qt_binpath="$ac_qt_binpath:$cutlib_binpath/bin:$PATH:/usr/local/bin:/usr/local/qt3/bin:/usr/local/qt2/bin:/usr/local/qt/bin:/usr/lib/qt3/bin:/usr/lib/qt2/bin:/usr/lib/qt/bin:/usr/bin:/usr/X11R6/bin"
    AC_PATH_PROGS(MOC, moc moc3 moc2, , $lc_qt_binpath)
    dnl AC_PATH_PROGS(UIC, uic, , $lc_qt_binpath)

    if test -z "$MOC"; then
        if test -n "$ac_cv_path_moc"; then
            output=`eval "$ac_cv_path_moc --help 2>&1 | sed -e '1q' | grep Qt"`
        fi
        echo "configure:__oline__: tried to call $ac_cv_path_moc --help 2>&1 | sed -e '1q' | grep Qt" >&AC_FD_CC
        echo "configure:__oline__: moc output: $output" >&AC_FD_CC

        if test -z "$output"; then
            KDE_MOC_ERROR_MESSAGE
        fi
    fi

    AC_SUBST(MOC)

    dnl We do not need UIC
    dnl [KDE_UIC_ERROR_MESSAGE])
    dnl AC_SUBST(UIC)
])


dnl AC_PATH_QT([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl ------------------------------------------------------------------------
dnl Try to find the Qt headers and libraries.
dnl $(QT_LIBS) will be -Lqtliblocation (if needed) -lqt_library_name
dnl and $(QT_CFLAGS) will be -Iqthdrlocation (if needed)
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_PATH_QT],
[
AC_REQUIRE([AC_PATH_XTRA])

ac_have_qt=yes
ac_qt_binpath=
ac_qt_includes=
ac_qt_libraries=
ac_qt_notfound=
qt_libraries=
qt_includes=
min_qt_version=ifelse([$1], ,100, $1)

AC_ARG_WITH(qt_prefix, [  --with-qt-prefix        where the root of Qt is installed.],
    [  ac_qt_includes="$withval"/include
       ac_qt_libraries="$withval"/lib
       ac_qt_binpath="$withval"/bin
    ])

AC_ARG_WITH(qt_dir, [  --with-qt-dir           obsoleted, use --with-qt-prefix!],
    [  ac_qt_includes="$withval"/include
       ac_qt_libraries="$withval"/lib
       ac_qt_binpath="$withval"/bin
       AC_MSG_WARN([
!!!
!!! Using obsoleted option: --with-qt-dir, instead use --with-qt-prefix
!!!])
    ])


test -n "$QTDIR" && ac_qt_binpath="$ac_qt_binpath:$QTDIR/bin:$QTDIR"

AC_ARG_WITH(qt_includes, [  --with-qt-includes      where the Qt includes are.],
    [ ac_qt_includes=$withval ])

AC_ARG_WITH(qt_libraries, [  --with-qt-libraries     where the Qt library is installed.],
    [ ac_qt_libraries=$withval ])

if test -z "$ac_qt_includes" || test -z "$ac_qt_libraries" ; then
    AC_CACHE_VAL(ac_cv_have_qt,
    [ # try to guess Qt locations
      # various paths from various distros - 
      # you are welcome to suggest another
	qt_incdirs="$QTINC
            /usr/local/lib/qt3/include
            /usr/local/lib/qt2/include
            /usr/local/qt/include
            /usr/local/include
            /usr/lib/qt-3.1/include
            /usr/lib/qt3/include
            /usr/lib/qt-2.3.1/include
            /usr/lib/qt2/include
            /usr/include/qt3
            /usr/include/qt
            /usr/lib/qt/include
            $x_includes/qt3
            $x_includes/qt2
            $x_includes/X11/qt
            $x_includes
            /usr/include"
	qt_libdirs="$QTLIB
            /usr/local/lib/qt3/lib
            /usr/local/lib/qt2/lib
            /usr/local/qt/lib
            /usr/local/lib
            /usr/lib/qt-3.1/lib
            /usr/lib/qt3/lib
            /usr/lib/qt-2.3.1/lib
            /usr/lib/qt2/lib
            /usr/lib/qt/lib
            /usr/lib/qt
            $x_libraries/qt3
            $x_libraries/qt2
            $x_libraries
            /usr/lib"

	if test -n "$QTDIR" ; then
            qt_incdirs="$QTDIR/include $QTDIR $qt_incdirs"
	    qt_libdirs="$QTDIR/lib $QTDIR $qt_libdirs"
        fi

	qt_incdirs="$ac_qt_includes $qt_incdirs"
	qt_libdirs="$ac_qt_libraries $qt_libdirs"

	AC_FIND_FILE(qmovie.h, $qt_incdirs, qt_incdir)
	ac_qt_includes="$qt_incdir"

	qt_libdir=NONE
	for dir in $qt_libdirs ; do
            for qtname in qt3 qt2 qt ; do
                try="ls -1 $dir/lib$qtname.* $dir/lib$qtname-mt.*"
                if test -n "`$try 2> /dev/null`"; then
                      test -z "$QTNAME" && QTNAME=$qtname
                      qt_libdir=$dir
                      break
                fi
            done
            if test x$qt_libdir != xNONE ; then
                break
            fi
	    echo "tried $dir" >&AC_FD_CC
        done

        ac_qt_libraries=$qt_libdir

	test -z "$QTNAME" && QTNAME=qt
        ac_QTNAME=$QTNAME

	ac_save_QTLIBS=$LIBS
	LIBS="-L$ac_qt_libraries $LIBS $PTHREAD_LIBS"
        AC_CHECK_LIB($QTNAME-mt, main, ac_QTNAME=$QTNAME-mt)
	LIBS=$ac_save_QTLIBS

	ac_cv_have_qt="ac_have_qt=yes ac_qt_includes=$ac_qt_includes ac_qt_libraries=$ac_qt_libraries ac_QTNAME=$ac_QTNAME"

        if test "$ac_qt_includes" = NO || test "$ac_qt_libraries" = NONE ; then
            ac_cv_have_qt="ac_have_qt=no"
            ac_qt_notfound="(headers)"
            if test "$ac_qt_includes" = NO; then
                if test "$ac_qt_libraries" = NONE; then
                    ac_qt_notfound="(headers and libraries)"
                fi
            else
                ac_qt_notfound="(libraries)";
            fi
        fi
    ])
    eval "$ac_cv_have_qt"
else
    for qtname in qt3 qt2 qt ; do
        try="ls -1 $ac_qt_libraries/lib$qtname.* $ac_qt_libraries/lib$qtname-mt.*"
        if test -n "`$try 2> /dev/null`"; then
            test -z "$QTNAME" && QTNAME=$qtname
            break
        fi
    done
    ac_QTNAME=$QTNAME
    ac_save_QTLIBS=$LIBS
    LIBS="-L$ac_qt_libraries $LIBS $PTHREAD_LIBS"
    AC_CHECK_LIB($QTNAME-mt, main, ac_QTNAME=$QTNAME-mt)
    LIBS=$ac_save_QTLIBS
fi

if test x$ac_have_qt = xyes ; then
    AC_MSG_CHECKING([for Qt library (version >= $min_qt_version)])
    AC_CACHE_VAL(ac_cv_qt_version,
    [
	AC_LANG_SAVE
	AC_LANG_CPLUSPLUS

	ac_save_QTCXXFLAGS=$CXXFLAGS
	ac_save_QTLIBS=$LIBS
	CXXFLAGS="$CXXFLAGS -I$ac_qt_includes"
	LIBS="-L$ac_qt_libraries -l$ac_QTNAME $X_LIBS $LIBS $PTHREAD_LIBS"
	AC_TRY_RUN([
	    /*#include <qapplication.h>*/
	    /*#include <qmovie.h>*/
	    #include <qstring.h>
	    #include <qglobal.h>
	    #include <stdio.h>

	    int main(int argc, char* argv[]) {
		/*QApplication a( argc, argv );*/
		/*QMovie m; int s = m.speed();*/
		QString qa("test");
		unsigned int v = QT_VERSION;
		FILE* f = fopen("conf.qttest", "w");
		if (v > 400) v = (((v >> 16) & 0xff) * 100) + (((v >> 8) & 0xff) * 10) + (v & 0xff);
		if (f) fprintf(f, "%d\n", v);
		return 0;
	    }
	],
	[ AC_MSG_RESULT(yes); ac_cv_qt_version=`cat conf.qttest`; rm -f conf.qttest ],
	[ AC_MSG_RESULT(no); ac_have_qt=no; ac_cv_qt_version=ERROR ],
	[ echo $ac_n "cross compiling; assumed OK... $ac_c" ])

	if test x$ac_cv_qt_version = xERROR ; then
	    AC_MSG_WARN([
*** Could not run Qt test program, checking why...
*** Configure discovered/uses these settings:
*** Qt libraries: $ac_qt_libraries
*** Qt headers: $ac_qt_includes
*** Note:
***    Compilation of Qt utilities also might be turned off (if not wanted).
***    If you are experiencing problems which will not be described
***    bellow please report then on 'avifile@prak.org' mailing list
***    (i.e. some misdetection or omitted path)]
             )
             AC_TRY_LINK([ #include <qstring.h> ],
		     [ QString qa("test") ],
                     [ AC_MSG_ERROR([
*** Qt test program compiled, but did not run. This usually means
*** that the run-time linker is not finding Qt library or finding the wrong
*** version of Qt. If it is not finding Qt, you will need to set your
*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point
*** to the installed location  Also, make sure you have run ldconfig if that
*** is required on your system.
***
*** If you have an old version installed, it is best to remove it, although
*** you may also be able to get things to work by modifying LD_LIBRARY_PATH
***
*** i.e. bash> export LD_LIBRARY_PATH=$ac_qt_libraries:\$LD_LIBRARY_PATH]) ],
                     [ AC_MSG_ERROR([
*** The test program failed to compile or link. See the file config.log for the
*** exact error that occured. This usually means Qt was incorrectly installed
*** or that you have moved Qt since it was installed. In the latter case, you
*** may want to set QTDIR shell variable
***
*** Another possibility is you try to link Qt libraries compiled with
*** different version of g++. Unfortunately you can not mix C++ libraries
*** and object files created with different C++ compiler
*** i.e. g++-2.96 libraries and g++-2.95 objects
*** The most common case: Some users seems to be downgrading their
*** compiler without thinking about consequencies...]) ]
	    )
	fi

	CXXFLAGS=$ac_save_QTCXXFLAGS
	LIBS=$ac_save_QTLIBS
	AC_LANG_RESTORE
     ])
     qt_version=$ac_cv_qt_version
fi

if test x$ac_have_qt != xyes; then
    AC_MSG_WARN([
*** Could not find usable Qt $ac_qt_notfound on your system!
*** If it _is_ installed, delete ./config.cache and re-run ./configure,
*** specifying path to Qt headers and libraries in configure options.
*** Switching off Qt compilation!])
    ac_have_qt=no
else
    AC_MSG_RESULT([found $ac_QTNAME version $qt_version, libraries $ac_qt_libraries, headers $ac_qt_includes])
    if test $min_qt_version -le $qt_version ; then
        qt_libraries=$ac_qt_libraries
        qt_includes=$ac_qt_includes

        if test "$qt_includes" = "$x_includes" -o -z "$qt_includes" ; then
            QT_CFLAGS=
        else
            QT_CFLAGS="-I$qt_includes"
            all_includes="$QT_CFLAGS $all_includes"
        fi
	QT_CFLAGS="$QT_CFLAGS -DQT_THREAD_SUPPORT"

        if test "$qt_libraries" = NONE -o "$qt_libraries" = "$x_libraries" -o -z "$qt_libraries" -o "$qt_libraries" = "/usr/lib" ; then
            QT_LIBS=
        else
            QT_LIBS="-L$qt_libraries"
            all_libraries="$QT_LIBS $all_libraries"
        fi

        QT_LIBS="$QT_LIBS -l$ac_QTNAME"

        if test x$ac_have_qt = xyes ; then
            AC_PATH_QT_MOC_UIC
        fi

        AC_SUBST(qt_version)
        AC_SUBST(QT_CFLAGS)
        AC_SUBST(QT_LIBS)
    else
        AC_MSG_WARN([
*** Unsupported old version of Qt library found. Please upgrade.])
        ac_have_qt=no
    fi
fi

if test x$ac_have_qt = xyes ; then
    ifelse([$2], , :, [$2])
else
    ifelse([$3], , :, [$3])
fi

])


AC_DEFUN([AC_FIND_ZLIB],
[
    AC_CACHE_CHECK([for libz], ac_cv_lib_z,
		    [ac_save_LIBS=$LIBS
		    LIBS="$LIBS -lz"
		    AC_TRY_LINK([#include<zlib.h>],
			    [return (zlibVersion() == ZLIB_VERSION); ],
			    [ ac_cv_lib_z=yes ], [ ac_cv_lib_z=no ])
		    LIBS=$ac_save_LIBS])
    AC_SUBST(Z_LIBS)
    if test x$ac_cv_lib_z = xyes ; then
	Z_LIBS="-lz"
	have_zlib=yes
	ifelse([$1], , :, [$1])
    else
	Z_LIBS=
	have_zlib=no
	ifelse([$2], , :, [$2])
    fi
])


AC_DEFUN([AC_FIND_PNG],
[
AC_REQUIRE([AC_FIND_ZLIB])
AC_MSG_CHECKING([for libpng])
AC_CACHE_VAL(ac_cv_lib_png,
[ac_save_LIBS="$LIBS"
LIBS="$all_libraries -lpng $LIBZ -lm -lX11 $LIBSOCKET"
AC_TRY_LINK([#include<png.h>],
    [
    png_structp png_ptr = png_create_read_struct(  // image ptr
		PNG_LIBPNG_VER_STRING, 0, 0, 0 );
    return( png_ptr != 0 );
    ],
    eval "ac_cv_lib_png='-lpng $LIBZ -lm'",
    eval "ac_cv_lib_png=no")
    LIBS=$ac_save_LIBS
])
if eval "test ! \"`echo $ac_cv_lib_png`\" = no"; then
  AC_DEFINE_UNQUOTED(HAVE_LIBPNG)
  LIBPNG="$ac_cv_lib_png"
  AC_SUBST(LIBPNG)
  AC_MSG_RESULT($ac_cv_lib_png)
else
  AC_MSG_RESULT(no)
  LIBPNG=""
  AC_SUBST(LIBPNG)
fi
])


dnl just a wrapper to clean up configure.in
AC_DEFUN([KDE_PROG_LIBTOOL],
[
AC_REQUIRE([AM_ENABLE_SHARED])
AC_REQUIRE([AM_ENABLE_STATIC])
dnl libtool is only for C, so I must force him
dnl to find the correct flags for C++
ac_save_cc=$CC
ac_save_cflags="$CFLAGS"
CC=$CXX
CFLAGS="$CXXFLAGS"
dnl AM_PROG_LIBTOOL dnl for libraries
CC=$ac_save_cc
CFLAGS="$ac_save_cflags"
])


dnl Check for the type of the third argument of getsockname
AC_DEFUN([AC_CHECK_KSIZE_T],
[AC_MSG_CHECKING(for the third argument of getsockname)
AC_LANG_CPLUSPLUS
AC_CACHE_VAL(ac_cv_ksize_t,
[AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],[
socklen_t a=0;
getsockname(0,(struct sockaddr*)0, &a);
],
ac_cv_ksize_t=socklen_t,
ac_cv_ksize_t=)
if test -z "$ac_cv_ksize_t"; then
ac_save_cxxflags="$CXXFLAGS"
if test "$GCC" = "yes"; then
  CXXFLAGS="-Werror $CXXFLAGS"
fi
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],[
int a=0;
getsockname(0,(struct sockaddr*)0, &a);
],
ac_cv_ksize_t=int,
ac_cv_ksize_t=size_t)
CXXFLAGS=$ac_save_cxxflags
fi
])

if test -z "$ac_cv_ksize_t"; then
  ac_cv_ksize_t=int
fi

AC_MSG_RESULT($ac_cv_ksize_t)
AC_DEFINE_UNQUOTED(ksize_t, $ac_cv_ksize_t)

])



# Search path for a program which passes the given test.
# Ulrich Drepper <drepper@cygnus.com>, 1996.

# serial 1
# Stephan Kulow: I appended a _KDE against name conflicts

dnl AM_PATH_PROG_WITH_TEST_KDE(VARIABLE, PROG-TO-CHECK-FOR,
dnl   TEST-PERFORMED-ON-FOUND_PROGRAM [, VALUE-IF-NOT-FOUND [, PATH]])
AC_DEFUN([AM_PATH_PROG_WITH_TEST_KDE],
[# Extract the first word of "$2", so it can be a program name with args.
    set dummy $2; ac_word=[$]2
    AC_MSG_CHECKING([for $ac_word])
    AC_CACHE_VAL(ac_cv_path_$1,
        [case "[$]$1" in
          /*) ac_cv_path_$1="[$]$1" # Let the user override the test with a path.
              ;;
          *)
              IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:"
              for ac_dir in ifelse([$5], , $PATH, [$5]); do
                test -z "$ac_dir" && ac_dir=.
                if test -f $ac_dir/$ac_word; then
                  if [$3]; then
                    ac_cv_path_$1="$ac_dir/$ac_word"
                    break
                  fi
                fi
              done
              IFS="$ac_save_ifs"
              dnl If no 4th arg is given, leave the cache variable unset,
              dnl so AC_PATH_PROGS will keep looking.
              ifelse([$4], , , [  test -z "[$]ac_cv_path_$1" && ac_cv_path_$1="$4"
                    ])
              ;;
         esac
        ])
    $1="$ac_cv_path_$1"
    if test -n "[$]$1"; then
      AC_MSG_RESULT([$]$1)
    else
      AC_MSG_RESULT(no)
    fi
    AC_SUBST($1)dnl
])



AC_DEFUN([AM_DISABLE_LIBRARIES],
[
    AC_PROVIDE([AM_ENABLE_STATIC])
    AC_PROVIDE([AM_ENABLE_SHARED])
    enable_static=no
    enable_shared=no
])






# Check whether LC_MESSAGES is available in <locale.h>.
# Ulrich Drepper <drepper@cygnus.com>, 1995.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU General Public
# License or the GNU Library General Public License but which still want
# to provide support for the GNU gettext functionality.
# Please note that the actual code of the GNU gettext library is covered
# by the GNU Library General Public License, and the rest of the GNU
# gettext package package is covered by the GNU General Public License.
# They are *not* in the public domain.

# serial 2

AC_DEFUN([AM_LC_MESSAGES],
[
    if test $ac_cv_header_locale_h = yes; then
    AC_CACHE_CHECK([for LC_MESSAGES], am_cv_val_LC_MESSAGES,
      [AC_TRY_LINK([#include <locale.h>], [return LC_MESSAGES],
       am_cv_val_LC_MESSAGES=yes, am_cv_val_LC_MESSAGES=no)])
    if test $am_cv_val_LC_MESSAGES = yes; then
      AC_DEFINE(HAVE_LC_MESSAGES, 1,
        [Define if your <locale.h> file defines LC_MESSAGES.])
    fi
  fi
])


dnl AM_PATH_LINUX([DEFAULT PATH, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl define LINUX_PATH and LINUX_CFLAGS
AC_DEFUN([AM_PATH_LINUX],
[
    AC_ARG_WITH(linux_prefix, [  --with-linux-prefix=PFX where are linux sources (=/usr/src/linux)],
	        [], with_linux_prefix=ifelse([$1], [], /usr/src/linux, $1))
    if test -f $with_linux_prefix/include/linux/modversions.h ; then
	LINUX_CFLAGS="-D__KERNEL__ -DMODULE -I$with_linux_prefix/include -include $with_linux_prefix/include/linux/modversions.h"
	LINUX_PREFIX=$with_linux_prefix
    else
        LINUX_CFLAGS=""
        LINUX_PREFIX=""
    fi
    AC_SUBST(LINUX_CFLAGS)
    AC_SUBST(LINUX_PREFIX)

    if test -n "$LINUX_PREFIX"; then
        ifelse([$2], [], [:], [$2])
    else
        ifelse([$3], [], [:], [$3])
    fi
])

