if AMM_USE_QT
bin_PROGRAMS = avirecompress
endif

avirecompress_SOURCES = \
 audc.cpp \
 audc_p.cpp \
 conf.cpp \
 filters.cpp \
 genctl.cpp \
 genctl_p.cpp \
 main.cpp \
 qimagecontrol.cpp \
 recompressor.cpp \
 recwnd.cpp \
 recwnd_p.cpp \
 rgn.cpp \
 srcctl.cpp \
 srcctl_p.cpp \
 thread.cpp

noinst_HEADERS = \
 audc.h \
 audc_p.h \
 conf.h \
 filters.h \
 genctl.h \
 genctl_p.h \
 imagecontrol.h \
 qimagecontrol.h \
 recompressor.h \
 recwnd.h \
 recwnd_p.h \
 rec_cb.h \
 rgn.h \
 srcctl.h \
 srcctl_p.h

avirecompress_LDADD = \
	../../libavqt/libqavm.la \
	$(QT_LIBS) \
	../../lib/libaviplay.la \
	$(SDL_LIBS) $(XFT_LIBS) $(X_LIBS)

AM_CPPFLAGS = $(QT_CFLAGS) $(X_CFLAGS) \
	-DQT_THREAD_SUPPORT -I. -I$(top_srcdir)/libavqt

SUFFIXES = .moc .ui
%.moc: %.h
	$(MOC) < $< > $@

.h.moc:
	$(MOC) < $< > $@

$(srcdir)/audc.cpp: audc.moc
$(srcdir)/filters.cpp: filters.moc
$(srcdir)/genctl_p.cpp: genctl_p.moc
$(srcdir)/recwnd_p.cpp: recwnd_p.moc
$(srcdir)/srcctl_p.cpp: srcctl_p.moc
$(srcdir)/audc_p.cpp: audc_p.moc
$(srcdir)/genctl.cpp: genctl.moc
$(srcdir)/recwnd.cpp: recwnd.moc
$(srcdir)/srcctl.cpp: srcctl.moc

CLEANFILES = *.moc
MAINTAINERCLEANFILES = Makefile.in

ui:
	$(UIC) ./genctl.ui -o ./genctl_p.h
	$(UIC) -impl ./genctl_p.h ./genctl.ui -o ./genctl_p.cpp
#	$(UIC) -subdecl QtRecompressorCtl ./genctl_p.h ./genctl.ui -o ./genctl.h
#	$(UIC) -subimpl QtRecompressorCtl ./genctl.h ./genctl.ui -o ./genctl.cpp


#METASOURCES = AUTO
#KDE_OPTIONS = qtonly
