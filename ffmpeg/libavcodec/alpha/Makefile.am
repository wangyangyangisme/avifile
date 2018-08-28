AUTOMAKE_OPTIONS = no-dependencies

noinst_LTLIBRARIES = libavcodecalpha.la

ALPHA_SOURCES = \
 dsputil_alpha.c \
 motion_est_alpha.c \
 mpegvideo_alpha.c \
 simple_idct_alpha.c
 
noinst_HEADERS = $(ALPHA_SOURCES) \
 asm.h \
 dsputil_alpha_asm.S \
 motion_est_mvi_asm.S \
 regdef.h

if AMM_FF_ALPHAOPT
libavcodecalpha_la_SOURCES = $(ALPHA_SOURCES)
endif

AM_CPPFLAGS = $(LTNOPIC) -DHAVE_AV_CONFIG_H -I$(srcdir)/../../libavutil -I$(srcdir)/..

MAINTAINERCLEANFILES = Makefile.in
