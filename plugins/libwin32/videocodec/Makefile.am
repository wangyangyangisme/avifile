if AMM_USE_WIN32
noinst_LTLIBRARIES = libvideocodec.la
endif

noinst_HEADERS = \
 DMO_VideoDecoder.h \
 DS_VideoDecoder.h \
 Module.h \
 VideoDecoder.h \
 VideoEncoder.h

libvideocodec_la_SOURCES = \
 DMO_VideoDecoder.cpp \
 DS_VideoDecoder.cpp \
 VideoEncoder.cpp \
 VideoDecoder.cpp \
 Module.cpp

AM_CPPFLAGS = $(LTNOPIC) -I$(srcdir)/../loader
AM_CXXFLAGS = $(CXXRTTIEXCEPT)

MAINTAINERCLEANFILES = Makefile.in
