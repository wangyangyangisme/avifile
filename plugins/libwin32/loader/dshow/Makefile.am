if AMM_USE_WIN32
noinst_LTLIBRARIES = libdshow.la
endif

noinst_HEADERS = \
 allocator.h \
 cmediasample.h \
 DS_Filter.h \
 guids.h \
 inputpin.h \
 interfaces.h \
 iunk.h \
 outputpin.h

libdshow_la_SOURCES = \
 allocator.c \
 cmediasample.c \
 DS_Filter.c \
 guids.c \
 inputpin.c \
 outputpin.c

AM_CPPFLAGS = $(LTNOPIC) -DAVIFILE -I$(srcdir)/..
CFLAGS = $(WIN32_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in
