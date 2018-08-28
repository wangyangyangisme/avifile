AUTOMAKE_OPTIONS = no-dependencies

noinst_LTLIBRARIES = libavcodecsparc.la

SPARCOPT_SOURCES = \
 dsputil_vis.c

noinst_HEADERS = $(SPARCOPT_SOURCES) \
 vis.h

if AMM_FF_SPARCOPT
libavcodecsparc_la_SOURCES = $(SPARCOPT_SOURCES)
endif

AM_CPPFLAGS = $(LTNOPIC) -DHAVE_AV_CONFIG_H -I$(srcdir)/../..

MAINTAINERCLEANFILES = Makefile.in
