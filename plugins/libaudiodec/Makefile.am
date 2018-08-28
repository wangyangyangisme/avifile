pkglib_LTLIBRARIES = audiodec.la

if AMM_USE_A52
SRC_A52 = a52.cpp
endif

if AMM_USE_A52BIN
FLG_A52 = -DA52BIN
else
LIB_A52 = $(A52_LIBS)
endif

if AMM_LINK_SHARED
LINK_AVIPLAY = ../../lib/libaviplay.la
endif

EXTRA_DIST = a52.cpp

audiodec_la_SOURCES = \
 audiodecoder.cpp \
 adpcm_impl.c \
 xa_gsm.c $(SRC_A52)

audiodec_la_LIBADD = $(LIB_A52) $(LINK_AVIPLAY)

noinst_HEADERS = adpcm_impl.h xa_gsm.h \
	xa_gsm_state.h aulaw.h fillplugins.h
audiodec_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS = $(LTNOPIC) $(FLG_A52) $(A52_CFLAGS)
AM_CXXFLAGS = $(CXXRTTIEXCEPT)

MAINTAINERCLEANFILES = Makefile.in
