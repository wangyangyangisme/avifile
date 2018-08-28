# libmad audio mpeg decoder pluging
# using either internal version 0.14.0b
# or system library
SUBDIRS = libmad

if AMM_USE_LIBMAD
CUSTOMFLAGS =
CUSTOMLIBS = $(MAD_LIBS)
else
CUSTOMFLAGS = -I$(SUBDIRS)
CUSTOMLIBS = $(SUBDIRS)/libmad.la
endif

if AMM_USE_MAD
pkglib_LTLIBRARIES = mad_audiodec.la
endif

if AMM_LINK_SHARED
LINK_AVIPLAY = ../../lib/libaviplay.la
endif

noinst_HEADERS = fillplugins.h
mad_audiodec_la_SOURCES = libmad.cpp
mad_audiodec_la_LIBADD = $(CUSTOMLIBS) $(LINK_AVIPLAY)
mad_audiodec_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS = $(LTNOPIC) $(CUSTOMFLAGS)
AM_CXXFLAGS = $(CXXRTTIEXCEPT)

MAINTAINERCLEANFILES = Makefile.in
