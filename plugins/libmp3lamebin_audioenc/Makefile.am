if AMM_USE_MP3LAMEBIN
pkglib_LTLIBRARIES = mp3lamebin_audioenc.la
endif

if AMM_LINK_SHARED
LINK_AVIPLAY = ../../lib/libaviplay.la
endif

noinst_HEADERS = fillplugins.h lame.h
mp3lamebin_audioenc_la_SOURCES = lameencoder.cpp
mp3lamebin_audioenc_la_LIBADD = $(LINK_AVIPLAY) -lm
mp3lamebin_audioenc_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS = $(LTNOPIC)
AM_CXXFLAGS = $(CXXRTTIEXCEPT)

MAINTAINERCLEANFILES = Makefile.in
#
# only lame.h header from the whole LAME project is here
#
# in case something would be changed new version has to used
# otherwise this plugin would be unable to use runtime library
# libmp3lame.so (which should be somewhere in your library path)
#
