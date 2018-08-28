# Configure paths for libxvidcore version 1.0

dnl AM_PATH_XVID([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libxvidcore, and define XVID4_CFLAGS and XVID4_LIBS
dnl
AC_DEFUN([AM_PATH_XVID4],
[
    AC_ARG_WITH(xvid4_prefix, [  --with-xvid4-prefix=PFX where XviD4 is installed. (optional)],
		[], with_xvid4_prefix="")

    AC_ARG_ENABLE(xvid4test, [  --disable-xvid4test     do not try to compile and run a test XviD4 program],
		  [], enable_xvid4test=yes)

    XVID4_CFLAGS=""
    XVID4_LIBS=""

    have_xvid4=no
    ac_save_CPPFLAGS=$CPPFLAGS
    if test -n "$with_xvid4_prefix" ; then
	dnl user has specified extra path for XviD instalation
	CPPFLAGS="-I$with_xvid4_prefix $CPPFLAGS"
	AC_CHECK_HEADER(include/xvid.h,
			XVID4_CFLAGS="-I$with_xvid4_prefix/include"
			XVID4_LIBS="-L$with_xvid4_prefix/lib"
			have_xvid4=yes)
	if test x$have_xvid4 = xno ; then
	    AC_CHECK_HEADER(xvid.h,
			    XVID4_CFLAGS="-I$with_xvid4_prefix"
			    XVID4_LIBS="-L$with_xvid4_prefix"
			    have_xvid4=yes)
	fi
    else
	AC_CHECK_HEADER(xvid.h, have_xvid4=yes)
    fi
    CPPFLAGS=$ac_save_CPPFLAGS
    XVID4_LIBS="$XVID4_LIBS -lxvidcore"

dnl
dnl Now check if the installed XviD is sufficiently new.
dnl
    if test x$have_xvid4 = xyes -a x$enable_xvid4test = xyes ; then
	AC_CACHE_CHECK([for xvid_global in -lxvidcore (4)],
		       ac_cv_val_HAVE_XVID4,
		       [ rm -f conf.xvidtest
		       ac_save_CFLAGS=$CFLAGS
		       ac_save_LIBS=$LIBS
		       CFLAGS="$CFLAGS $XVID4_CFLAGS"
		       LIBS="$LIBS $XVID4_LIBS"
		       AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xvid.h>

int main ()
{
  xvid_gbl_info_t xinfo;
  system("touch conf.xvidtest");

  memset(&xinfo, 0, sizeof(xinfo));
  xinfo.version = XVID_VERSION;

  if (xvid_global(NULL, XVID_GBL_INIT, &xinfo, NULL) == XVID_ERR_FAIL) {
    printf("Header file and library are out of sync. Header file supports\n"
	   "version %d.%d API and shared library supports version %d.%d API.\n",
	   XVID_VERSION >> 16, XVID_VERSION & 0xFFFF,
	   xinfo.version >> 16, xinfo.version & 0xFFFF);
    return 1;
  } else 
    return 0;
}
		       ],
		       [ ac_cv_val_HAVE_XVID4=yes ],
		       [ ac_cv_val_HAVE_XVID4=no ],
		       [ echo $ac_n "cross compiling; assumed OK... $ac_c" ])
		       CFLAGS=$ac_save_CFLAGS
		       LIBS=$ac_save_LIBS])
	test x$ac_cv_val_HAVE_XVID4 = xno && have_xvid4=no
    fi

    if test x$have_xvid4 = xyes ; then
	ifelse([$1], , :, [$1])
    else
	if test ! -f conf.xvidtest ; then
	    AC_MSG_WARN([Could not run XviD4 test program, checking why...])
	    CFLAGS="$CFLAGS $XVID4_CFLAGS"
	    LIBS="$LIBS $XVID4_LIBS"
	    AC_TRY_LINK([
#include <stdio.h>
#include <xvid.h>
], [ return 0 ], [ 
AC_MSG_RESULT([*** The test program compiled, but did not run. This usually means
*** that the run-time linker is not finding XviD or finding the wrong
*** version of XviD. If it is not finding XviD, you'll need to set your
*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point
*** to the installed location  Also, make sure you have run ldconfig if that
*** is required on your system
***
*** If you have an old version installed, it is best to remove it, although
*** you may also be able to get things to work by modifying LD_LIBRARY_PATH])], [
AC_MSG_RESULT([*** The test program failed to compile or link. See the file config.log for the
*** exact error that occured. This usually means XviD was incorrectly installed.])])
	    CFLAGS="$ac_save_CFLAGS"
	    LIBS="$ac_save_LIBS"
	fi
	XVID4_CFLAGS=""
	XVID4_LIBS=""
	ifelse([$2], , :, [$2])
    fi
    AC_SUBST(XVID4_CFLAGS)
    AC_SUBST(XVID4_LIBS)
    rm -f conf.xvidtest
])

