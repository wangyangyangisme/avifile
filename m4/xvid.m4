# Configure paths for libxvidcore

dnl AM_PATH_XVID([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libxvidcore, and define XVID_CFLAGS and XVID_LIBS
dnl
AC_DEFUN([AM_PATH_XVID],
[
    AC_ARG_WITH(xvid_prefix, [  --with-xvid-prefix=PFX  where XviD is installed. (optional)],
		[], with_xvid_prefix="")

    AC_ARG_ENABLE(xvidtest, [  --disable-xvidtest      do not try to compile and run a test XviD program],
		  [], enable_xvidtest=yes)

    XVID_CFLAGS=""
    XVID_LIBS=""

    have_xvid=no
    ac_save_CPPFLAGS=$CPPFLAGS
    if test -n "$with_xvid_prefix" ; then
	dnl user has specified extra path for XviD instalation
	CPPFLAGS="-I$with_xvid_prefix $CPPFLAGS"
	AC_CHECK_HEADER(include/xvid.h,
			XVID_CFLAGS="-I$with_xvid_prefix/include"
			XVID_LIBS="-L$with_xvid_prefix/lib"
			have_xvid=yes)
	if test x$have_xvid = xno ; then
	    AC_CHECK_HEADER(xvid.h,
			    XVID_CFLAGS="-I$with_xvid_prefix"
			    XVID_LIBS="-L$with_xvid_prefix"
			    have_xvid=yes)
	fi
    else
	AC_CHECK_HEADER(xvid.h, have_xvid=yes)
    fi
    CPPFLAGS=$ac_save_CPPFLAGS
    XVID_LIBS="$XVID_LIBS -lxvidcore"

dnl
dnl Now check if the installed XviD is sufficiently new.
dnl
    if test x$have_xvid = xyes -a x$enable_xvidtest = xyes ; then
	AC_CACHE_CHECK([for xvid_init in -lxvidcore],
		       ac_cv_val_HAVE_XVID,
		       [ rm -f conf.xvidtest
		       ac_save_CFLAGS=$CFLAGS
		       ac_save_LIBS=$LIBS
		       CFLAGS="$CFLAGS $XVID_CFLAGS"
		       LIBS="$LIBS $XVID_LIBS"
		       AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xvid.h>

int main ()
{
  XVID_INIT_PARAM xinit;
  system("touch conf.xvidtest");

  xinit.cpu_flags = 0;
  xvid_init(NULL, 0, &xinit, NULL);

  if (xinit.api_version == API_VERSION) {
    return 0;
  } else {
    printf("Header file and library are out of sync. Header file supports\n"
	   "version %d.%d API and shared library supports version %d.%d API.\n",
	   API_VERSION >> 16, API_VERSION & 0xFFFF,
	   xinit.api_version >> 16, xinit.api_version & 0xFFFF);
    return 1;
  }
}		       
		       ],
		       [ ac_cv_val_HAVE_XVID=yes ],
		       [ ac_cv_val_HAVE_XVID=no ],
		       [ echo $ac_n "cross compiling; assumed OK... $ac_c" ])
		       CFLAGS=$ac_save_CFLAGS
		       LIBS=$ac_save_LIBS])
	test x$ac_cv_val_HAVE_XVID = xno && have_xvid=no
    fi

    if test x$have_xvid = xyes ; then
	ifelse([$1], , :, [$1])
    else
	if test ! -f conf.xvidtest ; then
	    AC_MSG_WARN([Could not run XviD test program, checking why...])
	    CFLAGS="$CFLAGS $XVID_CFLAGS"
	    LIBS="$LIBS $XVID_LIBS"
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
	XVID_CFLAGS=""
	XVID_LIBS=""
	ifelse([$2], , :, [$2])
    fi
    AC_SUBST(XVID_CFLAGS)
    AC_SUBST(XVID_LIBS)
    rm -f conf.xvidtest
])

