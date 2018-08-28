if AMM_USE_AC3PASS
pkglib_LTLIBRARIES = ac3pass.la
endif

pkgdocdir = $(datadir)/$(PACKAGE)/doc
pkgdoc_DATA = README.ac3pass

EXTRA_DIST = Makefile.emu $(pkgdoc_DATA) $(ac3pass_la_SOURCES)

if AMM_LINK_SHARED
ac3pass_la_LIBADD = ../../lib/libaviplay.la
endif

noinst_HEADERS = ac3-iec958.h fillplugins.h
ac3pass_la_SOURCES = ac3-iec958.c libac3pass.cpp
ac3pass_la_LDFLAGS = -module -avoid-version -no-undefined
#-release $(LT_RELEASE)

AM_CPPFLAGS = $(LTNOPIC)
AM_CXXFLAGS = $(CXXRTTIEXCEPT)

#check_PROGRAMS = play_ac3
#play_ac3_SOURCES = play-ac3.c ac3-iec958.c

LINE1=Digital L
LINE2=Digital R
SPCS=1

load:	unload
	emu-config -t $(SPCS)
	emu-dspmgr -a"fx15:$(LINE1)"
	emu-dspmgr -a"fx15:$(LINE2)"
	emu-dspmgr -l "$(LINE1)" -l "$(LINE2)" -f /usr/local/share/emu10k1/ac3pass.bin

unload:
	emu-dspmgr -z
	emu-config -t 0

CLEANFILES = play-ac3
MAINTAINERCLEANFILES = Makefile.in
