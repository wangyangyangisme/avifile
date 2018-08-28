pkglib_LTLIBRARIES = mpeg_audiodec.la

if AMM_LINK_SHARED
LINK_AVIPLAY = ../../lib/libaviplay.la
endif

mpeg_audiodec_la_SOURCES = bitwindow.cpp filter.cpp filter_2.cpp \
	huffmantable.cpp mpeglayer1.cpp mpeglayer2.cpp mpeglayer3.cpp \
	mpegtable.cpp mpegtoraw.cpp
mpeg_audiodec_la_LIBADD = $(LINK_AVIPLAY)
mpeg_audiodec_la_LDFLAGS = -module -avoid-version

noinst_HEADERS = mpegsound.h fillplugins.h

# currently it has to be compiled with optimalizations (inline has to work)
#-D__NO_STRING_INLINES -DDEBUG

AM_CPPFLAGS = $(LTNOPIC)
AM_CXXFLAGS = $(CXXRTTIEXCEPT)

MAINTAINERCLEANFILES = Makefile.in
