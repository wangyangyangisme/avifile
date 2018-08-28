AUTOMAKE_OPTIONS = no-dependencies

noinst_LTLIBRARIES = libavcodecsh4.la

SH4_SOURCES = \
 dsputil_sh4.c \
 idct_sh4.c \
 qpel.c

noinst_HEADERS = $(SH4_SOURCES) \
 dsputil_align.c

if AMM_FF_SH4OPT
libavcodecsh4_la_SOURCES = $(SH4_SOURCES)
endif

AM_CPPFLAGS = $(LTNOPIC) -DHAVE_AV_CONFIG_H -I$(srcdir)/../..

MAINTAINERCLEANFILES = Makefile.in
