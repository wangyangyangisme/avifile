EXTRA_DIST = $(pkgdata_DATA)

if AMM_USE_QT
bin_PROGRAMS = aviplay
endif

aviplay_SOURCES = \
 actioncfg.cpp \
 main.cpp \
 playercontrol.cpp \
 decoder_config.cpp \
 configdialog.cpp  \
 configdialog_impl.cpp

pkgdata_DATA = \
 action.cfg

noinst_HEADERS = \
 actioncfg.h \
 configdialog.h \
 configdialog_impl.h \
 decoder_config.h \
 playercontrol.h

aviplay_LDADD = \
	../libavqt/libqavm.la \
	$(QT_LIBS) \
	../lib/libaviplay.la \
	$(SDL_LIBS) $(X_LIBS)

AM_CPPFLAGS = $(QT_CFLAGS) $(SDL_CFLAGS) $(X_CFLAGS) \
	-I$(top_srcdir)/libavqt \
	-DPIXMAP_PATH=\"$(pkgdatadir)/pixmaps\"

SUFFIXES = .moc .ui
%.moc: %.h
	$(MOC) < $< > $@

.h.moc:
	$(MOC) < $< > $@

$(srcdir)/configdialog.cpp: configdialog.moc
$(srcdir)/configdialog_impl.cpp: configdialog_impl.moc
$(srcdir)/decoder_config.cpp: decoder_config.moc
$(srcdir)/playercontrol.cpp: playercontrol.moc

CLEANFILES = *.moc
MAINTAINERCLEANFILES = Makefile.in

#METASOURCES = AUTO
#KDE_OPTIONS = qtonly
