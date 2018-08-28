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
