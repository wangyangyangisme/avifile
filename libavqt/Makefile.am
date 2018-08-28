if AMM_USE_QT
lib_LTLIBRARIES = libqavm.la
check_PROGRAMS = testcodecs
endif

libqavm_la_SOURCES = \
 codecdialog.cpp \
 okdialog.cpp

EXTRA_DIST = \
 audc_p.cpp audc.cpp audc_p.h audc.h

testcodecs_SOURCES = testcodecs.cpp
# hierarchyview.cpp \
# propertyeditor.cpp

noinst_HEADERS = \
 codecdialog.h \
 okdialog.h \
 pixmapbutton.h

if AMM_LINK_SHARED
LINK_AVIPLAY = ../lib/libaviplay.la
endif

#testt

#MPATROL_LIBS = -lmpatrol -lbfd -lelf
#MPATROL_CXXFLAGS = -DUSE_MPATROL
#EFENCE_LIBS = -lefence

libqavm_la_LDFLAGS = -release $(AVIFILE_MAJOR_VERSION).$(AVIFILE_MINOR_VERSION) -version-info 0:$(AVIFILE_MICRO_VERSION):0
libqavm_la_LIBADD = $(LINK_AVIPLAY) $(QT_LIBS)

TEST_LIBS = ../lib/libaviplay.la $(lib_LTLIBRARIES) \
 $(MPATROL_LIBS) $(EFENCE_LIBS)

#testt_SOURCES = testt.cpp
#testt_LDFLAGS = $(TEST_LIBS)

#order.cpp
testcodecs_LDFLAGS = $(TEST_LIBS)

#noinst_PROGRAMS = ordertest
#ordertest_SOURCES = order.cpp
#ordertest_LDADD = $(LIBRARY)
#LIBRARY = ../lib/libaviplay.la -lm

AM_CPPFLAGS = $(QT_CFLAGS) -DPIXMAP_PATH=\"$(pkgdatadir)/pixmaps\"

CLEANFILES = ordertest testcodecs *.moc

SUFFIXES = .moc .ui
%.moc: %.h
	$(MOC) < $< > $@

.h.moc:
	$(MOC) < $< > $@

# this is how we have to handle dependencies to .moc files
$(srcdir)/okdialog.cpp: okdialog.moc
$(srcdir)/codecdialog.cpp: codecdialog.moc

MAINTAINERCLEANFILES = Makefile.in

#METASOURCES = AUTO
#KDE_OPTIONS = qtonly
