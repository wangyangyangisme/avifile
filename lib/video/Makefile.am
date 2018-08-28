noinst_LTLIBRARIES = libvideo.la
noinst_HEADERS = \
 rendlock.h \
 subrend.h

#if AMM_USE_SDL
#SDLSOURCES = renderer.cpp
#endif

libvideo_la_SOURCES = \
 sub_ft.cpp \
 renderer.cpp \
 VideoDPMS.cpp

if AMM_USE_VIDIX
if AMM_LINK_SHARED
VIDIX_LIB = ../../drivers/vidix/libaviplayvidix.la
else 
VIDIX_LIB = ../../drivers/vidix/libvidix.la
endif
endif

libvideo_la_LIBADD = $(VIDIX_LIB)
libvideo_la_LDFLAGS = $(SDL_LIBS) $(FT2_LIBS) $(ICONV_LIBS) $(X_LIBS) $(XFT_LIBS) -lXrender

AM_CPPFLAGS = $(LTNOPIC) $(SDL_CFLAGS) $(X_CFLAGS) $(XFT_CFLAGS) \
 $(PTHREAD_CFLAGS) $(ICONV_CFLAGS) $(FT2_CFLAGS) \
 -I$(srcdir)/../../drivers/vidix -DVIDIX_LIBDIR=\"$(pkglibdir)/vidix/\" \
 -I$(srcdir)/../subtitles 

MAINTAINERCLEANFILES = Makefile.in
