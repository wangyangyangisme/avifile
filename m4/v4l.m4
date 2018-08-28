# Configure paths for video4linux

dnl Usage:
dnl AC_CHECK_V4L([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for video4linux interface, and defines
dnl prerequisites:

AC_DEFUN([AC_CHECK_V4L],
[
    AC_ARG_ENABLE(v4l, [  --enable-v4l            support v4l video interface. (autodetect)],
		  [], enable_v4l=yes)
    if test x$enable_v4l = xyes; then
	AC_CHECK_HEADER([linux/videodev.h], [], [ enable_v4l=no; ])
    fi

    if test x$enable_v4l = xyes; then
        ifelse([$1], , :, [$1])
    else
        ifelse([$2], , :, [$2])
    fi
])

