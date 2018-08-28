noinst_LTLIBRARIES = libaviread.la
noinst_HEADERS = \
 AsfFileInputStream.h \
 asffmt.h \
 AsfInputStream.h \
 AsfNetworkInputStream.h \
 AsfReadHandler.h \
 AsfReadStream.h \
 AsfRedirectInputStream.h \
 asf_guids.h \
 AsxReader.h \
 avifmt.h \
 AviReadHandler.h \
 AviReadStream.h \
 Cache.h \
 FFReadHandler.h \
 FFReadStream.h \
 ReadHandlers.h \
 ReadStream.h \
 ReadStreamA.h \
 ReadStreamV.h \
 StreamInfoPriv.h

libaviread_la_SOURCES = \
 AsfFileInputStream.cpp \
 asffmt.cpp \
 AsfNetworkInputStream.cpp \
 AsfReadHandler.cpp \
 AsfReadStream.cpp \
 AsfRedirectInputStream.cpp \
 AsfStreamSeekInfo.cpp \
 asf_guids.cpp \
 AsxReader.cpp \
 AviReadHandler.cpp \
 AviReadStream.cpp \
 Cache.cpp \
 FFReadHandler.cpp \
 FFReadStream.cpp \
 ReadFile.cpp \
 ReadStream.cpp \
 ReadStreamA.cpp \
 ReadStreamV.cpp \
 StreamInfo.cpp


if AMM_LINK_SHARED
FF_LIB = \
 ../../ffmpeg/libavformat/libaviplayavformat.la \
 ../../ffmpeg/libavcodec/libaviplayavcodec.la \
 ../../ffmpeg/libavutil/libaviplayavutil.la
else 
FF_LIB = \
 ../../ffmpeg/libavformat/libavformat.la \
 ../../ffmpeg/libavcodec/libavcodec.la \
 ../../ffmpeg/libavutil/libavutil.la
endif

libaviread_la_LIBADD = $(FF_LIB)

AM_CPPFLAGS = $(LTNOPIC) \
 -I$(srcdir)/../common \
 -I$(srcdir)/../../ffmpeg/libavformat \
 -I$(srcdir)/../../ffmpeg/libavcodec \
 -I$(srcdir)/../../ffmpeg/libavutil

MAINTAINERCLEANFILES = Makefile.in
