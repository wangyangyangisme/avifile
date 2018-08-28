# Configure paths for DivX

dnl Usage:
dnl AM_PATH_DIVX([ACTION-IF-FOUND-DECORE [, ACTION-IF-NOT-FOUND-DECORE
dnl		 [, ACTION-IF-FOUND-ENCORE, [ACTION-IF-NOT-FOUND-DECORE]]]] )
dnl Test for libdivxdecore, libdivxencore, and define XVID_CFLAGS and XVID_LIBS
dnl
AC_DEFUN([AM_PATH_DIVX],
[
    AC_ARG_WITH(divx4_prefix, [  --with-divx4-prefix=PFX where divx4linux is installed. (optional)],
	    [], with_divx4_prefix=)

    have_divx_decore=no
    have_divx_encore=no
    DIVX_CFLAGS=""
    DIVX_LIBS=""
    save_CPPFLAGS=$CPPFLAGS
    if test -n "$with_divx4_prefix" ; then
	dnl user has specified extra path for DivX4 instalation
	CPPFLAGS="-I$with_divx4_prefix $CPPFLAGS"
	AC_CHECK_HEADER(include/decore.h,
			DIVX_CFLAGS="-I$with_divx4_prefix/include"
			DIVX_LIBS="-L$with_divx4_prefix/lib"
			have_divx_decore=yes)
	if test x$have_divx_decore = xno ; then
	    CPPFLAGS="-I$with_divx4_prefix $CPPFLAGS"
	    AC_CHECK_HEADER(decore.h,
			    DIVX_CFLAGS="-I$with_divx4_prefix"
			    DIVX_LIBS="-L$with_divx4_prefix"
			    have_divx_decore=yes)
	fi
    else
	AC_CHECK_HEADER(decore.h, have_divx_decore=yes)
    fi

    AC_CHECK_HEADER(encore2.h, have_divx_encore=yes)
    CPPFLAGS=$save_CPPFLAGS

    if test x$have_divx_decore = xyes ; then
	AC_CHECK_LIB(divxdecore, decore,
		     DIVX_LIBS="$DIVX_LIBS -ldivxdecore",
		     have_divx_decore=no, $DIVX_LIBS)
    fi

    if test x$have_divx_encore = xyes ; then
	AC_CHECK_LIB(divxencore, encore,
		     DIVX_LIBS="$DIVX_LIBS -ldivxencore",
		     have_divx_encore=no, $DIVX_LIBS)
    fi

    if test x$have_divx_decore = xyes ; then
        ifelse([$1], , :, [$1])
    else
        ifelse([$2], , :, [$2])
    fi

    if test x$have_divx_encore = xyes ; then
        ifelse([$3], , :, [$3])
    else
        ifelse([$4], , :, [$4])
    fi

    AC_SUBST(DIVX_CFLAGS)
    AC_SUBST(DIVX_LIBS)
])

