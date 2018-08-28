AUTOMAKE_OPTIONS = no-dependencies

noinst_LTLIBRARIES = libavcodecmlib.la

MLIB_SOURCES = dsputil_mlib.c

noinst_HEADERS = $(MLIB_SOURCES)

if AMM_FF_MLIBOPT
libavcodecmlib_la_SOURCES = $(MLIB_SOURCES)
endif

AM_CPPFLAGS = $(LTNOPIC) -DHAVE_AV_CONFIG_H -I$(srcdir)/../../libavutil -I$(srcdir)/..

MAINTAINERCLEANFILES = Makefile.in
