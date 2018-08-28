#AUTOMAKE_OPTIONS = no-dependencies
noinst_LTLIBRARIES = libpostproc.la

noinst_HEADERS = \
 postprocess_altivec_template.c \
 postprocess_internal.h \
 postprocess_template.c \
 postprocess.h \
 mangle.h

libpostproc_la_SOURCES = postprocess.c

AM_CPPFLAGS = $(LTNOPIC) -DHAVE_AV_CONFIG_H $(FFMPEG_CFLAGS) -I$(srcdir)/../libavutil -I$(srcdir)/..
AM_CFLAGS = -O2 -fomit-frame-pointer
#-mno-omit-leaf-frame-pointer

CLEANFILES = .depend *.a
MAINTAINERCLEANFILES = Makefile.in
