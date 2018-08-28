if AMM_USE_JPEGLIB
pkglib_LTLIBRARIES = osmjpeg.la
#noinst_LTLIBRARIES=libmjpeg.la
endif

if AMM_LINK_SHARED
LINK_AVIPLAY = ../../lib/libaviplay.la
endif

noinst_HEADERS = colorspace.h mjpeg.h fillplugins.h
osmjpeg_la_SOURCES = main.cpp mjpeg.c 
osmjpeg_la_LDFLAGS = -module -avoid-version
osmjpeg_la_LIBADD = -ljpeg $(LINK_AVIPLAY)

AM_CPPFLAGS = $(LTNOPIC) $(XVID_CFLAGS)
AM_CXXFLAGS = $(CXXRTTIEXCEPT)

MAINTAINERCLEANFILES = Makefile.in
