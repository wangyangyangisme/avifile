#AUTOMAKE_OPTIONS = no-dependencies

DIST_SUBDIRS = alpha armv4l i386 liba52 libpostproc mlib ppc ps2 sh4 sparc

if AMM_FF_FAAD
FAAD_SRC = faad.c
endif

# write  bin version of ogg.c
#if AMM_USE_VORBIS
#OGGSRC = oggvorbis.c
#endif

noinst_HEADERS = \
 ac3.h \
 ac3tab.h \
 avcodec.h \
 bitstream.h \
 cabac.h \
 cookdata.h \
 dsputil.h \
 dvdata.h \
 faandct.h \
 golomb.h \
 h261data.h \
 h263data.h \
 h264data.h \
 imgconvert_template.h \
 indeo2data.h \
 indeo3data.h \
 lzo.h \
 mdec.c \
 motion_est_template.c \
 mpeg12data.h \
 mpeg4data.h \
 mpegaudio.h \
 mpegaudiodectab.h \
 mpegaudiotab.h \
 mpegvideo.h \
 msmpeg4data.h \
 opt.h \
 qdm2data.h \
 ra144.h \
 ra288.h \
 rangecoder.h \
 simple_idct.h \
 sp5x.h \
 svq1_cb.h \
 svq1_vlc.h \
 svq3.c \
 truemotion1data.h \
 truespeech_data.h \
 ulti_cb.h \
 vc9data.h \
 vorbis.h \
 vp3data.h \
 wmadata.h \
 wmv2.c

AVCODEC_SRC = $(FAAD_SRC) \
 4xm.c \
 8bps.c \
 a52dec.c \
 aasc.c \
 ac3enc.c \
 adpcm.c \
 adx.c \
 alac.c \
 allcodecs.c \
 asv1.c \
 avcodec.c \
 avs.c \
 bitstream.c \
 bmp.c \
 cabac.c \
 cinepak.c \
 cljr.c \
 cook.c \
 cscd.c \
 cyuv.c \
 dpcm.c \
 dsputil.c \
 dv.c \
 dvbsub.c \
 dvbsubdec.c \
 dvdsub.c \
 dvdsubenc.c \
 error_resilience.c \
 faandct.c \
 fft.c \
 ffv1.c \
 flac.c \
 flicvideo.c \
 fraps.c \
 g726.c \
 golomb.c \
 h261.c \
 h263.c \
 h263dec.c \
 h264.c \
 h264idct.c \
 huffyuv.c \
 idcinvideo.c \
 imgconvert.c \
 imgresample.c \
 indeo2.c \
 indeo3.c \
 interplayvideo.c \
 jfdctfst.c \
 jfdctint.c \
 jrevdct.c \
 lcl.c \
 loco.c \
 lzo.c \
 mace.c \
 mdct.c \
 mem.c \
 mjpeg.c \
 mmvideo.c \
 motion_est.c \
 mpeg12.c \
 mpegaudio.c \
 mpegaudiodec.c \
 mpegvideo.c \
 msmpeg4.c \
 msrle.c \
 msvideo1.c \
 opt.c \
 parser.c \
 pcm.c \
 png.c \
 pnm.c \
 pthread.c \
 qdm2.c \
 qdrw.c \
 qpeg.c \
 qtrle.c \
 ra144.c \
 ra288.c \
 rangecoder.c \
 ratecontrol.c \
 raw.c \
 resample.c \
 resample2.c \
 roqvideo.c \
 rpza.c \
 rv10.c \
 shorten.c \
 simple_idct.c \
 smc.c \
 snow.c \
 sonic.c \
 svq1.c \
 truemotion1.c \
 truemotion2.c \
 truespeech.c \
 tscc.c \
 tta.c \
 ulti.c \
 utils.c \
 vc9.c \
 vcr1.c \
 vmdav.c \
 vorbis.c \
 vp3.c \
 vp3dsp.c \
 vqavideo.c \
 wmadec.c \
 wnv1.c \
 ws-snd1.c \
 xan.c \
 xl.c \
 zmbv.c

# $(OGGSRC)

#if AMM_USE_FAST_BUILD
#libaviplayavcodec_la_SOURCES = libavcodec.c
#libaviplayavcodec_la_LIBADD = $(LIBA52)
#SUBDIRS = $(SUBDIRS_LIBA52)
#else

SUBDIRS = $(DIST_SUBDIRS)

ADD_LIBS = \
 alpha/libavcodecalpha.la \
 armv4l/libavcodecarmv4l.la \
 i386/libavcodeci386.la \
 liba52/liba52.la \
 mlib/libavcodecmlib.la \
 ppc/libavcodecppc.la \
 ps2/libavcodecps2.la \
 sh4/libavcodecsh4.la \
 sparc/libavcodecsparc.la

#endif

if AMM_LINK_SHARED
lib_LTLIBRARIES = libaviplayavcodec.la
else
noinst_LTLIBRARIES = libavcodec.la
endif

libaviplayavcodec_la_SOURCES = $(AVCODEC_SRC)
libaviplayavcodec_la_LIBADD = $(ADD_LIBS)
libaviplayavcodec_la_LDFLAGS = -release $(AVIFILE_MAJOR_VERSION).$(AVIFILE_MINOR_VERSION) -version-info 0:$(AVIFILE_MICRO_VERSION):0

libavcodec_la_SOURCES = $(AVCODEC_SRC)
libavcodec_la_LIBADD = $(ADD_LIBS)

EXTRA_DIST = \
 amr.c \
 apiexample.c \
 beosthread.c \
 dct-test.c \
 dtsdec.c \
 eval.c \
 faac.c \
 faad.c \
 fdctref.c \
 fft-test.c \
 jpeg_ls.c \
 libgsm.c \
 Makefile.ffmpeg \
 motion_test.c \
 mp3lameaudio.c \
 oggtheora.c \
 oggvorbis.c \
 opts.c \
 x264.c \
 xvidff.c \
 xvid_rc.c \
 xvmcvideo.c \
 w32thread.c

AM_CPPFLAGS = $(LTNOPIC) -DHAVE_AV_CONFIG_H -DHAVE_THREADS -I$(srcdir)/../libavutil -I$(srcdir)/..

#INSTRUMENT = -finstrument-functions
CFLAGS = $(FFMPEG_CFLAGS) $(INSTRUMENT)
#-fno-omit-frame-pointer -O0

check_PROGRAMS = apiexample
apiexample_SOURCES = apiexample.c
if AMM_LINK_SHARED
AVUTIL = libaviplayavutil.la
else
AVUTIL = libavutil.la
endif
apiexample_LDADD =  ../libavutil/$(AVUTIL) $(lib_LTLIBRARIES) $(noinst_LTLIBRARIES) $(Z_LIBS) $(PTHREAD_LIBS)

mostlyclean-generic:
	-rm -rf doxy

CLEANFILES = .depend *.a *.la
MAINTAINERCLEANFILES = Makefile.in
