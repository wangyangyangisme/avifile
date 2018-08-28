SUBDIRS = drivers

noinst_HEADERS = fourcc.h vidix.h vidixlib.h
EXTRA_DIST = README vidix.txt
VIDIX_SRC = vidixlib.c

if AMM_USE_VIDIX
if AMM_LINK_SHARED
lib_LTLIBRARIES = libaviplayvidix.la
else
noinst_LTLIBRARIES = libvidix.la
endif
endif

libaviplayvidix_la_SOURCES = $(VIDIX_SRC)
libaviplayvidix_la_LIBADD = ../libdha/libaviplaydha.la
libaviplayvidix_la_LDFLAGS = -release $(AVIFILE_MAJOR_VERSION).$(AVIFILE_MINOR_VERSION) -version-info 0:$(AVIFILE_MICRO_VERSION):0

libvidix_la_SOURCES = $(VIDIX_SRC)
libvidix_la_LIBADD = ../libdha/libdha.la

MAINTAINERCLEANFILES = Makefile.in
