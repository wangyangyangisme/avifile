# for testing only - swap S
SSUBDIRS = $(SUBDIRS_VORBIS)

SUBDIRS = \
 libac3pass \
 libaudiodec \
 libdivx4 \
 libffmpeg \
 libmad \
 libmp3lamebin_audioenc \
 libmp3lame_audioenc \
 libmpeg_audiodec \
 libvorbis \
 libwin32 \
 libxvid \
 libxvid4

#noinst_LTLIBRARIES = libplugins.la
#libplugins_la_SOURCES = libplugins.cpp plugins.c

AM_CPPFLAGS = -Ilibac3pass -Ilibwin32 -Ilibwin32/audiodec \
 -Ilibwin32/videocodec -Ilibwin32/loader \
 -Ilibwin32/loader/dshow -I../ffmpeg/libavcodec \
 -DWIN32_PATH=\"$(WIN32_PATH)\" -D__WINE__ -DUNICODE \
 -Ddbg_printf=__vprintf -DTRACE=__vprintf \
 -Wmissing-prototypes -Wimplicit-function-declaration \
 -DAVIFILE

MAINTAINERCLEANFILES = Makefile.in
