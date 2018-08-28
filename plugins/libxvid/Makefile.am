if AMM_USE_XVID
pkglib_LTLIBRARIES = xvid.la
endif

if AMM_LINK_SHARED
LINK_AVIPLAY = ../../lib/libaviplay.la
endif

noinst_HEADERS = fillplugins.h
xvid_la_SOURCES = xvid.cpp
xvid_la_LIBADD = $(XVID_LIBS) $(LINK_AVIPLAY)
xvid_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS = $(LTNOPIC) $(XVID_CFLAGS)
AM_CXXFLAGS = $(CXXRTTIEXCEPT)

MAINTAINERCLEANFILES = Makefile.in
