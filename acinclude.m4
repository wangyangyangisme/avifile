# as.m4
# Figure out how to run the assembler.

# AM_PROG_AS
AC_DEFUN([AM_PROG_AS],
[# By default we simply use the C compiler to build assembly code.
AC_REQUIRE([AC_PROG_CC])
: ${AS='$(CC)'}
# Set ASFLAGS if not already set.
: ${ASFLAGS='$(CFLAGS)'}
AC_SUBST(AS)
AC_SUBST(ASFLAGS)
])

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

dnl Check for lrintf presence
dnl currently needed only by ffmpeg
AC_DEFUN([AC_FUNC_LRINTF],
[
    AC_CACHE_CHECK([whether system declares lrintf function in math.h],
		   ac_cv_val_HAVE_LRINTF,
		   [ac_save_CFLAGS=$CFLAGS
		   ac_save_LIBS=$LIBS
                   CFLAGS="$CFLAGS -O2"
		   LIBS="$LIBS -lm"
                   AC_TRY_RUN([
		    #define _ISOC9X_SOURCE  1
		    #include <math.h>
		    int main( void ) { return (lrintf(3.999f) > 0)?0:1; } ],
		   [ ac_cv_val_HAVE_LRINTF=yes ], [ ac_cv_val_HAVE_LRINTF=no ],
		   [ echo $ac_n "cross compiling; assumed OK... $ac_c" ])
                   CFLAGS=$ac_save_CFLAGS
                   LIBS=$ac_save_LIBS
		   ])
    if test x$ac_cv_val_HAVE_LRINTF = xyes; then
	AC_DEFINE(HAVE_LRINTF, 1, [Define if you have lrintf on your system.])
    fi
])


dnl Usage:
dnl AM_PATH_FFMPEG([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for ffmpeg, and define FFMPEG_CFLAGS and FFMPEG_LIBS

AC_DEFUN([AM_PATH_FFMPEG],
[
    AC_REQUIRE([AC_FUNC_LRINTF])

    dnl !!! - it really sucks - Makefile.am can't be optimized even by
    dnl !!! preparing some extra variable forward
    dnl !!! FFAVFORMAT_SRC=

    AC_REQUIRE([AC_CHECK_OSS])
    test x$enable_oss = xyes && AC_DEFINE(CONFIG_AUDIO_OSS, 1, [Define if you want to have ffmpeg OSS audio support compiled.])

    AC_REQUIRE([AC_CHECK_V4L])
    test x$enable_v4l = xyes && AC_DEFINE(CONFIG_VIDEO4LINUX, 1, [Define if you have video4linux device. (ffmpeg)])

    AC_REQUIRE([AC_FIND_ZLIB])
    test x$have_zlib = xyes && AC_DEFINE(CONFIG_ZLIB, 1, [Define if you have z library (-lz) (ffmpeg)])

    AC_ARG_ENABLE(ffmpeg_faadbin, [  --enable-ffmpeg-faadbin build ffmpeg with FAAD binary support. (default=yes)],
	          [], enable_ffmpeg_faadbin=yes)
    test x$enable_ffmpeg_faadbin = xyes && AC_DEFINE(CONFIG_FAADBIN, 1, [Define if you want to build support for runtime linked libfaad.])
    AC_REQUIRE([AM_PATH_FAAD])
    test x$have_faad = xyes && AC_DEFINE(CONFIG_FAAD, 1, [Define if you want to have AAC support compiled. (ffmpeg)])

    AC_ARG_ENABLE(ffmpeg_risky, [  --enable-ffmpeg-risky   build ffmpeg risky code. (default=yes)],
	          [], enable_ffmpeg_risky=yes)

    AC_ARG_ENABLE(ffmpeg_a52, [  --enable-ffmpeg-a52     build ffmpeg with A52 (AC3) support. (default=yes)],
		  [], enable_ffmpeg_a52=yes)
    if test x$enable_ffmpeg_a52 = xyes ; then
	AC_DEFINE(HAVE_FFMPEG_A52, 1, [Define if you want to use ffmpeg A52 audio decoder.])
	AC_ARG_ENABLE(ffmpeg_a52bin, [  --enable-ffmpeg-a52bin  A52 plugin dlopens liba52.so.0 at runtime (default=no)],
		      [], enable_ffmpeg_a52bin=no)

        if test x$enable_ffmpeg_a52bin = xyes ; then
            AC_DEFINE(CONFIG_A52BIN, 1, [Define if you want to build ffmpeg with A52 dlopened decoder.])
        fi
    fi

    test x$enable_ffmpeg_risky = xyes && AC_DEFINE(CONFIG_RISKY, 1, [Define if you want to compile patent encumbered codecs. (ffmpeg)])

    AC_DEFINE(CONFIG_FFSERVER, 1, [Define if you want to build ffmpeg server.])

    AC_DEFINE(CONFIG_ENCODERS, 1, [Define if you want to build ffmpeg encoders.])
    AC_DEFINE(CONFIG_DECODERS, 1, [Define if you want to build ffmpeg decoders.])
    AC_DEFINE(CONFIG_AC3, 1, [Define if you want to build ffmpeg with AC3 decoder.])
    AC_DEFINE(SIMPLE_IDCT, 1, [Define if you want to build ffmpeg with simples idct.])
    AC_DEFINE(CONFIG_NETWORK, 1, [Define if you want to build ffmpeg with network support.])
    AC_DEFINE(CONFIG_MPEGAUDIO_HP, 1, [Define if you want to have highquality ffmpeg mpeg audio support compiled.])
    AC_DEFINE(HAVE_PTHREADS, 1, [Define if you want to have ffmpeg pthread support compiled.])

dnl cut & paste from ffmpeg
dnl    AC_DEFINE(CONFIG_AC3_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MP2_ENCODER, 1, [ffmpeg])
dnl    AC_DEFINE(CONFIG_MP3LAME_ENCODER, 1, [ffmpeg])
dnl    AC_DEFINE(CONFIG_OGGVORBIS_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_OGGVORBIS_DECODER, 1, [ffmpeg])
dnl    AC_DEFINE(CONFIG_OGGTHEORA_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_OGGTHEORA_DECODER, 1, [ffmpeg])
dnl    AC_DEFINE(CONFIG_FAAC_ENCODER, 1, [ffmpeg])
dnl    AC_DEFINE(CONFIG_XVID_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MPEG1VIDEO_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_H264_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MPEG2VIDEO_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_H261_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_H263_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_H263P_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_FLV_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_RV10_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_RV20_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MPEG4_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MSMPEG4V1_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MSMPEG4V2_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MSMPEG4V3_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_WMV1_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_WMV2_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_SVQ1_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MJPEG_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_LJPEG_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_PNG_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_PPM_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_PGM_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_PGMYUV_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_PBM_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_PAM_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_HUFFYUV_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_FFVHUFF_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_ASV1_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_ASV2_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_FFV1_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_SNOW_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_ZLIB_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_DVVIDEO_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_SONIC_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_SONIC_LS_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_X264_ENCODER, 1, [ffmpeg])
dnl    AC_DEFINE(CONFIG_LIBGSM_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_RAWVIDEO_ENCODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_RAWVIDEO_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_H263_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_H261_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MPEG4_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MSMPEG4V1_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MSMPEG4V2_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MSMPEG4V3_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_WMV1_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_WMV2_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_VC9_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_WMV3_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_H263I_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_FLV_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_RV10_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_RV20_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_SVQ1_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_SVQ3_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_WMAV1_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_WMAV2_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_INDEO2_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_INDEO3_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_TSCC_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_ULTI_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_QDRAW_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_XL_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_QPEG_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_LOCO_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_WNV1_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_AASC_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_FRAPS_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_AAC_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MPEG4AAC_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MPEG1VIDEO_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MPEG2VIDEO_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MPEGVIDEO_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MPEG_XVMC_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_DVVIDEO_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MJPEG_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MJPEGB_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_SP5X_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_PNG_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MP2_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MP3_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MP3ADU_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MP3ON4_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MACE3_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MACE6_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_HUFFYUV_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_FFVHUFF_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_FFV1_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_SNOW_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_CYUV_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_H264_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_VP3_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_THEORA_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_ASV1_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_ASV2_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_VCR1_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_CLJR_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_FOURXM_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MDEC_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_ROQ_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_INTERPLAY_VIDEO_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_XAN_WC3_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_RPZA_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_CINEPAK_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MSRLE_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MSVIDEO1_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_VQA_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_IDCIN_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_EIGHTBPS_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_SMC_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_FLIC_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_TRUEMOTION1_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_TRUEMOTION2_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_VMDVIDEO_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_VMDAUDIO_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_MSZH_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_ZLIB_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_SONIC_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_AC3_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_DTS_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_RA_144_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_RA_288_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_ROQ_DPCM_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_INTERPLAY_DPCM_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_XAN_DPCM_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_SOL_DPCM_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_QTRLE_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_FLAC_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_SHORTEN_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_ALAC_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_WS_SND1_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_VORBIS_DECODER, 1, [ffmpeg])
    AC_DEFINE(CONFIG_QDM2_DECODER, 1, [ffmpeg])
dnl    AC_DEFINE(CONFIG_LIBGSM_DECODER, 1, [ffmpeg])
dnl    AC_DEFINE(CONFIG_AMR_NB_DECODER, 1, [ffmpeg])
dnl    AC_DEFINE(CONFIG_AMR_NB_ENCODER, 1, [ffmpeg])
dnl    AC_DEFINE(CONFIG_AMR_WB_DECODER, 1, [ffmpeg])
dnl    AC_DEFINE(CONFIG_AMR_WB_ENCODER, 1, [ffmpeg])

    AC_DEFINE(CONFIG_MUXERS, 1, [ffmpeg])
    AC_DEFINE(CONFIG_DEMUXERS, 1, [ffmpeg])
    if test -n "$ffmpeg_profiling" ; then
	AC_DEFINE(HAVE_GPROF, 1, [Define if you want to build ffmpeg with gprof support.])
    fi

    test -z "$FFMPEG_CFLAGS" && FFMPEG_CFLAGS="-O4 $DEFAULT_FLAGS"
    FFMPEG_CFLAGS="$FFMPEG_CFLAGS $DEFAULT_DEFINES"

    AC_CHECK_GNU_VECTOR(FFMPEG_SSE="-msse";
		        AC_DEFINE(HAVE_BUILTIN_VECTOR, 1, [Define if your compiler supports vector builtins (MMX).]), [])
    AC_SUBST(FFMPEG_CFLAGS)
    AC_SUBST(FFMPEG_SSE)

dnl ffmpeg_x86opt=
dnl test x$enable_x86opt = xno && ffmpeg_x86opt="--disable-mmx"
dnl AC_MSG_RESULT([=== configuring ffmpeg ===])
dnl    ( cd $srcdir/ffmpeg ; \
dnl      ./configure --prefix=$prefix --cc=$CC \
dnl		$ffmpeg_profiling $ffmpeg_cpu \
dnl		$ffmpeg_x86opt --disable-mp3lib \
dnl		--enable-simple_idct )
])
# Configure paths for FreeType2
# Marcelo Magallon 2001-10-26, based on gtk.m4 by Owen Taylor

dnl AC_CHECK_FT2([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for FreeType2, and define FT2_CFLAGS and FT2_LIBS
dnl
AC_DEFUN([AC_CHECK_FT2],
[dnl
dnl Get the cflags and libraries from the freetype-config script
dnl
AC_ARG_WITH(ft-prefix,
[  --with-ft-prefix=PREFIX prefix where FreeType is installed (optional)],
            ft_config_prefix="$withval", ft_config_prefix="")
AC_ARG_WITH(ft-exec-prefix,
[  --with-ft-exec-prefix=PFX exec prefix where FreeType is installed (optional)],
            ft_config_exec_prefix="$withval", ft_config_exec_prefix="")
AC_ARG_ENABLE(freetypetest,
[  --disable-freetypetest  Do not try to compile and run FT2 test program],
              [], enable_fttest=yes)

if test x$ft_config_exec_prefix != x ; then
  ft_config_args="$ft_config_args --exec-prefix=$ft_config_exec_prefix"
  if test x${FT2_CONFIG+set} != xset ; then
    FT2_CONFIG=$ft_config_exec_prefix/bin/freetype-config
  fi
fi
if test x$ft_config_prefix != x ; then
  ft_config_args="$ft_config_args --prefix=$ft_config_prefix"
  if test x${FT2_CONFIG+set} != xset ; then
    FT2_CONFIG=$ft_config_prefix/bin/freetype-config
  fi
fi
AC_PATH_PROG(FT2_CONFIG, freetype-config, no)

min_ft_version=ifelse([$1], ,6.1.0,$1)
AC_MSG_CHECKING(for FreeType - version >= $min_ft_version)
no_ft=""
if test "$FT2_CONFIG" = "no" ; then
  no_ft=yes
else
  FT2_CFLAGS=`$FT2_CONFIG $ft_config_args --cflags`
  FT2_LIBS=`$FT2_CONFIG $ft_config_args --libs`
  ft_config_major_version=`$FT2_CONFIG $ft_config_args --version | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
  ft_config_minor_version=`$FT2_CONFIG $ft_config_args --version | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
  ft_config_micro_version=`$FT2_CONFIG $ft_config_args --version | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
  ft_min_major_version=`echo $min_ft_version | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
  ft_min_minor_version=`echo $min_ft_version | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
  ft_min_micro_version=`echo $min_ft_version | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
  if test x$enable_fttest = xyes ; then
    ft_config_is_lt=""
    if test $ft_config_major_version -lt $ft_min_major_version ; then
      ft_config_is_lt=yes
    else
      if test $ft_config_major_version -eq $ft_min_major_version ; then
        if test $ft_config_minor_version -lt $ft_min_minor_version ; then
          ft_config_is_lt=yes
        else
          if test $ft_config_minor_version -eq $ft_min_minor_version ; then
            if test $ft_config_micro_version -lt $ft_min_micro_version ; then
              ft_config_is_lt=yes
            fi
          fi
        fi
      fi
    fi
    if test x$ft_config_is_lt = xyes ; then
      no_ft=yes
    else
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $FT2_CFLAGS"
      LIBS="$FT2_LIBS $LIBS"
dnl
dnl Sanity checks for the results of freetype-config to some extent
dnl
      AC_TRY_RUN([
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdio.h>
#include <stdlib.h>

int
main()
{
  FT_Library library;
  FT_Error error;

  error = FT_Init_FreeType(&library);

  if (error)
    return 1;
  else
  {
    FT_Done_FreeType(library);
    return 0;
  }
}
],, no_ft=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
      CFLAGS="$ac_save_CFLAGS"
      LIBS="$ac_save_LIBS"
    fi             # test $ft_config_version -lt $ft_min_version
  fi               # test x$enable_fttest = xyes
fi                 # test "$FT2_CONFIG" = "no"
if test x$no_ft = x ; then
   AC_MSG_RESULT(yes)
   ifelse([$2], , :, [$2])
else
   AC_MSG_RESULT(no)
   if test "$FT2_CONFIG" = "no" ; then
     echo "*** The freetype-config script installed by FreeType 2 could not be found."
     echo "*** If FreeType 2 was installed in PREFIX, make sure PREFIX/bin is in"
     echo "*** your path, or set the FT2_CONFIG environment variable to the"
     echo "*** full path to freetype-config."
   else
     if test x$ft_config_is_lt = xyes ; then
       echo "*** Your installed version of the FreeType 2 library is too old."
       echo "*** If you have different versions of FreeType 2, make sure that"
       echo "*** correct values for --with-ft-prefix or --with-ft-exec-prefix"
       echo "*** are used, or set the FT2_CONFIG environment variable to the"
       echo "*** full path to freetype-config."
     else
       echo "*** The FreeType test program failed to run.  If your system uses"
       echo "*** shared libraries and they are installed outside the normal"
       echo "*** system library path, make sure the variable LD_LIBRARY_PATH"
       echo "*** (or whatever is appropiate for your system) is correctly set."
     fi
   fi
   FT2_CFLAGS=""
   FT2_LIBS=""
   ifelse([$3], , :, [$3])
fi
AC_SUBST(FT2_CFLAGS)
AC_SUBST(FT2_LIBS)
])
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
# Configure paths for libogg
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl AM_PATH_OGG([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libogg, and define OGG_CFLAGS and OGG_LIBS
dnl
AC_DEFUN([AM_PATH_OGG],
[dnl
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(ogg-prefix,[  --with-ogg-prefix=PFX   prefix where libogg is installed. (optional)], ogg_prefix="$withval", ogg_prefix="")
AC_ARG_ENABLE(oggtest, [  --disable-oggtest       do not try to compile and run a test Ogg program.],, enable_oggtest=yes)

  if test "x$ogg_prefix" != "xNONE" ; then
    ogg_args="$ogg_args --prefix=$ogg_prefix"
    OGG_CFLAGS="-I$ogg_prefix/include"
    OGG_LIBS="-L$ogg_prefix/lib"
  elif test "$prefix" != ""; then
    ogg_args="$ogg_args --prefix=$prefix"
    OGG_CFLAGS="-I$prefix/include"
    OGG_LIBS="-L$prefix/lib"
  fi

  OGG_LIBS="$OGG_LIBS -logg"

  AC_MSG_CHECKING(for Ogg)
  no_ogg=""


  if test "x$enable_oggtest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $OGG_CFLAGS"
    LIBS="$LIBS $OGG_LIBS"
dnl
dnl Now check if the installed Ogg is sufficiently new.
dnl
      rm -f conf.oggtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>

int main ()
{
  system("touch conf.oggtest");
  return 0;
}

],, no_ogg=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_ogg" = "x" ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])
  else
     AC_MSG_RESULT(no)
     if test -f conf.oggtest ; then
       :
     else
       echo "*** Could not run Ogg test program, checking why..."
       CFLAGS="$CFLAGS $OGG_CFLAGS"
       LIBS="$LIBS $OGG_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <ogg/ogg.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding Ogg or finding the wrong"
       echo "*** version of Ogg. If it is not finding Ogg, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means Ogg was incorrectly installed"
       echo "*** or that you have moved Ogg since it was installed. In the latter case, you"
       echo "*** may want to edit the ogg-config script: $OGG_CONFIG" ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     OGG_CFLAGS=""
     OGG_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(OGG_CFLAGS)
  AC_SUBST(OGG_LIBS)
  rm -f conf.oggtest
])

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


dnl PKG_CHECK_MODULES(GSTUFF, gtk+-2.0 >= 1.3 glib = 1.3.4, action-if, action-not)
dnl defines GSTUFF_LIBS, GSTUFF_CFLAGS, see pkg-config man page
dnl also defines GSTUFF_PKG_ERRORS on error
AC_DEFUN([PKG_CHECK_MODULES], [
  succeeded=no

  if test -z "$PKG_CONFIG"; then
    AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  fi

  if test "$PKG_CONFIG" = "no" ; then
     echo "*** The pkg-config script could not be found. Make sure it is"
     echo "*** in your path, or set the PKG_CONFIG environment variable"
     echo "*** to the full path to pkg-config."
     echo "*** Or see http://www.freedesktop.org/software/pkgconfig to get pkg-config."
  else
     PKG_CONFIG_MIN_VERSION=0.9.0
     if $PKG_CONFIG --atleast-pkgconfig-version $PKG_CONFIG_MIN_VERSION; then
        AC_MSG_CHECKING(for $2)

        if $PKG_CONFIG --exists "$2" ; then
            AC_MSG_RESULT(yes)
            succeeded=yes

            AC_MSG_CHECKING($1_CFLAGS)
            $1_CFLAGS=`$PKG_CONFIG --cflags "$2"`
            AC_MSG_RESULT($$1_CFLAGS)

            AC_MSG_CHECKING($1_LIBS)
            $1_LIBS=`$PKG_CONFIG --libs "$2"`
            AC_MSG_RESULT($$1_LIBS)
        else
            $1_CFLAGS=""
            $1_LIBS=""
            ## If we have a custom action on failure, don't print errors, but 
            ## do set a variable so people can do so.
            $1_PKG_ERRORS=`$PKG_CONFIG --errors-to-stdout --print-errors "$2"`
            ifelse([$4], ,echo $$1_PKG_ERRORS,)
        fi

        AC_SUBST($1_CFLAGS)
        AC_SUBST($1_LIBS)
     else
        echo "*** Your version of pkg-config is too old. You need version $PKG_CONFIG_MIN_VERSION or newer."
        echo "*** See http://www.freedesktop.org/software/pkgconfig"
     fi
  fi

  if test $succeeded = yes; then
     ifelse([$3], , :, [$3])
  else
     ifelse([$4], , AC_MSG_ERROR([Library requirements ($2) not met; consider adjusting the PKG_CONFIG_PATH environment variable if your libraries are in a nonstandard prefix so pkg-config can find them.]), [$4])
  fi
])

# Configure paths for SDL
# Sam Lantinga 9/21/99
# stolen from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor
# slightly modified for avifile

dnl AM_PATH_SDL([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for SDL, and define SDL_CFLAGS and SDL_LIBS
dnl
AC_DEFUN([AM_PATH_SDL],
[dnl 
dnl Get the cflags and libraries from the sdl-config script
dnl
AC_ARG_WITH(sdl-prefix,[  --with-sdl-prefix=PFX   prefix where SDL is installed. (optional)],
            sdl_prefix="$withval", sdl_prefix="")
AC_ARG_WITH(sdl-exec-prefix,[  --with-sdl-exec-prefix=PFX exec prefix where SDL is installed. (optional)],
            sdl_exec_prefix="$withval", sdl_exec_prefix="")
AC_ARG_ENABLE(sdltest, [  --disable-sdltest       do not try to compile and run a test SDL program.],
		    , enable_sdltest=yes)

  if test x$sdl_exec_prefix != x ; then
     sdl_args="$sdl_args --exec-prefix=$sdl_exec_prefix"
     if test x${SDL_CONFIG+set} != xset ; then
        SDL_CONFIG=$sdl_exec_prefix/bin/sdl-config
     fi
  fi
  if test x$sdl_prefix != x ; then
     sdl_args="$sdl_args --prefix=$sdl_prefix"
     if test x${SDL_CONFIG+set} != xset ; then
        SDL_CONFIG=$sdl_prefix/bin/sdl-config
     fi
  fi
  if test -z "$SDL_CONFIG"; then
      AC_CHECK_PROGS(SDL_MY_CONFIG, sdl-config sdl11-config, "")
      if test -n "$SDL_MY_CONFIG"; then
	   SDL_CONFIG=`which $SDL_MY_CONFIG`
	   echo "setting SDL_CONFIG to $SDL_CONFIG"
      fi
  fi

  AC_REQUIRE([AC_CANONICAL_TARGET])
  PATH="$prefix/bin:$prefix/usr/bin:$PATH"
  AC_PATH_PROG(SDL_CONFIG, sdl-config, no, [$PATH])
  min_sdl_version=ifelse([$1], ,0.11.0,$1)
  AC_MSG_CHECKING(for SDL - version >= $min_sdl_version)
  no_sdl=""
  if test "$SDL_CONFIG" = "no" ; then
    no_sdl=yes
  else
    SDL_CFLAGS=`$SDL_CONFIG $sdlconf_args --cflags`
    SDL_LIBS=`$SDL_CONFIG $sdlconf_args --libs | sed -e 's!-L/usr/lib[[^/]]!!g'`

    sdl_major_version=`$SDL_CONFIG $sdl_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    sdl_minor_version=`$SDL_CONFIG $sdl_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    sdl_micro_version=`$SDL_CONFIG $sdl_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_sdltest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $SDL_CFLAGS"
      LIBS="$LIBS $SDL_LIBS"
dnl
dnl Now check if the installed SDL is sufficiently new. (Also sanity
dnl checks the results of sdl-config to some extent
dnl
      rm -f conf.sdltest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.sdltest");
  */
  { FILE *fp = fopen("conf.sdltest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_sdl_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_sdl_version");
     exit(1);
   }

   if (($sdl_major_version > major) ||
      (($sdl_major_version == major) && ($sdl_minor_version > minor)) ||
      (($sdl_major_version == major) && ($sdl_minor_version == minor) && ($sdl_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'sdl-config --version' returned %d.%d.%d, but the minimum version\n", $sdl_major_version, $sdl_minor_version, $sdl_micro_version);
      printf("*** of SDL required is %d.%d.%d. If sdl-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If sdl-config was wrong, set the environment variable SDL_CONFIG\n");
      printf("*** to point to the correct copy of sdl-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_sdl=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_sdl" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$SDL_CONFIG" = "no" ; then
       echo "*** The sdl-config script installed by SDL could not be found"
       echo "*** If SDL was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the SDL_CONFIG environment variable to the"
       echo "*** full path to sdl-config."
     else
       if test -f conf.sdltest ; then
        :
       else
          echo "*** Could not run SDL test program, checking why..."
          CFLAGS="$CFLAGS $SDL_CFLAGS"
          LIBS="$LIBS $SDL_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include "SDL.h"

int main(int argc, char *argv[])
{ return 0; }
#undef  main
#define main K_and_R_C_main
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding SDL or finding the wrong"
          echo "*** version of SDL. If it is not finding SDL, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means SDL was incorrectly installed"
          echo "*** or that you have moved SDL since it was installed. In the latter case, you"
          echo "*** may want to edit the sdl-config script: $SDL_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     SDL_CFLAGS=""
     SDL_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(SDL_CFLAGS)
  AC_SUBST(SDL_LIBS)
  rm -f conf.sdltest
])

AC_DEFUN([MY_TEST_SDL],
[
AC_MSG_CHECKING([for SDL development libraries])
cat > conftest.c <<EOF
void main()
{
}
EOF
SDL_LIBTOOL=`$SDL_CONFIG --prefix`/lib/libSDL.la
if test -r $SDL_LIBTOOL ; then
    if libtool $CC conftest.c $SDL_LIBTOOL $LIBS $SDL_LIBS -o conftest >&5 2>&5; then
	AC_MSG_RESULT([found])
        GOOD_SDL_INSTALLATION=yes
    else
	AC_MSG_RESULT([linking against SDL library failed. Check config.log for details.])
        GOOD_SDL_INSTALLATION=no
    fi
else
    AC_MSG_RESULT([not found])
    GOOD_SDL_INSTALLATION=no
fi
rm -f conftest.c conftest
])

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

dnl    -*- shell-script -*-

dnl    This file is part of the Avifile packages
dnl    and has been heavily modified for its purposes
dnl    Copyright (C) 2002 Zdenek Kabelac (kabi@users.sourceforge.net)
dnl
dnl    Originaly this file was part of the KDE libraries/packages
dnl    Copyright (C) 1997 Janos Farkas (chexum@shadow.banki.hu)
dnl              (C) 1997 Stephan Kulow (coolo@kde.org)

dnl    This file is free software; you can redistribute it and/or
dnl    modify it under the terms of the GNU Library General Public
dnl    License as published by the Free Software Foundation; either
dnl    version 2 of the License, or (at your option) any later version.

dnl    This library is distributed in the hope that it will be useful,
dnl    but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl    Library General Public License for more details.

dnl    You should have received a copy of the GNU Library General Public License
dnl    along with this library; see the file COPYING.LIB.  If not, write to
dnl    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
dnl    Boston, MA 02111-1307, USA.


dnl ------------------------------------------------------------------------
dnl Find a file (or one of more files in a list of dirs)
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_FIND_FILE],
[
$3=NO
for i in $2;
do
  for j in $1;
  do
    if test -r "$i/$j"; then
      $3=$i
      break 2
    fi
  done
done
])

AC_DEFUN([KDE_MOC_ERROR_MESSAGE],
[
    AC_MSG_ERROR([No valid Qt meta object compiler (moc) found!
Please check whether you installed Qt correctly.
You need to have a running moc binary.
configure tried to run $ac_cv_path_moc and the test did not
succeed. If configure should not have tried this one, set
the environment variable MOC to the right one before running
configure.])
])

AC_DEFUN([KDE_UIC_ERROR_MESSAGE],
[
    AC_MSG_WARN([No valid Qt ui compiler (uic) found!
Please check whether you installed Qt correctly.
You need to have a running uic binary.
configure tried to run $ac_cv_path_uic and the test did not
succeed. If configure should not have tried this one, set
the environment variable UIC to the right one before running
configure.])
])

dnl ------------------------------------------------------------------------
dnl Find the meta object compiler in the PATH, in $QTDIR/bin, and some
dnl more usual places
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_PATH_QT_MOC_UIC],
[
dnl usually qt is installed in such a way it might help to
dnl try this binpath first
    cutlib_binpath=`echo "$ac_qt_libraries" | sed 's%/[[^/]]*$%%'`
    lc_qt_binpath="$ac_qt_binpath:$cutlib_binpath/bin:$PATH:/usr/local/bin:/usr/local/qt3/bin:/usr/local/qt2/bin:/usr/local/qt/bin:/usr/lib/qt3/bin:/usr/lib/qt2/bin:/usr/lib/qt/bin:/usr/bin:/usr/X11R6/bin"
    AC_PATH_PROGS(MOC, moc moc3 moc2, , $lc_qt_binpath)
    dnl AC_PATH_PROGS(UIC, uic, , $lc_qt_binpath)

    if test -z "$MOC"; then
        if test -n "$ac_cv_path_moc"; then
            output=`eval "$ac_cv_path_moc --help 2>&1 | sed -e '1q' | grep Qt"`
        fi
        echo "configure:__oline__: tried to call $ac_cv_path_moc --help 2>&1 | sed -e '1q' | grep Qt" >&AC_FD_CC
        echo "configure:__oline__: moc output: $output" >&AC_FD_CC

        if test -z "$output"; then
            KDE_MOC_ERROR_MESSAGE
        fi
    fi

    AC_SUBST(MOC)

    dnl We do not need UIC
    dnl [KDE_UIC_ERROR_MESSAGE])
    dnl AC_SUBST(UIC)
])


dnl AC_PATH_QT([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl ------------------------------------------------------------------------
dnl Try to find the Qt headers and libraries.
dnl $(QT_LIBS) will be -Lqtliblocation (if needed) -lqt_library_name
dnl and $(QT_CFLAGS) will be -Iqthdrlocation (if needed)
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_PATH_QT],
[
AC_REQUIRE([AC_PATH_XTRA])

ac_have_qt=yes
ac_qt_binpath=
ac_qt_includes=
ac_qt_libraries=
ac_qt_notfound=
qt_libraries=
qt_includes=
min_qt_version=ifelse([$1], ,100, $1)

AC_ARG_WITH(qt_prefix, [  --with-qt-prefix        where the root of Qt is installed.],
    [  ac_qt_includes="$withval"/include
       ac_qt_libraries="$withval"/lib
       ac_qt_binpath="$withval"/bin
    ])

AC_ARG_WITH(qt_dir, [  --with-qt-dir           obsoleted, use --with-qt-prefix!],
    [  ac_qt_includes="$withval"/include
       ac_qt_libraries="$withval"/lib
       ac_qt_binpath="$withval"/bin
       AC_MSG_WARN([
!!!
!!! Using obsoleted option: --with-qt-dir, instead use --with-qt-prefix
!!!])
    ])


test -n "$QTDIR" && ac_qt_binpath="$ac_qt_binpath:$QTDIR/bin:$QTDIR"

AC_ARG_WITH(qt_includes, [  --with-qt-includes      where the Qt includes are.],
    [ ac_qt_includes=$withval ])

AC_ARG_WITH(qt_libraries, [  --with-qt-libraries     where the Qt library is installed.],
    [ ac_qt_libraries=$withval ])

if test -z "$ac_qt_includes" || test -z "$ac_qt_libraries" ; then
    AC_CACHE_VAL(ac_cv_have_qt,
    [ # try to guess Qt locations
      # various paths from various distros - 
      # you are welcome to suggest another
	qt_incdirs="$QTINC
            /usr/local/lib/qt3/include
            /usr/local/lib/qt2/include
            /usr/local/qt/include
            /usr/local/include
            /usr/lib/qt-3.1/include
            /usr/lib/qt3/include
            /usr/lib/qt-2.3.1/include
            /usr/lib/qt2/include
            /usr/include/qt3
            /usr/include/qt
            /usr/lib/qt/include
            $x_includes/qt3
            $x_includes/qt2
            $x_includes/X11/qt
            $x_includes
            /usr/include"
	qt_libdirs="$QTLIB
            /usr/local/lib/qt3/lib
            /usr/local/lib/qt2/lib
            /usr/local/qt/lib
            /usr/local/lib
            /usr/lib/qt-3.1/lib
            /usr/lib/qt3/lib
            /usr/lib/qt-2.3.1/lib
            /usr/lib/qt2/lib
            /usr/lib/qt/lib
            /usr/lib/qt
            $x_libraries/qt3
            $x_libraries/qt2
            $x_libraries
            /usr/lib"

	if test -n "$QTDIR" ; then
            qt_incdirs="$QTDIR/include $QTDIR $qt_incdirs"
	    qt_libdirs="$QTDIR/lib $QTDIR $qt_libdirs"
        fi

	qt_incdirs="$ac_qt_includes $qt_incdirs"
	qt_libdirs="$ac_qt_libraries $qt_libdirs"

	AC_FIND_FILE(qmovie.h, $qt_incdirs, qt_incdir)
	ac_qt_includes="$qt_incdir"

	qt_libdir=NONE
	for dir in $qt_libdirs ; do
            for qtname in qt3 qt2 qt ; do
                try="ls -1 $dir/lib$qtname.* $dir/lib$qtname-mt.*"
                if test -n "`$try 2> /dev/null`"; then
                      test -z "$QTNAME" && QTNAME=$qtname
                      qt_libdir=$dir
                      break
                fi
            done
            if test x$qt_libdir != xNONE ; then
                break
            fi
	    echo "tried $dir" >&AC_FD_CC
        done

        ac_qt_libraries=$qt_libdir

	test -z "$QTNAME" && QTNAME=qt
        ac_QTNAME=$QTNAME

	ac_save_QTLIBS=$LIBS
	LIBS="-L$ac_qt_libraries $LIBS $PTHREAD_LIBS"
        AC_CHECK_LIB($QTNAME-mt, main, ac_QTNAME=$QTNAME-mt)
	LIBS=$ac_save_QTLIBS

	ac_cv_have_qt="ac_have_qt=yes ac_qt_includes=$ac_qt_includes ac_qt_libraries=$ac_qt_libraries ac_QTNAME=$ac_QTNAME"

        if test "$ac_qt_includes" = NO || test "$ac_qt_libraries" = NONE ; then
            ac_cv_have_qt="ac_have_qt=no"
            ac_qt_notfound="(headers)"
            if test "$ac_qt_includes" = NO; then
                if test "$ac_qt_libraries" = NONE; then
                    ac_qt_notfound="(headers and libraries)"
                fi
            else
                ac_qt_notfound="(libraries)";
            fi
        fi
    ])
    eval "$ac_cv_have_qt"
else
    for qtname in qt3 qt2 qt ; do
        try="ls -1 $ac_qt_libraries/lib$qtname.* $ac_qt_libraries/lib$qtname-mt.*"
        if test -n "`$try 2> /dev/null`"; then
            test -z "$QTNAME" && QTNAME=$qtname
            break
        fi
    done
    ac_QTNAME=$QTNAME
    ac_save_QTLIBS=$LIBS
    LIBS="-L$ac_qt_libraries $LIBS $PTHREAD_LIBS"
    AC_CHECK_LIB($QTNAME-mt, main, ac_QTNAME=$QTNAME-mt)
    LIBS=$ac_save_QTLIBS
fi

if test x$ac_have_qt = xyes ; then
    AC_MSG_CHECKING([for Qt library (version >= $min_qt_version)])
    AC_CACHE_VAL(ac_cv_qt_version,
    [
	AC_LANG_SAVE
	AC_LANG_CPLUSPLUS

	ac_save_QTCXXFLAGS=$CXXFLAGS
	ac_save_QTLIBS=$LIBS
	CXXFLAGS="$CXXFLAGS -I$ac_qt_includes"
	LIBS="-L$ac_qt_libraries -l$ac_QTNAME $X_LIBS $LIBS $PTHREAD_LIBS"
	AC_TRY_RUN([
	    /*#include <qapplication.h>*/
	    /*#include <qmovie.h>*/
	    #include <qstring.h>
	    #include <qglobal.h>
	    #include <stdio.h>

	    int main(int argc, char* argv[]) {
		/*QApplication a( argc, argv );*/
		/*QMovie m; int s = m.speed();*/
		QString qa("test");
		unsigned int v = QT_VERSION;
		FILE* f = fopen("conf.qttest", "w");
		if (v > 400) v = (((v >> 16) & 0xff) * 100) + (((v >> 8) & 0xff) * 10) + (v & 0xff);
		if (f) fprintf(f, "%d\n", v);
		return 0;
	    }
	],
	[ AC_MSG_RESULT(yes); ac_cv_qt_version=`cat conf.qttest`; rm -f conf.qttest ],
	[ AC_MSG_RESULT(no); ac_have_qt=no; ac_cv_qt_version=ERROR ],
	[ echo $ac_n "cross compiling; assumed OK... $ac_c" ])

	if test x$ac_cv_qt_version = xERROR ; then
	    AC_MSG_WARN([
*** Could not run Qt test program, checking why...
*** Configure discovered/uses these settings:
*** Qt libraries: $ac_qt_libraries
*** Qt headers: $ac_qt_includes
*** Note:
***    Compilation of Qt utilities also might be turned off (if not wanted).
***    If you are experiencing problems which will not be described
***    bellow please report then on 'avifile@prak.org' mailing list
***    (i.e. some misdetection or omitted path)]
             )
             AC_TRY_LINK([ #include <qstring.h> ],
		     [ QString qa("test") ],
                     [ AC_MSG_ERROR([
*** Qt test program compiled, but did not run. This usually means
*** that the run-time linker is not finding Qt library or finding the wrong
*** version of Qt. If it is not finding Qt, you will need to set your
*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point
*** to the installed location  Also, make sure you have run ldconfig if that
*** is required on your system.
***
*** If you have an old version installed, it is best to remove it, although
*** you may also be able to get things to work by modifying LD_LIBRARY_PATH
***
*** i.e. bash> export LD_LIBRARY_PATH=$ac_qt_libraries:\$LD_LIBRARY_PATH]) ],
                     [ AC_MSG_ERROR([
*** The test program failed to compile or link. See the file config.log for the
*** exact error that occured. This usually means Qt was incorrectly installed
*** or that you have moved Qt since it was installed. In the latter case, you
*** may want to set QTDIR shell variable
***
*** Another possibility is you try to link Qt libraries compiled with
*** different version of g++. Unfortunately you can not mix C++ libraries
*** and object files created with different C++ compiler
*** i.e. g++-2.96 libraries and g++-2.95 objects
*** The most common case: Some users seems to be downgrading their
*** compiler without thinking about consequencies...]) ]
	    )
	fi

	CXXFLAGS=$ac_save_QTCXXFLAGS
	LIBS=$ac_save_QTLIBS
	AC_LANG_RESTORE
     ])
     qt_version=$ac_cv_qt_version
fi

if test x$ac_have_qt != xyes; then
    AC_MSG_WARN([
*** Could not find usable Qt $ac_qt_notfound on your system!
*** If it _is_ installed, delete ./config.cache and re-run ./configure,
*** specifying path to Qt headers and libraries in configure options.
*** Switching off Qt compilation!])
    ac_have_qt=no
else
    AC_MSG_RESULT([found $ac_QTNAME version $qt_version, libraries $ac_qt_libraries, headers $ac_qt_includes])
    if test $min_qt_version -le $qt_version ; then
        qt_libraries=$ac_qt_libraries
        qt_includes=$ac_qt_includes

        if test "$qt_includes" = "$x_includes" -o -z "$qt_includes" ; then
            QT_CFLAGS=
        else
            QT_CFLAGS="-I$qt_includes"
            all_includes="$QT_CFLAGS $all_includes"
        fi
	QT_CFLAGS="$QT_CFLAGS -DQT_THREAD_SUPPORT"

        if test "$qt_libraries" = NONE -o "$qt_libraries" = "$x_libraries" -o -z "$qt_libraries" -o "$qt_libraries" = "/usr/lib" ; then
            QT_LIBS=
        else
            QT_LIBS="-L$qt_libraries"
            all_libraries="$QT_LIBS $all_libraries"
        fi

        QT_LIBS="$QT_LIBS -l$ac_QTNAME"

        if test x$ac_have_qt = xyes ; then
            AC_PATH_QT_MOC_UIC
        fi

        AC_SUBST(qt_version)
        AC_SUBST(QT_CFLAGS)
        AC_SUBST(QT_LIBS)
    else
        AC_MSG_WARN([
*** Unsupported old version of Qt library found. Please upgrade.])
        ac_have_qt=no
    fi
fi

if test x$ac_have_qt = xyes ; then
    ifelse([$2], , :, [$2])
else
    ifelse([$3], , :, [$3])
fi

])


AC_DEFUN([AC_FIND_ZLIB],
[
    AC_CACHE_CHECK([for libz], ac_cv_lib_z,
		    [ac_save_LIBS=$LIBS
		    LIBS="$LIBS -lz"
		    AC_TRY_LINK([#include<zlib.h>],
			    [return (zlibVersion() == ZLIB_VERSION); ],
			    [ ac_cv_lib_z=yes ], [ ac_cv_lib_z=no ])
		    LIBS=$ac_save_LIBS])
    AC_SUBST(Z_LIBS)
    if test x$ac_cv_lib_z = xyes ; then
	Z_LIBS="-lz"
	have_zlib=yes
	ifelse([$1], , :, [$1])
    else
	Z_LIBS=
	have_zlib=no
	ifelse([$2], , :, [$2])
    fi
])


AC_DEFUN([AC_FIND_PNG],
[
AC_REQUIRE([AC_FIND_ZLIB])
AC_MSG_CHECKING([for libpng])
AC_CACHE_VAL(ac_cv_lib_png,
[ac_save_LIBS="$LIBS"
LIBS="$all_libraries -lpng $LIBZ -lm -lX11 $LIBSOCKET"
AC_TRY_LINK([#include<png.h>],
    [
    png_structp png_ptr = png_create_read_struct(  // image ptr
		PNG_LIBPNG_VER_STRING, 0, 0, 0 );
    return( png_ptr != 0 );
    ],
    eval "ac_cv_lib_png='-lpng $LIBZ -lm'",
    eval "ac_cv_lib_png=no")
    LIBS=$ac_save_LIBS
])
if eval "test ! \"`echo $ac_cv_lib_png`\" = no"; then
  AC_DEFINE_UNQUOTED(HAVE_LIBPNG)
  LIBPNG="$ac_cv_lib_png"
  AC_SUBST(LIBPNG)
  AC_MSG_RESULT($ac_cv_lib_png)
else
  AC_MSG_RESULT(no)
  LIBPNG=""
  AC_SUBST(LIBPNG)
fi
])


dnl just a wrapper to clean up configure.in
AC_DEFUN([KDE_PROG_LIBTOOL],
[
AC_REQUIRE([AM_ENABLE_SHARED])
AC_REQUIRE([AM_ENABLE_STATIC])
dnl libtool is only for C, so I must force him
dnl to find the correct flags for C++
ac_save_cc=$CC
ac_save_cflags="$CFLAGS"
CC=$CXX
CFLAGS="$CXXFLAGS"
dnl AM_PROG_LIBTOOL dnl for libraries
CC=$ac_save_cc
CFLAGS="$ac_save_cflags"
])


dnl Check for the type of the third argument of getsockname
AC_DEFUN([AC_CHECK_KSIZE_T],
[AC_MSG_CHECKING(for the third argument of getsockname)
AC_LANG_CPLUSPLUS
AC_CACHE_VAL(ac_cv_ksize_t,
[AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],[
socklen_t a=0;
getsockname(0,(struct sockaddr*)0, &a);
],
ac_cv_ksize_t=socklen_t,
ac_cv_ksize_t=)
if test -z "$ac_cv_ksize_t"; then
ac_save_cxxflags="$CXXFLAGS"
if test "$GCC" = "yes"; then
  CXXFLAGS="-Werror $CXXFLAGS"
fi
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],[
int a=0;
getsockname(0,(struct sockaddr*)0, &a);
],
ac_cv_ksize_t=int,
ac_cv_ksize_t=size_t)
CXXFLAGS=$ac_save_cxxflags
fi
])

if test -z "$ac_cv_ksize_t"; then
  ac_cv_ksize_t=int
fi

AC_MSG_RESULT($ac_cv_ksize_t)
AC_DEFINE_UNQUOTED(ksize_t, $ac_cv_ksize_t)

])



# Search path for a program which passes the given test.
# Ulrich Drepper <drepper@cygnus.com>, 1996.

# serial 1
# Stephan Kulow: I appended a _KDE against name conflicts

dnl AM_PATH_PROG_WITH_TEST_KDE(VARIABLE, PROG-TO-CHECK-FOR,
dnl   TEST-PERFORMED-ON-FOUND_PROGRAM [, VALUE-IF-NOT-FOUND [, PATH]])
AC_DEFUN([AM_PATH_PROG_WITH_TEST_KDE],
[# Extract the first word of "$2", so it can be a program name with args.
    set dummy $2; ac_word=[$]2
    AC_MSG_CHECKING([for $ac_word])
    AC_CACHE_VAL(ac_cv_path_$1,
        [case "[$]$1" in
          /*) ac_cv_path_$1="[$]$1" # Let the user override the test with a path.
              ;;
          *)
              IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:"
              for ac_dir in ifelse([$5], , $PATH, [$5]); do
                test -z "$ac_dir" && ac_dir=.
                if test -f $ac_dir/$ac_word; then
                  if [$3]; then
                    ac_cv_path_$1="$ac_dir/$ac_word"
                    break
                  fi
                fi
              done
              IFS="$ac_save_ifs"
              dnl If no 4th arg is given, leave the cache variable unset,
              dnl so AC_PATH_PROGS will keep looking.
              ifelse([$4], , , [  test -z "[$]ac_cv_path_$1" && ac_cv_path_$1="$4"
                    ])
              ;;
         esac
        ])
    $1="$ac_cv_path_$1"
    if test -n "[$]$1"; then
      AC_MSG_RESULT([$]$1)
    else
      AC_MSG_RESULT(no)
    fi
    AC_SUBST($1)dnl
])



AC_DEFUN([AM_DISABLE_LIBRARIES],
[
    AC_PROVIDE([AM_ENABLE_STATIC])
    AC_PROVIDE([AM_ENABLE_SHARED])
    enable_static=no
    enable_shared=no
])






# Check whether LC_MESSAGES is available in <locale.h>.
# Ulrich Drepper <drepper@cygnus.com>, 1995.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU General Public
# License or the GNU Library General Public License but which still want
# to provide support for the GNU gettext functionality.
# Please note that the actual code of the GNU gettext library is covered
# by the GNU Library General Public License, and the rest of the GNU
# gettext package package is covered by the GNU General Public License.
# They are *not* in the public domain.

# serial 2

AC_DEFUN([AM_LC_MESSAGES],
[
    if test $ac_cv_header_locale_h = yes; then
    AC_CACHE_CHECK([for LC_MESSAGES], am_cv_val_LC_MESSAGES,
      [AC_TRY_LINK([#include <locale.h>], [return LC_MESSAGES],
       am_cv_val_LC_MESSAGES=yes, am_cv_val_LC_MESSAGES=no)])
    if test $am_cv_val_LC_MESSAGES = yes; then
      AC_DEFINE(HAVE_LC_MESSAGES, 1,
        [Define if your <locale.h> file defines LC_MESSAGES.])
    fi
  fi
])


dnl AM_PATH_LINUX([DEFAULT PATH, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl define LINUX_PATH and LINUX_CFLAGS
AC_DEFUN([AM_PATH_LINUX],
[
    AC_ARG_WITH(linux_prefix, [  --with-linux-prefix=PFX where are linux sources (=/usr/src/linux)],
	        [], with_linux_prefix=ifelse([$1], [], /usr/src/linux, $1))
    if test -f $with_linux_prefix/include/linux/modversions.h ; then
	LINUX_CFLAGS="-D__KERNEL__ -DMODULE -I$with_linux_prefix/include -include $with_linux_prefix/include/linux/modversions.h"
	LINUX_PREFIX=$with_linux_prefix
    else
        LINUX_CFLAGS=""
        LINUX_PREFIX=""
    fi
    AC_SUBST(LINUX_CFLAGS)
    AC_SUBST(LINUX_PREFIX)

    if test -n "$LINUX_PREFIX"; then
        ifelse([$2], [], [:], [$2])
    else
        ifelse([$3], [], [:], [$3])
    fi
])

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

# Configure paths for libvorbis
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl AM_PATH_VORBIS([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libvorbis, and define VORBIS_CFLAGS and VORBIS_LIBS
dnl
AC_DEFUN([AM_PATH_VORBIS],
[dnl
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(vorbis-prefix,[  --with-vorbis-prefix=PFX prefix where libvorbis is installed. (optional)], vorbis_prefix="$withval", vorbis_prefix="")
AC_ARG_ENABLE(vorbistest, [  --disable-vorbistest    do not try to compile and run a test Vorbis program.],, enable_vorbistest=yes)

  if test "x$vorbis_prefix" != "xNONE" ; then
    vorbis_args="$vorbis_args --prefix=$vorbis_prefix"
    VORBIS_CFLAGS="-I$vorbis_prefix/include"
    VORBIS_LIBDIR="-L$vorbis_prefix/lib"
  elif test "$prefix" != ""; then
    vorbis_args="$vorbis_args --prefix=$prefix"
    VORBIS_CFLAGS="-I$prefix/include"
    VORBIS_LIBDIR="-L$prefix/lib"
  fi

  VORBIS_LIBS="$VORBIS_LIBDIR -lvorbis -lm"
  VORBISFILE_LIBS="-lvorbisfile"
  VORBISENC_LIBS="-lvorbisenc"

  AC_MSG_CHECKING(for Vorbis)
  no_vorbis=""


  if test "x$enable_vorbistest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $VORBIS_CFLAGS"
    LIBS="$LIBS $VORBIS_LIBS $OGG_LIBS"
dnl
dnl Now check if the installed Vorbis is sufficiently new.
dnl
      rm -f conf.vorbistest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis/codec.h>

int main ()
{
  system("touch conf.vorbistest");
  return 0;
}

],, no_vorbis=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_vorbis" = "x" ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])
  else
     AC_MSG_RESULT(no)
     if test -f conf.vorbistest ; then
       :
     else
       echo "*** Could not run Vorbis test program, checking why..."
       CFLAGS="$CFLAGS $VORBIS_CFLAGS"
       LIBS="$LIBS $VORBIS_LIBS $OGG_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <vorbis/codec.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding Vorbis or finding the wrong"
       echo "*** version of Vorbis. If it is not finding Vorbis, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means Vorbis was incorrectly installed"
       echo "*** or that you have moved Vorbis since it was installed." ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     VORBIS_CFLAGS=""
     VORBIS_LIBS=""
     VORBISFILE_LIBS=""
     VORBISENC_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(VORBIS_CFLAGS)
  AC_SUBST(VORBIS_LIBS)
  AC_SUBST(VORBISFILE_LIBS)
  AC_SUBST(VORBISENC_LIBS)
  rm -f conf.vorbistest
])

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

