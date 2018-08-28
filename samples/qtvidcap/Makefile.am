SUBDIRS = . scripts

if AMM_USE_QT
QT_AVICAP = avicap
else
QT_AVICAP =
endif

pkgdatadir= $(datadir)/$(PACKAGE)/i18
pkgdata_DATA = avicap_de.qm

EXTRA_DIST= $(pkgdata_DATA)

bin_PROGRAMS = kv4lsetup avirec $(QT_AVICAP)

noinst_HEADERS = \
 avicapwnd.h \
 avicapwnd_p.h \
 capproc.h \
 ccap.h \
 dsp.h \
 picprop.h \
 picprop_p.h \
 qtrenderer.h \
 qt_visual.h \
 v4lwindow.h \
 v4lxif.h \
 vidconf.h \
 vidconf_p.h \
 wintv.h \
 deinterlace-yuv.h \
 timertable.h \
 epgwindow.h \
 fullscreen_renderer.h \
 frequencies.h

# epgbrowser_SOURCES = epgbrowser.cpp epgwindow.cpp

# epgbrowser_LDADD = $(QT_LIBS)

avicap_SOURCES = \
 avicapwnd.cpp \
 avicapwnd_p.cpp \
 capproc.cpp \
 ccap.cpp \
 dsp.cpp \
 main.cpp \
 picprop.cpp \
 picprop_p.cpp \
 qtrenderer.cpp \
 qt_visual.c \
 v4lwindow.cpp \
 v4lxif.cpp \
 vidconf.cpp \
 vidconf_p.cpp \
 deinterlace-rgb.cpp \
 deinterlace-yuv.c \
 timertable.cpp \
 epgwindow.cpp \
 fullscreen_renderer.cpp \
 frequencies.c

avicap_LDADD = \
	../../libavqt/libqavm.la \
	$(QT_LIBS) \
	../../lib/libaviplay.la \
	$(SDL_LIBS) $(X_LIBS)

avirec_SOURCES=\
	deinterlace-rgb.cpp\
	deinterlace-yuv.c\
	deinterlace-yuv.h\
	capproc.cpp\
	capproc.h\
	ccap.cpp\
	ccap.h\
	dsp.cpp\
	dsp.h\
	avirec.cpp\
	v4lxif.cpp\
	v4lxif.h\
	wintv.h \
	qt_visual.c

kv4lsetup_SOURCES = kv4lsetup.cpp

kv4lsetup_LDADD = \
 ../../lib/libaviplay.la \
 $(SDL_LIBS) $(X_LIBS)
#$(X_LIBS) 
#../../lib/common/libcommon.la

#
# until fixed we still need XFree with avirec
#
avirec_LDADD = ../../lib/libaviplay.la $(SDL_LIBS) $(XFT_LIBS) $(X_LIBS)


#install-exec-local:
#	@INSTALL@ -m4755 ./kv4lsetup $(DESTDIR)@bindir@

AM_CPPFLAGS = $(QT_CFLAGS) $(X_CFLAGS) -I$(top_srcdir)/libavqt \
 -DI18N_PATH=\"$(pkgdatadir)\"

SUFFIXES = .moc .ui
%.moc: %.h
	$(MOC) < $< > $@

.h.moc:
	$(MOC) < $< > $@

$(srcdir)/avicapwnd.cpp: avicapwnd.moc
$(srcdir)/picprop.cpp: picprop.moc
$(srcdir)/v4lwindow.cpp: v4lwindow.moc
$(srcdir)/vidconf_p.cpp: vidconf_p.moc
$(srcdir)/avicapwnd_p.cpp: avicapwnd_p.moc
$(srcdir)/picprop_p.cpp: picprop_p.moc
$(srcdir)/vidconf.cpp: vidconf.moc
$(srcdir)/timertable.cpp: timertable.moc
$(srcdir)/epgwindow.cpp: epgwindow.moc
$(srcdir)/fullscreen_renderer.cpp: epgwindow.moc

CLEANFILES = *.moc
MAINTAINERCLEANFILES = Makefile.in

ui:
	$(UIC) ./capwindow.ui -o ./capwindow_p.h
	$(UIC) -impl ./capwindow_p.h ./capwindow.ui -o ./capwindow_p.cpp
#	$(UIC) -subdecl CapWindow ./capwindow_p.h ./config.ui -o ./capwindow.h
#	$(UIC) -subimpl CapWindow ./capwindow.h ./capwindow.ui -o ./capwindow.cpp

#METASOURCES = AUTO
#KDE_OPTIONS = qtonly
