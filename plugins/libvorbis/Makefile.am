if AMM_USE_VORBIS
pkglib_LTLIBRARIES = vorbis_audio.la
endif

if AMM_LINK_SHARED
LINK_AVIPLAY = ../../lib/libaviplay.la
endif

noinst_HEADERS = fillplugins.h
vorbis_audio_la_SOURCES = libvorbis.cpp
vorbis_audio_la_LIBADD = -lvorbisfile -lvorbis $(LINK_AVIPLAY)
vorbis_audio_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS = $(LTNOPIC)
AM_CXXFLAGS = $(CXXRTTIEXCEPT)

MAINTAINERCLEANFILES = Makefile.in
