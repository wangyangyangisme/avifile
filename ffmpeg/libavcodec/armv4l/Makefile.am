AUTOMAKE_OPTIONS = no-dependencies

noinst_LTLIBRARIES = libavcodecarmv4l.la

ARM_SOURCES = \
 dsputil_arm.c \
 dsputil_iwmmxt.c

# would need CCAS in configure.in FIXME
#jrevdct_arm.S

noinst_HEADERS = $(ARM_SOURCES) \
 dsputil_arm_s.S \
 dsputil_iwmmxt_rnd.h \
 jrevdct_arm.S \
 mpegvideo_arm.c

if AMM_FF_ARMOPT
libavcodecarmv4l_la_SOURCES = $(ARM_SOURCES)
endif

AM_CPPFLAGS = $(LTNOPIC) -DHAVE_AV_CONFIG_H -I$(srcdir)/../../libavutil -I$(srcdir)/..

MAINTAINERCLEANFILES = Makefile.in
