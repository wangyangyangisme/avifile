dnl 
dnl slightly modified  dmalloc.m4
dnl 

AC_DEFUN([AC_WITH_DMALLOCTH],
[
AC_ARG_WITH(dmallocth, [  --with-dmallocth        use dmallocth. (see: http://www.dmalloc.com)],
	    [], with_dmallocth=no)
if test x$with_dmallocth = xyes ; then
    AC_MSG_RESULT([enabling dmalloc thread debugging])
    AC_CHECK_HEADER(dmalloc.h, , AC_MSG_ERROR([header file 'dmalloc.h' is missing!]))
    AC_DEFINE(WITH_DMALLOCTH,1, [Define if using the dmallocth debugging malloc package])
    LIBS="$LIBS -ldmallocth"
    LDFLAGS="$LDFLAGS -g"
fi
])
