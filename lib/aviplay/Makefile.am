noinst_LTLIBRARIES = libaviplay.la
noinst_HEADERS = \
 AudioCleaner.h \
 AudioFpHQResampler.h \
 AudioIntResampler.h \
 AudioQueue.h \
 aviplay_impl.h \
 CopyAudioRenderer.h \
 IAudioRenderer.h \
 IAudioResampler.h \
 OssAudioRenderer.h \
 SdlAudioRenderer.h \
 SunAudioRenderer.h \
 Statistic.h

libaviplay_la_SOURCES = \
 AudioCleaner.cpp \
 AudioFpHQResampler.cpp \
 AudioIntResampler.cpp \
 AudioQueue.cpp \
 aviplay.cpp \
 aviplay2.cpp \
 aviplayprop.cpp \
 aviplayth.cpp \
 CopyAudioRenderer.cpp \
 IAudioRenderer.cpp \
 Statistic.cpp \
 SunAudioRenderer.cpp \
 $(OSS_SRC) $(SDL_SRC)

if AMM_USE_OSS
OSS_SRC = OssAudioRenderer.cpp
else
OSS_SRC =
endif

if AMM_USE_SDL
SDL_SRC = SdlAudioRenderer.cpp
else
SDL_SRC =
endif

EXTRA_DIST = OssAudioRenderer.cpp SdlAudioRenderer.cpp

libaviplay_la_LIBADD = $(I386_LDADD)

AM_CPPFLAGS = $(LTNOPIC) $(SDL_CFLAGS) -I$(srcdir)/../subtitles -I$(srcdir)/../../drivers

# just for testing speed
#check_PROGRAMS = hqtest
#hqtest_SOURCES = AudioFpHQResampler.cpp hqtest.cpp
#hqtest_LDFLAGS = ../libaviplay.la $(SDL_LDADD)

#res_SOURCES = ResampleSinc.cpp
CLEANFILES = hqtest

MAINTAINERCLEANFILES = Makefile.in
