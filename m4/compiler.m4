dnl AC_GCC_VERSION
dnl check for compiler version
dnl sets COMPILER_VERSION and GCC_VERSION

AC_DEFUN([AC_CC_VERSION],
[
    AC_MSG_CHECKING([C compiler version])
    COMPILER_VERSION=`$CC -v 2>&1 | grep version`
    case $COMPILER_VERSION in
        *gcc*)
	   dnl Ok, how to turn $3 into the real $3
	   GCC_VERSION=`echo $COMPILER_VERSION | \
	   sed -e 's/[[^ ]]*\ [[^ ]]*\ \([[^ ]]*\)\ .*/\1/'` ;;
	*) GCC_VERSION=unknown ;;
    esac
    AC_MSG_RESULT([$GCC_VERSION])
])

dnl AC_TRY_CFLAGS (CFLAGS, [ACTION-IF-WORKS], [ACTION-IF-FAILS])
dnl check if $CC supports a given set of cflags

AC_DEFUN([AC_TRY_CFLAGS],
[
    AC_MSG_CHECKING([if $CC supports $1 flag(s)])
    ac_save_CFLAGS=$CFLAGS
    CFLAGS=$1
    AC_TRY_COMPILE([], [], [ ac_cv_try_cflags_ok=yes ],[ ac_cv_try_cflags_ok=no ])
    CFLAGS=$ac_save_CFLAGS
    AC_MSG_RESULT([$ac_cv_try_cflags_ok])
    if test x$ac_cv_try_cflags_ok = xyes; then
        ifelse([$2], [], [:], [$2])
    else
        ifelse([$3], [], [:], [$3])
    fi
])

dnl AC_TRY_CXXFLAGS (CXXFLAGS, [ACTION-IF-WORKS], [ACTION-IF-FAILS])
dnl check if $CXX supports a given set of cxxflags

AC_DEFUN([AC_TRY_CXXFLAGS],
[
    AC_MSG_CHECKING([if $CXX supports $1 flag(s)])
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    ac_save_CXXFLAGS=$CXXFLAGS
    CXXFLAGS=$1
    AC_TRY_COMPILE([], [], [ ac_cv_try_cxxflags_ok=yes ],[ ac_cv_try_cxxflags_ok=no ])
    CXXFLAGS=$ac_save_CXXFLAGS
    AC_MSG_RESULT([$ac_cv_try_cxxflags_ok])
    AC_LANG_RESTORE
    if test x$ac_cv_try_cxxflags_ok = xyes; then
        ifelse([$2], [], [:], [$2])
    else
        ifelse([$3], [], [:], [$3])
    fi
])

dnl AC_CHECK_GNU_EXTENSIONS([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])

AC_DEFUN([AC_CHECK_GNU_EXTENSIONS],
[
    AC_CACHE_CHECK([if you need GNU extensions], ac_cv_gnu_extensions,
    [ AC_TRY_COMPILE([#include <features.h>], [
#ifndef __GNU_LIBRARY__
gnuneeded
#endif
    ], [ ac_cv_gnu_extensions=yes ], [ ac_cv_gnu_extensions=no ])
    ])
    if test x$ac_cv_gnu_extensions = xyes; then
        ifelse([$1], [], [:], [$1])
    dnl AC_DEFINE_UNQUOTED(_GNU_SOURCE)
    else
        ifelse([$2], [], [:], [$2])
    fi
])

dnl
dnl AC_CHECK_GNU_VECTOR([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
AC_DEFUN([AC_CHECK_GNU_VECTOR],
[
    AC_CACHE_CHECK([if gcc supports vector builtins], ac_cv_gnu_builtins,
    [ AC_TRY_COMPILE([#include <features.h>], [
int main(void) { 
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 2)
return 0;
#else
#error no vector builtins
#endif
}
    ], [ ac_cv_gnu_builtins=yes ], [ ac_cv_gnu_builtins=no ])
    ])
    if test x$ac_cv_gnu_builtins = xyes; then
        ifelse([$1], [], [:], [$1])
    else
        ifelse([$2], [], [:], [$2])
    fi
])
