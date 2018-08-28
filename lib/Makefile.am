DIST_SUBDIRS = aviplay aviread aviwrite common subtitle video
noinst_HEADERS = Uncompressed.h

COMMONCFLAGS = $(DIVX4_CFLAGS) $(XVID_CFLAGS) -DPLUGIN_PATH=\"$(pkglibdir)\" \
 -I$(srcdir)/../ffmpeg/libavcodec \
 -I$(srcdir)/../ffmpeg/libavformat \
 -I$(srcdir)/../ffmpeg/libavutil
if AMM_USE_FAST_BUILD
AVISOURCES = libaviplay.cpp
AVILIBADD =
SUBDIRS =
AVICPPFLAGS = $(SDL_CFLAGS) $(X_CFLAGS) $(COMMONCFLAGS) $(PTHREAD_CFLAGS) -I$(srcdir)/aviread
else
AVISOURCES = codeckeeper.cpp Uncompressed.cpp
SUBDIRS = aviplay aviread aviwrite common subtitle video
ADD_LIBS = \
 aviplay/libaviplay.la \
 aviread/libaviread.la \
 aviwrite/libaviwrite.la \
 common/libcommon.la \
 subtitle/libsubtitle.la \
 video/libvideo.la
AVICPPFLAGS = $(COMMONCFLAGS)
endif

AM_CPPFLAGS = $(LTNOPIC) $(AVICPPFLAGS)
EXTRA_DIST = codeckeeper.cpp Uncompressed.cpp libaviplay.cpp

lib_LTLIBRARIES = libaviplay.la
libaviplay_la_SOURCES = $(AVISOURCES)
libaviplay_la_LIBADD = $(ADD_LIBS) $(PTHREAD_LIBS)
libaviplay_la_LDFLAGS = -release $(AVIFILE_MAJOR_VERSION).$(AVIFILE_MINOR_VERSION) -version-info 0:$(AVIFILE_MICRO_VERSION):0
# speedup compilation of codekeeper.cpp - it's not necessary to have
# this code fast anyway - runs only few times
AM_CXXFLAGS = $(CINLINEFLAGS)

MAINTAINERCLEANFILES = Makefile.in
