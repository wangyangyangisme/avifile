# Configure paths for libfaad

dnl AM_PATH_FAAD([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libfaad, and define FAAD_CFLAGS and FAAD_LIBS
dnl
AC_DEFUN([AM_PATH_FAAD],
[
    AC_ARG_WITH(faad_prefix, [  --with-faad-prefix=PFX  where FAAD is installed. (optional)],
		[], with_faad_prefix="")

    AC_ARG_ENABLE(faadtest, [  --disable-faadtest      do not try to compile and run a test FAAD program],
		  [], enable_faadtest=yes)

    FAAD_CFLAGS=""
    FAAD_LIBS=""

    have_faad=no
    ac_save_CPPFLAGS=$CPPFLAGS
    if test -n "$with_faad_prefix" ; then
	dnl user has specified extra path for FAAD instalation
	CPPFLAGS="-I$with_faad_prefix $CPPFLAGS"
	AC_CHECK_HEADER(faad.h,
			FAAD_CFLAGS="-I$with_faad_prefix"
			FAAD_LIBS="-L$with_faad_prefix"
			have_faad=yes)
    else
	AC_CHECK_HEADER(faad.h, have_faad=yes)
    fi
    CPPFLAGS=$ac_save_CPPFLAGS

dnl
dnl Now check if the installed FAAD is sufficiently new.
dnl
    if test x$have_faad = xyes ; then
	FAAD_LIBS="$FAAD_LIBS -lfaad -lm"
	if test x$enable_faadtest = xyes ; then
	    AC_CACHE_CHECK([for faacDecOpen in -lfaad],
			  ac_cv_val_HAVE_FAAD,
			  [ rm -f conf.faadtest
			  ac_save_CFLAGS=$CFLAGS
			  ac_save_LIBS=$LIBS
			  CFLAGS="$CFLAGS $FAAD_CFLAGS"
			  LIBS="$LIBS $FAAD_LIBS"
			  AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <faad.h>

int main ()
{
	faacDecHandle h = faacDecOpen();
	system("touch conf.faadtest");
	return 0;
}			  ],
			  [ ac_cv_val_HAVE_FAAD=yes ],
			  [ ac_cv_val_HAVE_FAAD=no ],
			  [ echo $ac_n "cross compiling; assumed OK... $ac_c" ])
			  CFLAGS=$ac_save_CFLAGS
			  LIBS=$ac_save_LIBS])
	    test x$ac_cv_val_HAVE_FAAD = xno && have_faad = xno
	fi
     fi

     if test x$have_faad = xyes ; then
	 ifelse([$1], , :, [$1])
     else
	 if test -n "$FAAD_LIBS" -a x$enable_faadtest = xyes -a ! -f conf.faadtest ; then
		    AC_MSG_WARN([Could not run FAAD test program, checking why...])
		    CFLAGS="$CFLAGS $FAAD_CFLAGS"
		    LIBS="$LIBS $FAAD_LIBS"
		    AC_TRY_LINK([
#include <stdio.h>
#include <faad.h>
], [ return 0 ], [
AC_MSG_RESULT([*** The test program compiled, but did not run. This usually means
*** that the run-time linker is not finding FAAD or finding the wrong
*** version of FAAD. If it is not finding FAAD, you'll need to set your
*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point
*** to the installed location  Also, make sure you have run ldconfig if that
*** is required on your system
***
*** If you have an old version installed, it is best to remove it, although
*** you may also be able to get things to work by modifying LD_LIBRARY_PATH])], [
AC_MSG_RESULT([*** The test program failed to compile or link. See the file config.log for the
*** exact error that occured. This usually means FAAD was incorrectly installed.])])
		CFLAGS="$ac_save_CFLAGS"
		LIBS="$ac_save_LIBS"
	fi
	FAAD_CFLAGS=""
	FAAD_LIBS=""
	ifelse([$2], , :, [$2])
    fi
    AC_SUBST(FAAD_CFLAGS)
    AC_SUBST(FAAD_LIBS)
    rm -f conf.faadtest
])

