pkglib_LTLIBRARIES = ffmpeg.la

if AMM_LINK_SHARED
LINK_AVIPLAY = ../../lib/libaviplay.la
endif

noinst_HEADERS = FFAudioDecoder.h FFVideoDecoder.h FFVideoEncoder.h fillplugins.h
ffmpeg_la_SOURCES = \
 libffmpeg.cpp \
 FFAudioDecoder.cpp \
 FFVideoDecoder.cpp \
 FFVideoEncoder.cpp

ffmpeg_la_LIBADD = $(LINK_AVIPLAY)
#ffmpeg_la_LIBADD = ../../ffmpeg/libavcodec/libavcodec.a
ffmpeg_la_LDFLAGS = -module -avoid-version

#KPROF_LIBS = -L/var/tmp/kprof/lib -lkprof
#INSTRUMENT = -finstrument-functions
AM_CPPFLAGS = $(LTNOPIC) $(INSTRUMENT) \
 -I$(srcdir)/../../ffmpeg/libavcodec \
 -I$(srcdir)/../../ffmpeg/libavformat \
 -I$(srcdir)/../../ffmpeg/libavutil \
 -I$(srcdir)/../../ffmpeg
AM_CXXFLAGS = $(CXXRTTIEXCEPT)

TEST_LIBS = ../../lib/libaviplay.la $(pkglib_LTLIBRARIES) $(SDL_LIBS) $(MPATROL_LIBS) $(EFENCE_LIBS) $(KPROF_LIBS)
check_PROGRAMS = testvideo
testvideo_SOURCES = testvideo.cpp
testvideo_LDFLAGS = -g -export-dynamic $(TEST_LIBS)

MAINTAINERCLEANFILES = Makefile.in
