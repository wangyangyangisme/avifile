noinst_LTLIBRARIES = libcommon.la
noinst_HEADERS = frame.h Config.h
libcommon_la_SOURCES = \
 Args.cpp \
 avm_output.cpp \
 BaseError.cpp \
 BitmapInfo.cpp \
 Config.cpp \
 fcc_type.c \
 image.cpp \
 impl_audio.cpp \
 impl_video.cpp \
 Locker.cpp \
 mmx.cpp \
 Registry.cpp \
 String.cpp \
 util.cpp \
 utils.c \
 vbrctrl.c \
 wave_type.c

libcommon_la_LIBADD = $(PTHREAD_LIBS)

AM_CPPFLAGS = $(PTHREAD_CPPFLAGS)
#MemQueue* queue;
#AM_CPPFLAGS = -O2 -march=i686 -mcpu=i686
#AM_CXXFLAGS = -O2 -march=i686 -mcpu=i686
CLEANFILES = imtest testimage
# do not want to perform optimalization
#mmx.lo: mmx.cpp
#	$(CXX) -I../../include -O1 -o $@ -c $<

check_PROGRAMS = testimage
testimage_SOURCES = testimage.cpp
testimage_LDFLAGS = -g ../libaviplay.la
#testimage_CXXFLAGS = -g -O4

imtest.o: image.o mmx.o
	$(CXXCOMPILE) -DIMTEST -o $@ -c $<

imtest: imtest.o
	$(CXXCOMPILE) -g -o $@ $< -laviplay

#qtest.o: MemQueue.cpp
#	$(CXXCOMPILE) $(CXXFLAGS) -o $@ -c $<
#qtest: qtest.o
#	$(CXXCOMPILE) -g -o $@ $< -laviplay

MAINTAINERCLEANFILES = Makefile.in
