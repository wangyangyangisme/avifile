AUTOMAKE_OPTIONS = no-dependencies

noinst_LTLIBRARIES = libavcodecppc.la

PPC_SOURCES = \
 dsputil_altivec.c \
 dsputil_h264_altivec.c \
 dsputil_ppc.c \
 fdct_altivec.c \
 fft_altivec.c \
 gmc_altivec.c \
 idct_altivec.c \
 mpegvideo_altivec.c \
 mpegvideo_ppc.c

noinst_HEADERS = $(PPC_SOURCES) \
 dsputil_altivec.h \
 dsputil_h264_template_altivec.c \
 dsputil_ppc.h \
 gcc_fixes.h

if AMM_FF_PPCOPT
libavcodecppc_la_SOURCES = $(PPC_SOURCES)
endif

AM_CPPFLAGS = $(LTNOPIC) -DHAVE_AV_CONFIG_H -I$(srcdir)/../../libavutil -I$(srcdir)/../..

MAINTAINERCLEANFILES = Makefile.in
