SUBDIRS = lame3.70

if AMM_USE_MP3LAME
pkglib_LTLIBRARIES = mp3lame_audioenc.la
endif

if AMM_LINK_SHARED
LINK_AVIPLAY = ../../lib/libaviplay.la
endif

noinst_HEADERS = fillplugins.h
mp3lame_audioenc_la_SOURCES = mp3encoder.cpp
mp3lame_audioenc_la_LIBADD = lame3.70/libmp3lame.la $(LINK_AVIPLAY)
mp3lame_audioenc_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS = $(LTNOPIC)
AM_CXXFLAGS = $(CXXRTTIEXCEPT)

MAINTAINERCLEANFILES = Makefile.in
