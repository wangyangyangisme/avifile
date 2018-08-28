if AMM_USE_WIN32
noinst_LTLIBRARIES = libdmo.la
endif

noinst_HEADERS = \
 DMO_Filter.h \
 dmo.h \
 dmo_guids.h \
 dmo_interfaces.h

libdmo_la_SOURCES = \
 buffer.c \
 dmo.c \
 dmo_guids.c

AM_CPPFLAGS = $(LTNOPIC) -I$(srcdir)/..
CFLAGS = $(WIN32_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in
