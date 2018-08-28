AUTOMAKE_OPTIONS = no-dependencies

noinst_LTLIBRARIES = liba52.la

A52_SOURCES = \
 bitstream.c \
 bit_allocate.c \
 crc.c \
 downmix.c \
 imdct.c \
 parse.c

noinst_HEADERS = $(A52_SOURCES) \
 a52.h \
 a52_internal.h \
 a52_util.h \
 bitstream.h \
 mm_accel.h \
 resample.c \
 resample_c.c \
 resample_mmx.c \
 tables.h

if AMM_FF_A52BIN
# nothing here
else
liba52_la_SOURCES = $(A52_SOURCES)
endif

liba52_la_LIBADD = -lm

AM_CPPFLAGS = $(LTNOPIC) -DHAVE_AV_CONFIG_H $(LIBA52_CFLAGS) -I$(srcdir)/../../libavutil

MAINTAINERCLEANFILES = Makefile.in
