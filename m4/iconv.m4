# Configure paths for  iconv

dnl Usage:
dnl AC_ICONV(MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for ares, and defines
dnl - ICONV_CFLAGS (compiler flags)
dnl - ICONV_LIBS (linker flags, stripping and path)
dnl prerequisites:

AC_DEFUN([AC_ICONV],
[
    dnl check it it's part of the libc
    AC_CHECK_LIB(c, iconv_open, ac_cv_have_iconv=yes, ac_cv_have_iconv=no)

    ICONV_CFLAGS=
    ICONV_LIBS=
    AC_ARG_WITH(iconv, [  --with-iconv=path       path for extra iconv library],
    [
	if test x$ac_cv_have_iconv = xno; then
	    ac_iconv_path=
	    case "$with_iconv" in
	    'yes') ac_iconv_path="/usr" ;;
	    'no') ;;
	    *) ac_iconv_path=$with_iconv ;;
	    esac
	    if test -n "$ac_iconv_path"; then
		ac_save_ICONVLIBS=$LIBS
		LIBS="-L$ac_iconv_path/lib $LIBS"
		AC_CHECK_LIB(iconv, iconv_open, ac_cv_have_iconv=yes,)
		if test x$ac_cv_have_iconv = xyes ; then
		    ICONV_LIBS="-liconv"
		    if test "$ac_iconv_path" != "/usr"; then
			ICONV_CFLAGS="-I$ac_iconv_path/include"
			ICONV_LIBS="$ac_iconv_path/lib $ICONV_LIBS"
		    fi
		fi
		LIBS=$ac_save_ICONVLIBS
	    fi
	fi
    ])
    
    if test x$ac_cv_have_iconv = xyes ; then
    	AC_MSG_CHECKING([for iconv declaration])
	AC_CACHE_VAL(ac_cv_proto_iconv, [
	AC_TRY_COMPILE([
#include <stdlib.h>
#include <iconv.h>
extern
#ifdef __cplusplus
"C"
#endif
#if defined(__STDC__) || defined(__cplusplus)
size_t iconv (iconv_t cd, char**, size_t*, char**, size_t*);
#else
size_t iconv();
#endif
], [],
	[ ac_cv_proto_iconv="" ], [ ac_cv_proto_iconv="const" ]) ])
	AC_DEFINE_UNQUOTED(ICONV_CONST_CAST, $ac_cv_proto_iconv, [Use casting for iconv's inputbuffer - don't ask me.])
	if test x$ac_cv_proto_iconv = xconst ; then
	    AC_MSG_RESULT([using const])
	else
	    AC_MSG_RESULT([not using const])
	fi
    	ifelse([$2], , :, [$2])
    else
	ICONV_CFLAGS=""
	ICONV_LIBS=""
	ifelse([$3], , :, [$3])
    fi

    AC_SUBST(ICONV_CFLAGS)
    AC_SUBST(ICONV_LIBS)
])
