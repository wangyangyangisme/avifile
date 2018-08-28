noinst_LTLIBRARIES = libsubtitle.la
# in include - noinst_HEADERS = subtitle.h

libsubtitle_la_SOURCES = subtitle.c subread.c subwrite.c
AM_CPPFLAGS = $(LTNOPIC)

check_PROGRAMS = subtest
subtest_SOURCES = subtest.c
subtest_LDADD = libsubtitle.la

MAINTAINERCLEANFILES = Makefile.in
