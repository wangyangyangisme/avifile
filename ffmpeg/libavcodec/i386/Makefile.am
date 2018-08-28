#AUTOMAKE_OPTIONS = no-dependencies

noinst_LTLIBRARIES = libavcodeci386.la

I386_SOURCES = \
 cputest.c \
 dsputil_mmx.c \
 fdct_mmx.c \
 fft_sse.c \
 idct_mmx.c \
 motion_est_mmx.c \
 mpegvideo_mmx.c \
 simple_idct_mmx.c \
 vp3dsp_mmx.c \
 vp3dsp_sse2.c

noinst_HEADERS = $(I386_SOURCES) \
 dsputil_h264_template_mmx.c \
 dsputil_mmx_avg.h \
 dsputil_mmx_rnd.h \
 h264dsp_mmx.c \
 mmx.h \
 mpegvideo_mmx_template.c

if AMM_FF_X86OPT
libavcodeci386_la_SOURCES = $(I386_SOURCES)
endif

AM_CPPFLAGS = $(LTNOPIC) -DHAVE_AV_CONFIG_H -I$(srcdir)/../../libavutil -I$(srcdir)/..

# without -O2 compiler may complain about 'fixed or forbidden register spilled'
#INSTRUMENT = -finstrument-functions
CFLAGS = $(FFMPEG_CFLAGS) -O2 $(INSTRUMENT)

MAINTAINERCLEANFILES = Makefile.in
#SUFFIXES = .s
#STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh
# not needed for now
#%.lo: %.s
#	$(LIBTOOL) --mode=compile @TAGCXX@ $(STRIP_FPIC) nasm -f elf $<
#	nasm -f elf -o $@ $<

fft_sse.lo: fft_sse.c
	$(LTCOMPILE) $(FFMPEG_SSE) -c $<
