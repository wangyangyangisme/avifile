AUTOMAKE_OPTIONS = no-dependencies

if AMM_USE_OSS
AUDIO_SRC = audio.c
endif

if AMM_USE_V4L
GRAB_SRC = grab.c v4l2.c
endif

AVFORMAT_SRC = \
 4xm.c \
 adtsenc.c \
 aiff.c \
 allformats.c \
 asf-enc.c \
 asf.c \
 au.c \
 avidec.c \
 avienc.c \
 avio.c \
 aviobuf.c \
 avs.c \
 crc.c \
 cutils.c \
 daud.c \
 dv.c \
 dv1394.c \
 electronicarts.c \
 ffm.c \
 file.c \
 flic.c \
 flvdec.c \
 flvenc.c \
 framehook.c \
 gif.c \
 gifdec.c \
 http.c \
 idcin.c \
 idroq.c \
 img.c \
 img2.c \
 ipmovie.c \
 jpeg.c \
 matroska.c \
 mm.c \
 mmf.c \
 mov.c \
 movenc.c \
 mp3.c \
 mpeg.c \
 mpegts.c \
 mpegtsenc.c \
 mpjpeg.c \
 nsvdec.c \
 nut.c \
 ogg2.c \
 oggparseflac.c \
 oggparsetheora.c \
 oggparsevorbis.c \
 os_support.c \
 png.c \
 pnm.c \
 psxstr.c \
 raw.c \
 rm.c \
 rtp.c \
 rtpproto.c \
 rtsp.c \
 segafilm.c \
 sgi.c \
 sierravmd.c \
 sol.c \
 swf.c \
 tcp.c \
 tta.c \
 udp.c \
 utils.c \
 voc.c \
 wav.c \
 wc3movie.c \
 westwood.c \
 yuv.c \
 yuv4mpeg.c $(AUDIO_SRC) $(GRAB_SRC)

noinst_HEADERS = \
 asf.h \
 avformat.h \
 avi.h \
 avio.h \
 dv.h \
 dv1394.h \
 framehook.h \
 mov.h \
 mpegts.h \
 ogg2.h \
 os_support.h \
 qtpalette.h \
 rtp.h \
 rtsp.h \
 rtspcodes.h \
 voc.h

EXTRA_DIST = \
 Makefile.ffmpeg \
 audio.c \
 barpainet.c \
 barpainet.h \
 beosaudio.cpp \
 dc1394.c \
 grab.c \
 ogg.c

if AMM_LINK_SHARED
lib_LTLIBRARIES = libaviplayavformat.la
else
noinst_LTLIBRARIES = libavformat.la
endif

libaviplayavformat_la_SOURCES = $(AVFORMAT_SRC)
libaviplayavformat_la_LIBADD = $(Z_LIBS)
libaviplayavformat_la_LDFLAGS = -release $(AVIFILE_MAJOR_VERSION).$(AVIFILE_MINOR_VERSION) -version-info 0:$(AVIFILE_MICRO_VERSION):0

libavformat_la_SOURCES = $(AVFORMAT_SRC)
libavformat_la_LIBADD = $(Z_LIBS)

AM_CPPFLAGS = $(LTNOPIC) -DHAVE_AV_CONFIG_H -I$(srcdir)/../libavutil -I$(srcdir)/../libavcodec -I$(srcdir)/..

CLEANFILES = .depend *.a *.la
MAINTAINERCLEANFILES = Makefile.in
