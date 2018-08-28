# Configure paths for VIDIX

dnl Check if vidix support should be build

AC_DEFUN([AM_PATH_VIDIX],
[
    AC_ARG_ENABLE(vidix, [  --enable-vidix          build vidix drivers. [default=yes]],
		  [], enable_vidix=yes)
    AC_MSG_CHECKING([for vidix])
    if test x$enable_vidix = xyes -a x$ac_cv_prog_AWK != xno; then
      case "$target" in
        i?86-*-linux* | k?-*-linux* | athlon-*-linux*)
          enable_vidix=yes
          enable_linux=yes
          ;;
        i386-*-freebsd*)
          enable_vidix=yes
          enable_dha_kmod=no
          ;;
        *)
          enable_dha_kmod=no
          enable_vidix=no
          ;;
      esac
    fi
dnl for now without linux kernel support
    enable_linux=no 

    AC_MSG_RESULT([$enable_vidix])

    if test x$enable_vidix = xyes ; then
    	AC_DEFINE(HAVE_VIDIX, 1, [Define if you want to have vidix support.])
    fi
])

