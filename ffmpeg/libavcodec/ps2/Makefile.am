AUTOMAKE_OPTIONS = no-dependencies

noinst_LTLIBRARIES = libavcodecps2.la

PS2_SOURCES = \
 dsputil_mmi.c \
 idct_mmi.c \
 mpegvideo_mmi.c

noinst_HEADERS = $(PS2_SOURCES) \
 mmi.h

if AMM_FF_PS2OPT
libavcodecps2_la_SOURCES = $(PS2_SOURCES)
endif

AM_CPPFLAGS = $(LTNOPIC) -DHAVE_AV_CONFIG_H -I$(srcdir)/../..

MAINTAINERCLEANFILES = Makefile.in
