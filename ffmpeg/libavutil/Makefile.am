#AUTOMAKE_OPTIONS = no-dependencies

noinst_HEADERS = \
 avutil.h \
 bswap.h \
 common.h \
 integer.h \
 intfloat_readwrite.h \
 mathematics.h \
 rational.h

AVUTIL_SRC = \
 eval.c \
 integer.c \
 intfloat_readwrite.c \
 mathematics.c \
 rational.c

if AMM_LINK_SHARED
lib_LTLIBRARIES = libaviplayavutil.la
else
noinst_LTLIBRARIES = libavutil.la
endif

libaviplayavutil_la_SOURCES = $(AVUTIL_SRC)
libaviplayavutil_la_LIBADD =
libaviplayavutil_la_LDFLAGS = -release $(AVIFILE_MAJOR_VERSION).$(AVIFILE_MINOR_VERSION) -version-info 0:$(AVIFILE_MICRO_VERSION):0

libavutil_la_SOURCES = $(AVUTIL_SRC)
libavutil_la_LIBADD =

AM_CPPFLAGS = $(LTNOPIC) -DHAVE_AV_CONFIG_H -DHAVE_THREADS -I$(srcdir)/../libavcodec -I$(srcdir)/..

#INSTRUMENT = -finstrument-functions
CFLAGS = $(FFMPEG_CFLAGS) $(INSTRUMENT)
#-fno-omit-frame-pointer -O0

CLEANFILES = .depend *.a *.la
MAINTAINERCLEANFILES = Makefile.in
