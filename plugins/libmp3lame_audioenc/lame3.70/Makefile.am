if AMM_USE_MP3LAME
noinst_LTLIBRARIES = libmp3lame.la
endif

libmp3lame_la_SOURCES = \
 brhist.c \
 formatBitstream.c \
 fft.c \
 get_audio.c \
 l3bitstream.c \
 id3tag.c \
 ieeefloat.c \
 lame.c \
 newmdct.c \
 parse.c \
 portableio.c \
 psymodel.c \
 quantize.c \
 quantize-pvt.c \
 vbrquantize.c \
 reservoir.c \
 tables.c \
 takehiro.c \
 timestatus.c \
 util.c \
 VbrTag.c \
 version.c \
 common.c \
 dct64_i386.c \
 decode_i386.c \
 layer3.c \
 tabinit.c \
 interface.c \
 main.c 

noinst_HEADERS = \
 VbrTag.h brhist.h encoder.h fft.h formatBitstream.h\
 get_audio.h gpkplotting.h gtkanal.h huffman.h id3tag.h ieeefloat.h\
 l3bitstream-pvt.h l3bitstream.h l3side.h lame.h machine.h mpg123.h\
 mpglib.h newmdct.h portableio.h psymodel.h quantize-pvt.h quantize.h\
 reservoir.h rtp.h tables.h timestatus.h util.h version.h 

# for profiling - currently not to much useful 
#CFLAGS = -ffast-math -funroll-loops -finline-functions @CFLAGS@

# 10/99 added -D__NO_MATH_INLINES to fix a bug in *all* versions of
# gcc 2.8+ as of 10/99.  
AM_CPPFLAGS = $(LTNOPIC) -DLAMEPARSE -DNDEBUG -D__NO_MATH_INLINES 


MAINTAINERCLEANFILES = Makefile.in
