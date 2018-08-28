dnl Usage:
dnl AC_CHECK_OSS([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for OSS audio interface, and defines
dnl prerequisites:

AC_DEFUN([AC_CHECK_OSS],
[
    AC_ARG_ENABLE(oss, [  --enable-oss            support the OSS audio API. (autodetect)],
		  [], enable_oss=yes)
    if test x$enable_oss = xyes; then
	AC_CACHE_CHECK([for OSS audio support], ac_cv_val_have_oss_audio,
	    [AC_TRY_COMPILE([
		  #ifdef __NetBSD__
		  #include <sys/ioccom.h>
		  #include <soundcard.h>
		  #else
		  #include <sys/soundcard.h>
		  #endif
		],[ int arg = SNDCTL_DSP_SETFRAGMENT; ],
		[ ac_cv_val_have_oss_audio=yes ], [ ac_cv_val_have_oss_audio=no ])
	    ])
	enable_oss=$ac_cv_val_have_oss_audio
    fi
    
    if test x$enable_oss = xyes; then
	AC_CHECK_LIB([ossaudio], [_oss_ioctl])
        ifelse([$1], , :, [$1])
    else
        ifelse([$2], , :, [$2])
    fi
])

