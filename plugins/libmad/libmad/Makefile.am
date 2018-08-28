# nothing to be compiled - system library will be used
noinst_LTLIBRARIES = libmad.la

noinst_HEADERS = mad.h madconfig.h \
  version.h fixed.h bit.h timer.h stream.h frame.h synth.h \
  decoder.h global.h huffman.h layer12.h layer3.h

if AMM_USE_LIBMAD
else
MAD_SRC = \
  fixed.c bit.c timer.c stream.c frame.c synth.c decoder.c  \
  layer12.c layer3.c huffman.c version.c
endif

EXTRA_DIST =  COPYRIGHT Makefile.am.mad acinclude.m4.mad configure.in \
  D.dat imdct_s.dat imdct_l_arm.S \
  qc_table.dat rq_table.dat sf_table.dat

libmad_la_SOURCES = $(MAD_SRC)

AM_CPPFLAGS = $(LTNOPIC) $(CUSTOMFLAGS)

MAINTAINERCLEANFILES = Makefile.in
