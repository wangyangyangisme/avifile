SUBDIRS = loader audiodec videocodec

if AMM_USE_WIN32
pkglib_LTLIBRARIES = win32.la
check_PROGRAMS = test testaudio testvideo testcodecs testprepare testloader
endif

if AMM_LINK_SHARED
LINK_AVIPLAY = ../../lib/libaviplay.la
# this looks like bad idea - causes some weird crashes
#else
#LINK_AVIPLAY = ../../lib/common/libcommon.la
endif

noinst_HEADERS = \
 fillplugins.h \
 libwin32.h

win32_la_SOURCES = libwin32.cpp

win32_la_LIBADD = \
 audiodec/libaudiodec.la \
 loader/libloader.la \
 videocodec/libvideocodec.la $(LINK_AVIPLAY)

win32_la_LDFLAGS = -module -avoid-version

#
# Comment out these defines if you do not have mpatrol installed
#
#MPATROL_LIBS = -L/var/tmp/mpatrol/lib -lmpatrol -lbfd -lelf -liberty
#MPATROL_CPPFLAGS = -I/var/tmp/mpatrol/include  -DUSE_MPATROL
#EFENCE_LIBS = -lefence
#$(pkglib_LTLIBRARIES) 
#TEST_LIBS = ../libffmpeg/ffmpeg.la ../../lib/libaviplay.la $(SDL_LIBS) $(MPATROL_LIBS) $(EFENCE_LIBS)
TEST_LIBS = win32.la ../../lib/libaviplay.la $(SDL_LIBS) $(MPATROL_LIBS) $(EFENCE_LIBS)

test_SOURCES = test.cpp
test_LDFLAGS = $(TEST_LIBS)
testaudio_SOURCES = testaudio.cpp
testaudio_LDFLAGS = $(TEST_LIBS)
testvideo_SOURCES = testvideo.cpp
testvideo_LDFLAGS = $(TEST_LIBS)
testcodecs_SOURCES = testcodecs.cpp
testcodecs_LDFLAGS = $(TEST_LIBS)
testprepare_SOURCES = testprepare.cpp
testprepare_LDFLAGS = $(TEST_LIBS)
testloader_SOURCES = testloader.c
testloader_LDFLAGS = $(TEST_LIBS)

AM_CPPFLAGS = $(LTNOPIC) -I$(srcdir)/loader -I$(srcdir)/loader/dshow \
  -I$(srcdir)/loader/wine -I$(srcdir)/videocodec -I$(srcdir)/audiodec \
  -I$(srcdir)/loader/dmo $(MPATROL_CPPFLAGS)
AM_CXXFLAGS = $(CXXRTTIEXCEPT)
 
# speedup compilation of libwin32.cpp - it's not necessary to have
# this code fast anyway - runs only few times
#AM_CXXFLAGS = $(CINLINEFLAGS) $(CXXRTTIEXCEPT)

MAINTAINERCLEANFILES = Makefile.in mpatrol.*
