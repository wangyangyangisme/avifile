noinst_LTLIBRARIES = libaviwrite.la
noinst_HEADERS = \
 AviWrite.h \
 FileBuffer.h

libaviwrite_la_SOURCES = \
 AviWrite.cpp \
 AviAudioWriteStream.cpp \
 AviVideoWriteStream.cpp

AM_CPPFLAGS = $(LTNOPIC) $(PTHREAD_CFLAGS) -I$(srcdir)/../aviread

MAINTAINERCLEANFILES = Makefile.in
