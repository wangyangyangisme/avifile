SUBDIRS = libavutil libavformat libavcodec

noinst_HEADERS = \
 ffserver.h \
 xvmc_render.h

EXTRA_DIST = \
 MAINTAINERS \
 build_avopt \
 clean-diff \
 cmdutils.c \
 cmdutils.h \
 common.mak \
 cws2fws.c \
 Doxyfile \
 ffplay.c \
 libavcodec-uninstalled.pc \
 libavcodec.pc \
 libavformat-uninstalled.pc \
 libavformat.pc \
 libpostproc-uninstalled.pc \
 libpostproc.pc \
 Makefile.ffmpeg \
 output_example.c \
 qt-faststart.c \
 unwrap-diff

AM_CPPFLAGS = \
 -I$(srcdir)/libavcodec \
 -I$(srcdir)/libavformat \
 -I$(srcdir)/libavutil -DFFMPEG_CONFIGURATION=\"0.4\"

check_PROGRAMS = ffmpeg ffserver

ffmpeg_SOURCES = ffmpeg.c cmdutils.c
if AMM_LINK_SHARED
ffmpeg_LDADD = libavformat/libaviplayavformat.la libavcodec/libaviplayavcodec.la libavutil/libaviplayavutil.la $(PTHREAD_LIBS)
else
ffmpeg_LDADD = libavformat/libavformat.la libavcodec/libavcodec.la libavutil/libavutil.la $(PTHREAD_LIBS)
endif

ffserver_SOURCES = ffserver.c
ffserver_LDADD = $(ffmpeg_LDADD)

CLEANFILES = .depend config.h config.log *.nsi doc/*.html doc/*.1 doc/Makefile .snprj
MAINTAINERCLEANFILES = Makefile.in config.mak ffmpeg ffplay ffserver \
 ffmpeg_g ffplay_g output_example qt-faststart testvideo

#
# files are 'usually' not modified (sometimes however there are
# some patches sooner then in ffmpeg CVS)
# original Makefiles have been rename to Makefile.ffmpeg
#
