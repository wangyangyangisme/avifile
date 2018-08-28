if AMM_USE_WIN32
noinst_LTLIBRARIES = libaudiodec.la
endif

noinst_HEADERS = \
 ACM_AudioDecoder.h \
 DMO_AudioDecoder.h \
 DS_AudioDecoder.h

libaudiodec_la_SOURCES = \
 ACM_AudioDecoder.cpp \
 DMO_AudioDecoder.cpp \
 DS_AudioDecoder.cpp

AM_CPPFLAGS = -I$(srcdir)/../loader -I$(srcdir)/../loader/dmo -I$(srcdir)/../loader/dshow -I$(srcdir)/..
AM_CXXFLAGS = $(CXXRTTIEXCEPT)

MAINTAINERCLEANFILES = Makefile.in
