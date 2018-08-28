if AMM_USE_XVID4
pkglib_LTLIBRARIES = xvid4.la
endif

if AMM_LINK_SHARED
LINK_AVIPLAY = ../../lib/libaviplay.la
endif

noinst_HEADERS = fillplugins.h
xvid4_la_SOURCES = xvid4.cpp
xvid4_la_LIBADD = $(XVID4_LIBS) $(LINK_AVIPLAY)
xvid4_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS = $(LTNOPIC) $(XVID1_CFLAGS)
AM_CXXFLAGS = $(CXXRTTIEXCEPT)

MAINTAINERCLEANFILES = Makefile.in
