if AMM_USE_DIVX
pkglib_LTLIBRARIES = divx4.la
check_PROGRAMS = test 
endif

if AMM_LINK_SHARED
LINK_AVIPLAY = ../../lib/libaviplay.la
endif

if AMM_USE_DIVXBIN
FLG_DIVX = -DDIVXBIN
else
LINK_DIVX_LIBS = $(DIVX_LIBS)
endif

divx4_la_SOURCES = divx4.cpp
divx4_la_LIBADD = $(LINK_DIVX_LIBS) $(LINK_AVIPLAY)
divx4_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS = $(LTNOPIC) $(FLG_DIVX) -DLINUX $(DIVX_CFLAGS)
AM_CXXFLAGS = $(CXXRTTIEXCEPT)

noinst_HEADERS = fillplugins.h
test_SOURCES = test.cpp
test_LDFLAGS = ../../lib/libaviplay.la $(SDL_LIBS) $(DIVX_LIBS)

MAINTAINERCLEANFILES = Makefile.in
